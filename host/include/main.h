#include <windows.h>
#include <vector>
#include "server.h"
int main();
void loop();
void process(std::vector<Packet> packets);
BOOL WINAPI ctrlEvent(DWORD signal);