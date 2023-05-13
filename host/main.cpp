#include <iostream>
#include <windows.h>
#include <signal.h>
#include <vector>
#include "main.h"
#include "sim.h"
#include "server.h"

int main() {
	SetConsoleCtrlHandler(ctrlEvent, true);
	FSSP fssp;
	fssp.server = new Server(&fssp);
	fssp.sim = new Sim(&fssp);
	
	while (true) {
		fssp.server->Loop();
		fssp.sim->Loop();
		Sleep(100);
	}
	return 1;
}

BOOL WINAPI ctrlEvent(DWORD signal) {
	exit(1);
	return true;
}