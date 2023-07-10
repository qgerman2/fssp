#include <stdio.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <string>
#include <time.h>
#include <WinSock2.h> 
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "serialib/serialib.h"
#include "main.h"
#include "sim.h"
#include "server.h"
#include "ui.h"

Server::Server(inoFS *inofs) {
	this->inofs = inofs;
	// Inicializar WinSock
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	UpdateLocalIPs();
	// Structs para dirección local
	sockaddr_in localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(SERVER_PORT);
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// Sockets y bind
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind(sock, (SOCKADDR*)&localAddr, sizeof(localAddr));
	threadUDP = std::thread(&Server::ThreadUDP, this);
}

// https://learn.microsoft.com/windows/win32/api/iphlpapi/nf-iphlpapi-getipaddrtable
void Server::UpdateLocalIPs() {
	localips.clear();
	PMIB_IPADDRTABLE pIPAddrTable;
	pIPAddrTable = (MIB_IPADDRTABLE *) HeapAlloc(GetProcessHeap(), 0, (sizeof(MIB_IPADDRTABLE)));
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	GetIpAddrTable(pIPAddrTable, &dwSize, 0);
	pIPAddrTable = (MIB_IPADDRTABLE *) HeapAlloc(GetProcessHeap(), 0, (dwSize));
	dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 );
	int i;
	IN_ADDR IPAddr;
	for (i=0; i < (int) pIPAddrTable->dwNumEntries; i++) {
		IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwAddr;
		localips.push_back(inet_ntoa(IPAddr));
	}
	HeapFree(GetProcessHeap(), 0, (pIPAddrTable));
}

void Server::PollSerialDevices(bool autoscan) {
	if (!autoscan) {
		dprintf("Scanning for serial devices...\n");
	}
	for (int i = 1; i < 99; i++) {
		char device_name[64];
		sprintf(device_name, "\\\\.\\COM%d", i);
		if (serial.openDevice(device_name, BAUD_RATE) == 1) {
			serial.closeDevice();
			ClientID id;
			id.device = std::string(device_name);
			id.isSerial = true;
			AddClientIfNew(id);
		}
	}
}

void Server::ThreadUDP() {
	char recvBuf[1024];
	int BufLen = 1024;
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);
	while (true) {
		ZeroMemory(&recvBuf, BufLen);
		int bytes = recvfrom(sock, recvBuf, BufLen, 0, (SOCKADDR*)&senderAddr, &senderAddrSize);
		if (bytes <= 0) {continue;}
		ClientID id;
		id.addr = senderAddr;
		id = AddClientIfNew(id);
		received_mutex.lock();
		std::pair<std::string, ClientID> msg;
		msg.first = std::string(recvBuf, bytes);
		msg.second = id;
		received.push(msg);
		received_mutex.unlock();
	}
}

// Need to fix all of this up its really ugly
void Server::ThreadSerial(ClientID id) {
	serialib serial;
	if (serial.openDevice(id.device.c_str(), BAUD_RATE)) {
		serial.DTR(true);
		serial.RTS(true);
		while (true) {
			// write 
			clients_mutex.lock();
			std::vector<std::string> msgs;
			Client *client;
			if (GetClient(id, &client)) {
				client->serial.tosend_mutex->lock();
				if (!client->serial.tosend.empty()) {
					msgs = client->serial.tosend;
					client->serial.tosend.clear();
				}
				client->serial.tosend_mutex->unlock();
			}
			clients_mutex.unlock();
			for (auto packet = msgs.begin(); packet != msgs.end(); packet++){
				if (serial.writeBytes(packet->c_str(), packet->length()) == -1) {
					dprintf("Error during serial write\n");
					break;
				}
			}
			// read
			if (serial.available()) {
				char recvBuf[1024];
				int BufLen = 1024;
				int bytes = serial.readBytes(recvBuf, 5);
				if (bytes == -1 || bytes == -2) {
					dprintf("Error during serial reading\n");
					break;
				} else if (bytes > 0) {
					if (strncmp(recvBuf, "inofs", 5) == 0) {
						char len[4];
						int len_bytes = serial.readBytes(len, 4);
						if (len_bytes == -1 || len_bytes == -2) {
							dprintf("Error during serial reading\n");
							break;
						}
						if (len_bytes == 4) {
							bytes = serial.readBytes(recvBuf, *(int*)len);
							if (bytes == -1 || bytes == -2) {
								dprintf("Error during serial reading\n");
								break;
							}
						} else {
							dprintf("Error reading message length\n");
							break;
						}
					} else {
						int more_bytes = serial.readString(&recvBuf[5], '\n', 1024-5);
						if (more_bytes == -1 || more_bytes == -2) {
							dprintf("Error during serial reading\n");
							break;
						}
						bytes = bytes + more_bytes;
					}
					received_mutex.lock();
					std::pair<std::string, ClientID> msg;
					msg.first = std::string(recvBuf, bytes);
					msg.second = id;
					received.push(msg);
					received_mutex.unlock();
				}
			}
		}
		serial.closeDevice();
	}
	clients_mutex.lock();
	Client *client;
	if (GetClient(id, &client)) {
		client->serial.running = false;
	} else {
		dprintf("Something went very wrong\n");
	}
	clients_mutex.unlock();
}

void Server::Loop() {
	CheckClients();
	ProcessPackets();
}

ClientID Server::AddClientIfNew(ClientID id) {
	clients_mutex.lock();
	int num = 0;
	if (!clients.empty()) {
		num = clients.back().id.num + 1;
	}
	bool newClient = true;
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		if (c->id.addr.sin_addr.S_un.S_addr == id.addr.sin_addr.S_un.S_addr
		|| c->id.device == id.device) {
			c->lastPing = time(NULL);
			newClient = false;
			id = c->id;
			break;
		}
	}
	if (newClient) {
		Client client;
		client.id = id;
		client.id.num = num;
		client.id.addr.sin_port = htons(CLIENT_PORT);
		client.lastPing = time(NULL);
		if (client.id.isSerial) {
			client.serial.thread = new std::thread(&Server::ThreadSerial, this, client.id);
			client.serial.tosend_mutex = new std::mutex();
			client.serial.running = true;
			dprintf("New client with ID %d (%s)\n", client.id.num, client.id.device.c_str());
		} else {
			dprintf("New client with ID %d (%s)\n", client.id.num, inet_ntoa(client.id.addr.sin_addr));
		}
		clients.push_back(client);
		id = client.id;
	}
	clients_mutex.unlock();
	return id;
}

void Server::CheckClients() {
	bool haveSerialClients = false;
	clients_mutex.lock();
	time_t t = time(NULL);
	auto client = clients.begin();
	while (client != clients.end()) {
		// Send monitor values
		// Don't spam the microcontroller
		if (!client->serial.sentMonitor) {
			//PrintValues();
			inofs->sim->SendValues(0, &(*client), &client->monitor);
			if (client->id.isSerial) {
				client->serial.sentMonitor = true;
			}
		}
		// Check if theyre alive
		// Probably should do these in inverse order
		if (!client->id.isSerial) { 
			if (client->lastPing + TIMEOUT < t) {
				dprintf("Client %d timed-out\n", client->id);
				client = clients.erase(client);
			} else {
				client++;
			}
		} else {
			if (!client->serial.running) {
				dprintf("Client %d stopped running\n", client->id);
				client->serial.thread->join();
				delete client->serial.thread;
				delete client->serial.tosend_mutex;
				client = clients.erase(client);
			} else {
				haveSerialClients = true;
				client++;
			}
		}
	}
	clients_mutex.unlock();
	if (!haveSerialClients && serialEnabled) {
		PollSerialDevices(true);
	}
}

void Server::ProcessPackets() {
	received_mutex.lock();
	clients_mutex.lock();
	while (!received.empty()) {
		std::string packet = received.front().first;
		Client *client;
		if (GetClient(received.front().second, &client)) {
			if (client->id.isSerial) {
				client->serial.sentMonitor = false;
			}
			inofs->ui->PrintComms("ID %d recv: %s", client->id.num, packet.c_str());
			if (packet.compare(0, 1, "d") == 0) {
				client->double_precision = true;
			} else if (packet.compare(0, 1, "f") == 0) {
				client->double_precision = false;
			}
			if (packet.compare(1, 2, "M;") == 0) {
				this->inofs->sim->Monitor(packet, client);
			} else if (packet.compare(1, 2, "C;") == 0) {
				this->inofs->sim->Control(packet, client);
			} else if (packet.compare(1, 2, "R:") == 0) {
				this->inofs->sim->Read(packet, client);
			} else if (packet.compare(1, 2, "W;") == 0) {
				this->inofs->sim->Write(packet, client);
			} else {
				this->inofs->sim->Input(packet, client->control, client->double_precision);
			}
		}
		received.pop();
	}
	received_mutex.unlock();
	clients_mutex.unlock();
}

// This functions assumes clients has a mutex lock
// , otherwhise these pointers are not safe
bool Server::GetClient(ClientID id, Client **client) {
	for (auto c = clients.begin(); c != clients.end(); c++) {
		if (id.num == c->id.num) {
			*client = &(*c);
			return true;
		}
	}
	return false;
}

void Server::Broadcast(ClientID id, char *ptr, int bytes) {
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		if (id.num == c->id.num) {
			inofs->ui->PrintComms("ID %d send: %d bytes\n", c->id.num, bytes);
			if (id.isSerial) {
				Client *client;
				if (GetClient(id, &client)) {
					client->serial.tosend_mutex->lock();
					client->serial.tosend.push_back(
						std::string("inofs") +
						std::string((char*)&bytes, sizeof(int)) +
						std::string((char*)&client->double_precision, sizeof(bool)) +
						std::string(ptr, bytes)
					);
					client->serial.tosend_mutex->unlock();
				}
			} else {
				sendto(sock, ptr, bytes, 0, (SOCKADDR*)&c->id.addr, sizeof(c->id.addr));
			}
			break;
		}
	}
}

std::vector<Client> Server::GetClients() {
	return clients;
}

std::vector<std::string> Server::GetLocalIPs() {
	return localips;
}