#ifndef SIM
#define SIM
#include <vector>
#include <string>
#include <windows.h>

struct Offset {
	u_int location;
	char type;
	char size;
	double value;
};
struct Client;
class inoFS;
class Sim {
	private:
		inoFS *inofs;
		DWORD FSUIPCResult;
		bool connected;
		bool Open();
		void Close();
		bool Poll(std::vector<Offset> *offsets);
		void PrintValues();
		void SendValues(Client *client, std::vector<Offset> *offsets);
		bool ParseOffsets(std::string str, std::vector<Offset> *dest);
	public:
		Sim(inoFS *inofs);
		void Loop();
		bool Monitor(std::string str, std::vector<Offset> *monitor);
		bool Control(std::string str, std::vector<Offset> *control);
		bool Input(std::string str, std::vector<Offset> control);
		bool Read(std::string str, Client *client);
		bool Write(std::string str, Client *client);
		bool isConnected();
};
#endif