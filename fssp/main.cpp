#include <windows.h>
#include <signal.h>
#include <vector>
#include "main.h"
#include "sim.h"
#include "server.h"
#include "ui.h"
#include "dbg.h"

int main() {
	FSSP fssp;
	dprintf("test %d", 5);
	fssp.server = new Server(&fssp);
	fssp.sim = new Sim(&fssp);
	fssp.ui = new UI(&fssp);
	while (true) {
		fssp.server->Loop();
		fssp.sim->Loop();
		fssp.ui->Loop();
		Sleep(100);
	}
	return 1;
}