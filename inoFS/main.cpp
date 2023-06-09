#include <windows.h>
#include <signal.h>
#include <vector>
#include "main.h"
#include "sim.h"
#include "server.h"
#include "ui.h"

int main() {
	inoFS inofs;
	inofs.server = new Server(&inofs);
	inofs.sim = new Sim(&inofs);
	inofs.ui = new UI(&inofs);
	while (true) {
		inofs.server->Loop();
		inofs.sim->Loop();
		inofs.ui->Loop();
		Sleep(1);
	}
	return 1;
}