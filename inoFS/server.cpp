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
	thread = std::thread(&Server::Thread, this);
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

void Server::Thread() {
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);
	while (true) {
		ZeroMemory(&recvBuf, BufLen);
		int bytes = recvfrom(sock, recvBuf, BufLen, 0, (SOCKADDR*)&senderAddr, &senderAddrSize);
		if (bytes <= 0) {continue;}
		int id = AddClientIfNew(senderAddr);
		received_mutex.lock();
		std::pair<std::string, int> msg;
		msg.first = std::string(recvBuf, bytes);
		msg.second = id;
		received.push(msg);
		received_mutex.unlock();
	}
}

void Server::Loop() {
	CheckClients();
	ProcessPackets();
}

int Server::AddClientIfNew(sockaddr_in remote) {
	clients_mutex.lock();
	int id = 0;
	if (!clients.empty()) {
		id = clients.back().id + 1;
	}
	bool newClient = true;
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		if (c->addr.sin_addr.S_un.S_addr == remote.sin_addr.S_un.S_addr) {
			c->lastPing = time(NULL);
			newClient = false;
			id = c->id;
			break;
		}
	}
	if (newClient) {
		Client client;
		client.id = id;
		client.addr = remote;
		client.addr.sin_port = htons(CLIENT_PORT);
		client.lastPing = time(NULL);
		clients.push_back(client);
		dprintf("New client connected %s, id %d\n", inet_ntoa(client.addr.sin_addr), id);
	}
	clients_mutex.unlock();
	return id;
}

void Server::CheckClients() {
	clients_mutex.lock();
	time_t t = time(NULL);
	auto client = clients.begin();
	while (client != clients.end()) {
		if (client->lastPing + TIMEOUT < t) {
			dprintf("Client timed-out %s, id %d", inet_ntoa(client->addr.sin_addr), client->id);
			client = clients.erase(client);
		} else {
			client++;
		}
	}
	clients_mutex.unlock();
}

void Server::ProcessPackets() {
	received_mutex.lock();
	clients_mutex.lock();
	while (!received.empty()) {
		std::string packet = received.front().first;
		Client* client;
		if (GetClient(received.front().second, &client)) {
			if (packet.compare("M;") >= 0) {
				this->inofs->sim->Monitor(packet, &client->monitor);
			} else if (packet.compare("C;") >= 0) {
				this->inofs->sim->Control(packet, &client->control);
			} else {
				this->inofs->sim->Input(packet, client->control);
			}
		}
		received.pop();
	}
	received_mutex.unlock();
	clients_mutex.unlock();
}

// This functions assumes clients has a mutex lock
// , otherwhise these pointers are not safe
bool Server::GetClient(int id, Client **client) {
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		if (id == c->id) {
			*client = &(*c);
			return true;
		}
	}
	return false;
}

void Server::Broadcast(int id, char *ptr, int bytes) {
	clients_mutex.lock();
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		if (id == c->id) {
			sendto(sock, ptr, bytes, 0, (SOCKADDR*)&c->addr, sizeof(c->addr));
			break;
		}
	}
	clients_mutex.unlock();
}

std::vector<Client> Server::GetClients() {
	return clients;
}

std::vector<std::string> Server::GetLocalIPs() {
	return localips;
}