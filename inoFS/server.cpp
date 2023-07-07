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
#include "main.h"
#include "sim.h"
#include "server.h"

Server::Server(inoFS *inofs) {
	this->inofs = inofs;
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
		ZeroMemory(&recvBuf, BufLen);
		int bytes = recvfrom(sock, recvBuf, BufLen, 0, (SOCKADDR*)&senderAddr, &senderAddrSize);
		if (bytes <= 0) {continue;}
		received_mutex.lock();
		received.push(std::string(recvBuf, bytes));
		received_mutex.unlock();
		AddClientIfNew(senderAddr);
	}
}

void Server::Loop() {
	CheckClients();
	ProcessPackets();
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
		printf("New client connected %s\n", inet_ntoa(client.addr.sin_addr));
	}
	clients_mutex.unlock();
}

void Server::CheckClients() {
	clients_mutex.lock();
	time_t t = time(NULL);
	for (auto client = clients.begin(); client != clients.end(); ++client) {
		if (client->lastPing + TIMEOUT < t) {
			printf("Client timed-out %s\n", inet_ntoa(client->addr.sin_addr));
			clients.erase(client--);
		}
	}
	clients_mutex.unlock();
}

void Server::ProcessPackets() {
	received_mutex.lock();
	while (!received.empty()) {
		std::string packet = received.front();
		if (packet.compare("M;") >= 0) {
			this->inofs->sim->Monitor(packet);
		} else if (packet.compare("C;") >= 0) {
			this->inofs->sim->Control(packet);
		} else {
			this->inofs->sim->Input(packet);
		}
		received.pop();
	}
	received_mutex.unlock();
}

void Server::Broadcast(char *ptr, int bytes) {
	clients_mutex.lock();
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		sendto(sock, ptr, bytes, 0, (SOCKADDR*)&c->addr, sizeof(c->addr));
	}
	clients_mutex.unlock();
}

