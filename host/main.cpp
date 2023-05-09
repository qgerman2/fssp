#include <iostream>
#include <thread>
#include <windows.h>
#include <signal.h>
#include "main.h"
#include "sim.h"
#include "server.h"


int main() {
	SetConsoleCtrlHandler(ctrlEvent, true);

	std::queue<std::string> msgs;
	std::thread t(main_server, &msgs);
	
	Sim sim;
	sim.monitor(0x238, 1);
	sim.monitor(0x239, 1);
	sim.monitor(0x23A, 1);
	sim.monitor(0x2B4, 4);
	while (true) {
		if (!sim.connected) {
			sim.open();
		}
		if (sim.connected) {
			sim.poll();
			sim.printOffsets();
		}
		if (!msgs.empty()) {
			std::cout << msgs.front();
			std::cout << "mensaje de la queue";
			msgs.pop();
		}
		Sleep(1000);
	}
	return 1;
}

BOOL WINAPI ctrlEvent(DWORD signal) {
	exit(1);
	return true;
}