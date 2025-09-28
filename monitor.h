#pragma once
#include <vector>
extern std::vector<std::thread> g_monitorThreads;
extern std::atomic<bool> g_shouldExit;
extern std::atomic<bool> g_isRunning;

int main();

extern "C" __declspec(dllexport) const void exitMonitor();
