#ifndef SERVER
#define SERVER
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <time.h>

#define TIMEOUT 10
#define SERVER_PORT 27015
#define CLIENT_PORT 27016

struct Client {
	struct sockaddr_in addr;
	time_t lastPing;
};

class inoFS;
class Server {
	private:
		inoFS *inofs;
		std::vector<Client> clients;
		std::thread thread;
		std::queue<std::string> received;
		std::mutex clients_mutex;		
		std::mutex received_mutex;

		SOCKET sock;
		char recvBuf[1024];
		int BufLen = 1024;

		void Thread();
		void AddClientIfNew(sockaddr_in address);
		void CheckClients();
		void ProcessPackets();
	public:
		Server(inoFS *inofs);
		void Loop();
		void Broadcast(char *ptr, int bytes);
};
#endif