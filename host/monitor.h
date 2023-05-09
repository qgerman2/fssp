#ifndef MONITOR
#define MONITOR
#include <Windows.h>
#include <map>
#include <string>

void addMonitor(int offset, int size);
void queryMonitor(DWORD* dwResult);
void printMonitor();

#endif