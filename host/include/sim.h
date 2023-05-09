#ifndef SIM
#define SIM
#include <map>
#include <string>
#include <windows.h>
class Sim {
	private:
		DWORD dwResult;
		std::map<int, std::string*> offsets;
	public:
		bool connected;
		Sim();
		bool open();
		void close();
		void monitor(int offset, int size);
		void poll();
		void printOffsets();
};
#endif