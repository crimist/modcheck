#include "pch.h"

void main(void* modArg) {
	auto module = *static_cast<HMODULE*>(modArg);
	delete modArg;

	typedef void function_t();
	intptr_t ptr = 0;
	std::cout << std::hex << std::setw(16) << std::setfill('0') << main << "\n";
	std::cout << "faddr> ";
	std::cin >> std::hex >> ptr;
	function_t* function = (function_t*)ptr;
	function();
	std::cout << "called\t" << std::setw(16) << ptr << "\n";
	FreeLibraryAndExitThread(module, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	auto mod = new HMODULE;
	*mod = hModule;

	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)main, mod, NULL, NULL);
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}
