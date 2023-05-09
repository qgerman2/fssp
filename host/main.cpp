#include <iostream>
#include "main.h"
#include "monitor.h"
#include <windows.h>
#include <signal.h>
#include "FSUIPC_User.h"

DWORD dwResult;
int main() {
	SetConsoleCtrlHandler(ctrlEvent, true);
	addMonitor(0x238, 1);
	addMonitor(0x239, 1);
	addMonitor(0x23A, 1);
	addMonitor(0x2B4, 4);
	
	while (true) {
		if (FSUIPC_Open(SIM_ANY, &dwResult)) {
			std::cout << "Conectado\n";
			loop();
			std::cout << "Desconectado\n";
			FSUIPC_Close();
		} else {
			std::cout << "Intento de conexión fallido\n";
		}
		Sleep(1000);
	}
	return 1;
}

void loop() {
	while (true) {
		queryMonitor(&dwResult);
		if (!FSUIPC_Process(&dwResult)) {break;}
		printMonitor();
		Sleep(1000);
	}
}

BOOL WINAPI ctrlEvent(DWORD signal) {
	exit(1);
	return true;
}