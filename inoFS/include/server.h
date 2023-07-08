#ifndef SERVER
#define SERVER
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <time.h>
#include "sim.h"

#define TIMEOUT 10
#define SERVER_PORT 27015
#define CLIENT_PORT 27016

struct Client {
	int id;
	struct sockaddr_in addr;
	time_t lastPing;
	std::vector<Offset> monitor;
	std::vector<Offset> control;
};

class inoFS;
class Server {
	private:
		inoFS *inofs;
		std::vector<std::string> localips;
		std::vector<Client> clients;
		std::thread thread;
		std::queue<std::pair<std::string, int>> received;
		std::mutex clients_mutex;	
		std::mutex received_mutex;

		SOCKET sock;
		char recvBuf[1024];
		int BufLen = 1024;

		char hostname[256];

		void UpdateLocalIPs();
		void Thread();
		int AddClientIfNew(sockaddr_in address);
		void CheckClients();
		void ProcessPackets();
		bool GetClient(int id, Client**);
	public:
		Server(inoFS *inofs);
		void Loop();
		void Broadcast(char *ptr, int bytes);
		std::vector<Client> GetClients();
		std::vector<std::string> GetLocalIPs();
};
#endif