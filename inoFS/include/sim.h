#ifndef SIM
#define SIM
#include <vector>
#include <string>
#include <windows.h>

struct Offset {
	u_int location;
	char type;
	char size;
	char value[sizeof(double)];
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
		bool ParseOffsets(std::string str, std::vector<Offset> *dest);
	public:
		Sim(inoFS *inofs);
		void Loop();
		bool Monitor(std::string str, Client *client);
		bool Control(std::string str, Client *client);
		bool Input(std::string str, std::vector<Offset> control, bool double_precision);
		bool Read(std::string str, Client *client);
		bool Write(std::string str, Client *client);
		bool isConnected();
		void SendValues(char header, Client *client, std::vector<Offset> *offsets);
};
#endif