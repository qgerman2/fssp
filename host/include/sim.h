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

class FSSP;
class Sim {
	private:
		FSSP *fssp;
		DWORD FSUIPCResult;
		bool connected;
		std::vector<Offset> monitor;
		std::vector<Offset> control;
		bool Open();
		void Close();
		bool Poll();
		void PrintValues();
		void SendValues();
		bool ParseOffsets(std::string str, std::vector<Offset> *dest);
	public:
		Sim(FSSP *fssp);
		void Loop();
		void Monitor(std::string str);
		void Control(std::string str);
		void Input(std::string str);
};
#endif