#ifndef SERVER
#define SERVER
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <time.h>
#include "serialib/serialib.h"
#include "sim.h"

#define TIMEOUT 10
#define BAUD_RATE 115200
#define SERVER_PORT 27015
#define CLIENT_PORT 27016

struct Client {
	int id;
	bool double_precision = false;
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
		serialib serial;

		void UpdateLocalIPs();
		void PollSerialDevices();
		void Thread();
		int AddClientIfNew(sockaddr_in address);
		void CheckClients();
		void ProcessPackets();
		bool GetClient(int id, Client**);
	public:
		Server(inoFS *inofs);
		void Loop();
		void Broadcast(int id, char *ptr, int bytes);
		std::vector<Client> GetClients();
		std::vector<std::string> GetLocalIPs();
};
#endif