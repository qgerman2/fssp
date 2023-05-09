#include <Windows.h>
#include <iostream>
#include <map>
#include <string>
#include "FSUIPC_User.h"

std::map<int, std::string*> monitor;

void addMonitor(int offset, int size) {
	std::string* value = new std::string(size, 0);
	monitor.insert(std::pair<int, std::string*>(offset, value));
}

void queryMonitor(DWORD* dwResult) {
    for (auto it = monitor.begin(); it!=monitor.end(); ++it) {
        FSUIPC_Read(it->first, it->second->size(), &it->second->at(0), dwResult);
    }
}

void printMonitor() {
    for (auto it = monitor.begin(); it!=monitor.end(); ++it) {
        printf("Offset: %X Value: %d \n", it->first, *(int*)it->second);
    }
}