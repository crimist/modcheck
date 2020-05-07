#include "pch.h"

HMODULE modSelf;
typedef void function_t();
extern "C" void SpoofCall(uintptr_t func, uintptr_t gadget, uintptr_t realret);

// don't inline so that we can get our real ret addr
__declspec(noinline) void SpoofCallWrapper(uintptr_t func, uintptr_t gadget) {
	SpoofCall(func, gadget, (uintptr_t)_ReturnAddress());
	return;
}

HMODULE getMainModuleFast() {
	auto proc = GetCurrentProcess();
	HMODULE mainMod = NULL;
	HMODULE modules[1024];
	DWORD needed;
	auto statusB = EnumProcessModules(proc, modules, sizeof(modules), &needed);
	assert(statusB);

	return modules[0];
}

uintptr_t findRetGadget() {
	// could loop through mem and use `VirtualQueryEx()` and find x bit but we know that
	// our main mod is xecute and will have a ret so why not just search that ez

	auto proc = GetCurrentProcess();
	auto modMain = getMainModuleFast();
	assert(modMain != NULL);
	MODULEINFO modInfo;
	auto statusB = GetModuleInformation(proc, modMain, &modInfo, sizeof(modInfo));
	assert(statusB);
	CloseHandle(proc);

	uintptr_t addr = (uintptr_t)modInfo.lpBaseOfDll;
	while (addr < (uintptr_t)modInfo.lpBaseOfDll + modInfo.SizeOfImage) {
		// https://c9x.me/x86/html/file_module_x86_id_280.html
		// match ret instruction
		if (*(uint8_t*)addr == 0xC3) {
			return addr;
		}
		addr++;
	}

	return 0;
}

void main() {
	std::cout << "main (dll) " << std::hex << std::setfill('0') << main << "\n";

	auto proc = GetCurrentProcess();
	auto modMain = getMainModuleFast();
	assert(modMain != NULL);
	MODULEINFO modInfo;
	auto statusB = GetModuleInformation(proc, modMain, &modInfo, sizeof(modInfo));
	assert(statusB);
	CloseHandle(proc);

	uintptr_t addr = 0;
	auto gadget = findRetGadget();
	assert(gadget);
	std::cout << "got ret gadget @ " << gadget << "\n";

	std::cout << "faddr> ";
	std::cin >> std::hex >> addr;
	if (addr == 0) {
		std::cout << "no addr, exiting\n";
		goto cleanup;
	}

	std::cout << "calling " << addr << "\n";
	SpoofCallWrapper(addr, gadget);
	// stack is now fucked - don't try access any local vars
	// modSelf works cause it's global but if we hit this functions return we would crash
	// todo: try to fix stack by subtracting rsp?

	cleanup:
	FreeLibraryAndExitThread(modSelf, 0);
	return;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	modSelf = hModule;

	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)main, NULL, NULL, NULL);
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}
