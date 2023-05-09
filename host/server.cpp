#include <iostream>
#include <queue>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include "server.h"

Server::Server(std::queue<std::string> *m) {
	msgs = m;
	SenderAddrSize = sizeof(SenderAddr);
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(RecvSocket, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
}

void Server::Loop() {
	while (true) {
		wprintf(L"Receiving datagrams...\n");
		recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
		msgs->push(std::string(RecvBuf));
		std::cout << RecvBuf;
	}
};

void main_server(std::queue<std::string> *m) {
	Server server(m);
	server.Loop();
}