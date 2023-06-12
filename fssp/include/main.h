#ifndef MAIN
#define MAIN
#include <windows.h>
class Sim;
class Server;
class FSSP {
	public:
		Sim *sim;
		Server *server;
		FSSP() {};
};
int main();
void loop();
void process();
BOOL WINAPI ctrlEvent(DWORD signal);
#endif