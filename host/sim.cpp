#include <Windows.h>
#include <iostream>
#include <map>
#include <string>
#include "FSUIPC_User.h"
#include "sim.h"

Sim::Sim() {
	connected = false;
}

bool Sim::open() {
	close();
	if (FSUIPC_Open(SIM_ANY, &dwResult)) {
		connected = true;
		return true;
	}
	return false;
};

void Sim::close() {
	connected = false;
	FSUIPC_Close();
}

void Sim::monitor(int offset, int size) {
	std::string *value = new std::string(size, 0);
	offsets.insert(std::pair<int, std::string*>(offset, value));
}

void Sim::poll() {
    for (auto it = offsets.begin(); it != offsets.end(); ++it) {
        FSUIPC_Read(it->first, it->second->size(), &it->second->at(0), &dwResult);
    }
	if (!FSUIPC_Process(&dwResult)) {close();}
}

void Sim::printOffsets() {
    for (auto it = offsets.begin(); it != offsets.end(); ++it) {
        printf("Offset: %X Value: %d \n", it->first, *(int*)it->second);
    }
}