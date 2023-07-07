#include <windows.h>
#include <signal.h>
#include <vector>
#include "main.h"
#include "sim.h"
#include "server.h"
#include "ui.h"
#include "dbg.h"

int main() {
	inoFS inofs;
	dprintf("test %d", 5);
	inofs.server = new Server(&inofs);
	inofs.sim = new Sim(&inofs);
	inofs.ui = new UI(&inofs);
	while (true) {
		inofs.server->Loop();
		inofs.sim->Loop();
		inofs.ui->Loop();
		Sleep(100);
	}
	return 1;
}