#include <iostream>
#include <windows.h>
#include <signal.h>
#include <vector>
#include "main.h"
#include "sim.h"
#include "server.h"

Server server;
Sim sim;

int main() {
	SetConsoleCtrlHandler(ctrlEvent, true);
	//sim.Monitor("M;23a:c;2b8:i;28c0:d;");
	while (true) {
		server.Loop();
		process(server.GetPackets());
		sim.Loop();
		Sleep(10);
	}
	return 1;
}

void process(std::vector<Packet> packets) {
	for (auto packet = packets.begin(); packet != packets.end(); packet++) {
		switch (packet->type) {
			case 0:
				sim.Monitor(packet->data);
				break;
		}
	}
}

BOOL WINAPI ctrlEvent(DWORD signal) {
	exit(1);
	return true;
}