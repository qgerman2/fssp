#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "FSUIPC/FSUIPC_User.h"
#include "main.h"
#include "sim.h"
#include "server.h"
#include "ui.h"

const char *FSIUPCErrors[] =
	{	"Okay",
		"Attempt to Open when already Open",
		"Cannot link to FSUIPC or WideClient",
		"Failed to Register common message with Windows",
		"Failed to create Atom for mapping filename",
		"Failed to create a file mapping object",
		"Failed to open a view to the file map",
		"Incorrect version of FSUIPC, or not FSUIPC",
		"Sim is not version requested",
		"Call cannot execute, link not Open",
		"Call cannot execute: no requests accumulated",
		"IPC timed out all retries",
		"IPC sendmessage failed all retries",
		"IPC request contains bad data",
		"Maybe running on WideClient, but FS not running on Server, or wrong FSUIPC",
		"Read or Write request cannot be added, memory for Process is full",
	};

Sim::Sim(inoFS *inofs) {
	this->inofs = inofs;
	connected = false;
}

void Sim::Loop() {
	if (!connected) {
		Open();
	}
	if (connected) {
		auto clients = inofs->server->GetClients();
		if (Poll(&clients)) {
			//PrintValues();
			SendValues(&clients);
		}
	}
}

bool Sim::Open() {
	if (connected) {Close();}
	if (FSUIPC_Open(SIM_ANY, &FSUIPCResult)) {
		connected = true;
		dprintf("Connected to flight sim!\n");
		return true;
	}
	return false;
};

void Sim::Close() {
	connected = false;
	dprintf("Disconnected from flight sim %s", FSIUPCErrors[FSUIPCResult]);
	FSUIPC_Close();
}

bool Sim::Poll(std::vector<Client> *clients) {
	for (auto c = clients->begin(); c != clients->end(); ++c) {
		if (c->monitor.size() == 0) {return false;}
		for (auto offset = c->monitor.begin(); offset != c->monitor.end(); offset++) {
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
	}
	return true;
}

void Sim::PrintValues() {
	auto clients = inofs->server->GetClients();
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		for (auto offset = c->monitor.begin(); offset != c->monitor.end(); offset++) {
			dprintf("location: %x, type: %d, value: %f\n", offset->location, offset->type, offset->value);
		}
	}
}

void Sim::SendValues(std::vector<Client> *clients) {
	for (auto c = clients->begin(); c != clients->end(); ++c) {
		std::vector<double> values;
		for (auto offset = c->monitor.begin(); offset != c->monitor.end(); offset++) {
			values.push_back(offset->value);
		}
		this->inofs->server->Broadcast((char*)values.data(), values.size() * sizeof(double));
	}
}

bool Sim::ParseOffsets(std::string str, std::vector<Offset> *dest) {
	dest->clear();
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
					dest->push_back(offset);
				} else {
					dprintf("ERROR: Unrecognized data type '%s'\n", type_str.c_str());
					return false;
				}
			} catch (...) {
				dprintf("ERROR: Failed to parse offset string\n");
				return false;
			}
		}
		s = e + 1;
	}
	return true;
}

bool Sim::Monitor(std::string str, std::vector<Offset> *monitor) {
	if (ParseOffsets(str, monitor)) {
		dprintf("Set up monitor with %d variables\n", monitor->size());
		return true;
	}
	return false;
}

bool Sim::Control(std::string str, std::vector<Offset> *control) {
	if (ParseOffsets(str, control)) {
		dprintf("Set up control with %d variables\n", control->size());
		return true;
	}
	return false;
}

void Sim::Input(std::string str, std::vector<Offset> control) {
	if (control.size() == 0) {return;}
	const int len = control.size() * sizeof(double);
	if (str.size() != len) {
		dprintf("Input string not right length (got %d bytes, should be %d bytes)\n",
			str.size(), len);
		return;
	}
	int i = 0;
	for (auto offset = control.begin(); offset != control.end(); offset++) {
		double value = *(double*)(str.data() + i * sizeof(double));
		char data[sizeof(double)];
		if (offset->type == 0) {u_char *ptr = (u_char*)data; *ptr = value;}
		else if (offset->type == 1) {u_short *ptr = (u_short*)data; *ptr = value;}
		else if (offset->type == 2) {u_int *ptr = (u_int*)data; *ptr = value;}
		else if (offset->type == 3) {u_long *ptr = (u_long*)data; *ptr = value;}
		else if (offset->type == 4) {char *ptr = (char*)data; *ptr = value;}
		else if (offset->type == 5) {short *ptr = (short*)data; *ptr = value;}
		else if (offset->type == 6) {int *ptr = (int*)data; *ptr = value;}
		else if (offset->type == 7) {long *ptr = (long*)data; *ptr = value;}
		else if (offset->type == 8) {float *ptr = (float*)data; *ptr = value;}
		else if (offset->type == 9) {double *ptr = (double*)data; *ptr = value;}
		FSUIPC_Write(offset->location, offset->size, data, &FSUIPCResult);
		if (!FSUIPC_Process(&FSUIPCResult)) {
			Close();
			return;
		}
		i++;
	}
}

bool Sim::isConnected() {
	return connected;
}