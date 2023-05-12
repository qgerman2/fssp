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

class Sim {
	private:
		DWORD FSUIPCResult;
		bool connected;
		std::vector<Offset> offsets;
		bool Open();
		void Close();
		bool Poll();
		void PrintValues();
	public:
		Sim();
		void Loop();
		bool Monitor(std::string str);
};
#endif