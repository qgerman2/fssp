#ifndef MAIN
#define MAIN
#include <windows.h>
class Sim;
class Server;
class UI;
class inoFS {
	public:
		Sim *sim;
		Server *server;
		UI *ui;
		inoFS() {};
};
int main();
void loop();
void process();
#endif