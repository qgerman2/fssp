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

struct ClientID {
	int num;
	struct sockaddr_in addr;
	std::string device;
	bool isSerial = false;
};

struct Serial {
	bool running = false;
	std::thread *thread;
	std::vector<std::string> tosend;
	std::mutex *tosend_mutex;
};

struct Client {
	ClientID id;
	bool double_precision = false;
	time_t lastPing;
	std::vector<Offset> monitor;
	std::vector<Offset> control;
	Serial serial;
};

class inoFS;
class Server {
	private:
		inoFS *inofs;
		// UDP Data structures
		SOCKET sock;
		std::vector<std::string> localips;
		std::thread threadUDP;
		// Serial data structures
		serialib serial;
		// Shared
		std::queue<std::pair<std::string, ClientID>> received;
		std::mutex received_mutex;
		std::mutex clients_mutex;

		void UpdateLocalIPs();
		void ThreadUDP();
		void ThreadSerial(ClientID id);
		ClientID AddClientIfNew(ClientID id);
		void CheckClients();
		void ProcessPackets();
		bool GetClient(ClientID id, Client**);
	public:
		std::vector<Client> clients;
		std::vector<Client> GetClients();
		std::vector<std::string> GetLocalIPs();
		bool serialEnabled = true;
		Server(inoFS *inofs);
		void Loop();
		void Broadcast(ClientID id, char *ptr, int bytes);
		void PollSerialDevices(bool autoscan);		
};
#endif