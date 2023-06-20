#ifndef MAIN
#define MAIN
#include <windows.h>
class Sim;
class Server;
class UI;
class FSSP {
	public:
		Sim *sim;
		Server *server;
		UI *ui;
		FSSP() {};
};
int main();
void loop();
void process();
#endif