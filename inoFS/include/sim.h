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

class inoFS;
class Sim {
	private:
		inoFS *inofs;
		DWORD FSUIPCResult;
		bool connected;
		bool Open();
		void Close();
		bool Poll();
		void PrintValues();
		void SendValues();
		bool ParseOffsets(std::string str, std::vector<Offset> *dest);
	public:
		Sim(inoFS *inofs);
		void Loop();
		bool Monitor(std::string str, std::vector<Offset> *monitor);
		bool Control(std::string str, std::vector<Offset> *control);
		void Input(std::string str, std::vector<Offset> control);
};
#endif