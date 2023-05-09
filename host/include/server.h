#ifndef SERVER
#define SERVER
#include <queue>
#include <string>
void main_server(std::queue<std::string> *msgs);
class Server {
	private:
		std::queue<std::string> *msgs;
		WSADATA wsaData;
		SOCKET RecvSocket;
		char RecvBuf[1024];
		int BufLen = 1024;
		unsigned short Port = 27015;
		struct sockaddr_in SenderAddr;
		struct sockaddr_in RecvAddr;
		int SenderAddrSize;
	public:
		Server(std::queue<std::string> *m);
		void Loop();
};
#endif