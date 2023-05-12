#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <string>
#include <time.h>
#include <WinSock2.h> 
#include <ws2tcpip.h>
#include "server.h"

Server::Server() {
	// Inicializar WinSock
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
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

void Server::Thread() {
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);
	while (true) {
		recvfrom(sock, recvBuf, BufLen, 0, (SOCKADDR*)&senderAddr, &senderAddrSize);
		received_mutex.lock();
		received.push(std::string(recvBuf));
		received_mutex.unlock();
		AddClientIfNew(senderAddr);
		ZeroMemory(&recvBuf, BufLen);
	}
}

void Server::Loop() {
	CheckClients();
}

void Server::AddClientIfNew(sockaddr_in remote) {
	clients_mutex.lock();
	bool newClient = true;
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		if (c->addr.sin_addr.S_un.S_addr == remote.sin_addr.S_un.S_addr) {
			c->lastPing = time(NULL);
			newClient = false;
			break;
		}
	}
	if (newClient) {
		Client client;
		client.addr = remote;
		client.addr.sin_port = htons(CLIENT_PORT);
		client.lastPing = time(NULL);
		clients.push_back(client);
	}
	clients_mutex.unlock();
}

void Server::CheckClients() {
	clients_mutex.lock();
	time_t t = time(NULL);
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		struct {
			int a = 50;
			int b = 60;
			int c = 70;
		} msg;
		sendto(sock, (char*)&msg, sizeof(msg), 0, (SOCKADDR*)&c->addr, sizeof(c->addr));
		if (c->lastPing + TIMEOUT < t) {
			clients.erase(c--);
		}
	}
	clients_mutex.unlock();
}

std::vector<Packet> Server::GetPackets() {
	received_mutex.lock();
	std::vector<Packet> packets;
	while (!received.empty()) {
		Packet packet;
		packet.data = received.front();
		packet.type = -1;
		if (packet.data.compare("M;") >= 0) {packet.type = 0;}
		if (packet.type > -1) {
			packets.push_back(packet);
		}
		received.pop();
	}
	received_mutex.unlock();
	return packets;
}

