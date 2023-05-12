#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "FSUIPC_User.h"
#include "sim.h"

Sim::Sim() {
	connected = false;
}

void Sim::Loop() {
	if (!connected) {
		Open();
	}
	if (connected) {
		if (Poll()) {
			PrintValues();
		}
	}
}

bool Sim::Open() {
	if (connected) {Close();}
	if (FSUIPC_Open(SIM_ANY, &FSUIPCResult)) {
		connected = true;
		printf("Connected to flight sim!\n");
		return true;
	}
	return false;
};

void Sim::Close() {
	connected = false;
	printf("Disconnected from flight sim\n");
	FSUIPC_Close();
}

bool Sim::Monitor(std::string str) {
	offsets.clear();
	if (str.compare("M;") >= 0) {
		int s = 2;
		int e;
		int m;
		while (true) {
			Offset offset;
			e = str.find(";", s);
			m = str.find(":", s);
			if (e == std::string::npos) {break;}
			if (m == std::string::npos) {break;}
			m++;
			if (m < e) {
				try {
					offset.location = std::stoul(str.substr(s, m - s), NULL, 16);
					std::string type_str = str.substr(m, e - m);
					offset.type = -1;
					if (type_str == "uc") {offset.type = 0; offset.size = 1;}
					else if (type_str == "us") {offset.type = 1; offset.size = 2;}
					else if (type_str == "ui") {offset.type = 2; offset.size = 4;}
					else if (type_str == "ul") {offset.type = 3; offset.size = 8;}
					else if (type_str == "c") {offset.type = 4; offset.size = 1;}
					else if (type_str == "s") {offset.type = 5; offset.size = 2;}
					else if (type_str == "i") {offset.type = 6; offset.size = 4;}
					else if (type_str == "l") {offset.type = 7; offset.size = 8;}
					else if (type_str == "f") {offset.type = 8; offset.size = 4;}
					else if (type_str == "d") {offset.type = 9; offset.size = 8;}
					if (offset.type > -1) {
						offset.value = 0;
						offsets.push_back(offset);
					}
				} catch (...) {
					printf("ERROR: Failed to parse monitor string\n");
					return false;
				}
			}
			s = e + 1;
		}
	}
	return true;
}

bool Sim::Poll() {
    for (auto offset = offsets.begin(); offset != offsets.end(); offset++) {
		char data[8];
		FSUIPC_Read(offset->location, offset->size, &data, &FSUIPCResult);
		if (!FSUIPC_Process(&FSUIPCResult)) {
			Close();
			return false;
		}
		if (offset->type == 0) {offset->value = *(u_char*)&data;}
		else if (offset->type == 1) {offset->value = *(u_short*)&data;}
		else if (offset->type == 2) {offset->value = *(u_int*)&data;}
		else if (offset->type == 3) {offset->value = *(u_long*)&data;}
		else if (offset->type == 4) {offset->value = *(char*)&data;}
		else if (offset->type == 5) {offset->value = *(short*)&data;}
		else if (offset->type == 6) {offset->value = *(int*)&data;}
		else if (offset->type == 7) {offset->value = *(long*)&data;}
		else if (offset->type == 8) {offset->value = *(float*)&data;}
		else if (offset->type == 9) {offset->value = *(double*)&data;}
	}
	return true;
}

void Sim::PrintValues() {
    for (auto offset = offsets.begin(); offset != offsets.end(); offset++) {
		printf("location: %x, type: %d, value: %f\n", offset->location, offset->type, offset->value);
	}
}