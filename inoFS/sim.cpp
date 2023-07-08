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
		for (auto c = clients.begin(); c != clients.end(); ++c) {
			if (Poll(&c->monitor)) {
				//PrintValues();
				SendValues(0, &(*c), &c->monitor);
			}
		}
	}
}


bool Sim::isConnected() {
	return connected;
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
	dprintf("Disconnected from flight sim %s\n", FSIUPCErrors[FSUIPCResult]);
	FSUIPC_Close();
}

void Sim::PrintValues() {
	auto clients = inofs->server->GetClients();
	for (auto c = clients.begin(); c != clients.end(); ++c) {
		for (auto offset = c->monitor.begin(); offset != c->monitor.end(); offset++) {
			dprintf("location: %x, type: %d, value: %f\n", offset->location, offset->type, offset->value);
		}
	}
}

bool Sim::ParseOffsets(std::string str, std::vector<Offset> *dest) {
	dest->clear();
	int s = 3;
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
					ZeroMemory(&offset.value, 8);
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

bool Sim::Monitor(std::string str, Client *client) {
	std::vector<Offset> *monitor = &client->monitor;
	if (ParseOffsets(str, monitor)) {
		dprintf("Client %d set up %d monitor variables\n", client->id, monitor->size());
		return true;
	}
	return false;
}

bool Sim::Control(std::string str, Client *client) {
	std::vector<Offset> *control = &client->control;
	if (ParseOffsets(str, control)) {
		dprintf("Client %d set up %d control variables\n", client->id, control->size());
		return true;
	}
	return false;
}

bool Sim::Read(std::string str, Client *client) {
	char header = str.at(3);
	if (header > 48) {
		header -= 48;
	}
	std::vector<Offset> offsets;
	if (ParseOffsets(str, &offsets)) {
		dprintf("Client %d requested %d variables with header %d\n",
			client->id, offsets.size(), header);
		if (Poll(&offsets)) {
			SendValues(header, client, &offsets);
			return true;
		}
	}
	return false;
}

bool Sim::Write(std::string str, Client *client) {
	int del = str.find_last_of(";", std::string::npos);
	std::vector<Offset> offsets;
	if (ParseOffsets(str.substr(0, del+1), &offsets)) {
		if (Input(str.substr(del + 1), offsets, client->double_precision)) {
			dprintf("Client %d wrote %d variables\n",
				client->id, offsets.size());
			return true;
		}
	}
	return false;
}

bool Sim::Poll(std::vector<Offset> *offsets) {
	if (offsets->size() == 0) {return false;}
	for (auto offset = offsets->begin(); offset != offsets->end(); offset++) {
		char data[8];
		FSUIPC_Read(offset->location, offset->size, &data, &FSUIPCResult);
		if (!FSUIPC_Process(&FSUIPCResult)) {
			Close();
			return false;
		}
		ZeroMemory(&offset->value, 8);
		strncpy(offset->value, data, 8);
	}
	return true;
}

bool Sim::Input(std::string str, std::vector<Offset> control, bool double_precision) {
	if (control.size() == 0) {return false;}
	int value_size = sizeof(double);
	if (!double_precision) {
		value_size = sizeof(float);
	}
	const int len = control.size() * value_size;
	if (str.size() != len) {
		dprintf("Input string not right length (got %d bytes, should be %d bytes)\n",
			str.size(), len);
		return false;
	}
	int i = 0;
	for (auto offset = control.begin(); offset != control.end(); offset++) {
		// Figure out how to make this better
		// I'm sorry
		if (double_precision) {
			double value = *(double*)(str.data() + i * value_size);
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
				return false;
			}
			i++;
		} else {
			float value = *(float*)(str.data() + i * value_size);
			char data[sizeof(float)];
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
				return false;
			}
			i++;
		}
	}
	return true;
}

void Sim::SendValues(char header, Client *client, std::vector<Offset> *offsets) {
	std::vector<double> values;
	values.push_back(header);
	for (auto offset = offsets->begin(); offset != offsets->end(); offset++) {
		double value = 0;
		char data[sizeof(double)];
		strncpy(data, offset->value, sizeof(double));
		if (offset->type == 0) {value = *(u_char*)&data;}
		else if (offset->type == 1) {value = *(u_short*)&data;}
		else if (offset->type == 2) {value = *(u_int*)&data;}
		else if (offset->type == 3) {value = *(u_long*)&data;}
		else if (offset->type == 4) {value = *(char*)&data;}
		else if (offset->type == 5) {value = *(short*)&data;}
		else if (offset->type == 6) {value = *(int*)&data;}
		else if (offset->type == 7) {value = *(long*)&data;}
		else if (offset->type == 8) {value = *(float*)&data;}
		else if (offset->type == 9) {value = *(double*)&data;}
		values.push_back(value);
	}
	if (client->double_precision) {
		this->inofs->server->Broadcast(client->id, (char*)values.data(), values.size() * sizeof(double));
	} else {
		// Figure out how to shut the compiler up about double -> float data loss
		std::vector<float> fvalues(values.begin(), values.end());
		this->inofs->server->Broadcast(client->id, (char*)fvalues.data(), fvalues.size() * sizeof(float));
	}
}