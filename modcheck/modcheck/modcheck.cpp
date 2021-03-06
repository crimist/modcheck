#include <iostream>
#include <string>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <mutex>
#include <cassert>
#include <ctype.h>

#include <Windows.h>
#include <Psapi.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
#include <intrin.h>

// whether to check multiple levels of stack - unnecessary for demo
#undef MODCHECK_MULTILEVEL

// possible improvements:
//	walk mem maps and check if parent has execute (could be hooked)

HMODULE getMainModule() {
	auto proc = GetCurrentProcess();
	wchar_t mainModule[MAX_PATH];
	auto status = GetProcessImageFileName(proc, (wchar_t*)&mainModule, MAX_PATH);
	assert(status);
	std::wstring mainModName = mainModule;
	auto pos = mainModName.find_last_of(L"\\");
	mainModName = mainModName.substr(pos + 1);

	std::wcout << "module " << mainModName << "\n";

	HMODULE modules[1024];
	DWORD needed;
	auto statusB = EnumProcessModules(proc, modules, sizeof(modules), &needed);
	assert(statusB);

	for (auto i = 0; i < (needed / sizeof(HMODULE)); i++) {
		wchar_t modName[MAX_PATH];
		status = GetModuleFileNameEx(proc, modules[i], modName, sizeof(modName) / sizeof(wchar_t));
		assert(status);

		std::wstring search = modName;
		std::wcout << "self: " << mainModName << " other: " << search << "\n";
		if (search.find(mainModName) != std::string::npos) {
			std::wcout << "match: " << mainModule << " -> " << modName << " @ " << modules[i] << "\n";
			return modules[i];
		}
	}

	return NULL;
}

// gets the main module by assuming that the first module will always be the main one
// not sure if guaranteed to work on all versions of win
HMODULE getMainModuleFast() {
	auto proc = GetCurrentProcess();
	HMODULE mainMod = NULL;
	HMODULE modules[1024];
	DWORD needed;
	auto statusB = EnumProcessModules(proc, modules, sizeof(modules), &needed);
	assert(statusB);

	return modules[0];
}

void checkCaller() {
	// would make static for real use so that injection would have to be on startup
	auto mainMod = getMainModuleFast();
	assert(mainMod != NULL);

	auto proc = GetCurrentProcess();
	MODULEINFO modInfo;
	auto statusB = GetModuleInformation(proc, mainMod, &modInfo, sizeof(modInfo));
	assert(statusB);
	CloseHandle(proc);

	uintptr_t lower = (uintptr_t)modInfo.lpBaseOfDll, upper = (uintptr_t)modInfo.lpBaseOfDll + modInfo.SizeOfImage;
	std::cout << "lower\t" << lower << "\n";
	std::cout << "upper\t" << upper << "\n";

	/* 
	// todo?
	IMAGE_NT_HEADERS *peHdr = ImageNtHeader(modInfo.lpBaseOfDll);
	CONTEXT ctx = {0};
	ctx.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&ctx);
	assert(statusB);
	STACKFRAME frame = {0};
	frame.AddrPC.Offset = ctx.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = ctx.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = ctx.Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;

	auto thread = GetCurrentThread();
	assert(thread);
	std::vector<uintptr_t> callers;

	while (true) {
		statusB = StackWalk(peHdr->FileHeader.Machine, proc, thread, &frame, &ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);
		callers.push_back(); // 2do
		if (statusB != FALSE) {
			break;
		}
	}
	CloseHandle(thread);
	CloseHandle(proc);

	for (auto const& addr : callers) {
		std::cout << "addr\t" << std::setw(16) << addr << "\n";
		assert(addr < upper && addr > lower);
	}
	*/

	// get single ret addr using asm
	auto addr = (uintptr_t*)_AddressOfReturnAddress(); // _ReturnAddress() for direct val
	std::cout << "asm\t" << *addr << "\n";
	assert(*addr <= upper && *addr >= lower);

	#ifdef MODCHECK_MULTILEVEL
	// use RtlCaptureStackBackTrace to get full backtrace - could easily be hooked
	const auto s = 6; // if > 6 we'll catch `Kernel32!BaseThreadInitThunk` from startup
	void* callers[s];
	auto captured = RtlCaptureStackBackTrace(0, s, callers, NULL);
	assert(captured);
	std::cout << "captured: " << captured << "\n";

	assert(*addr == (uintptr_t)callers[0]); // we can detect spoof for first level
	for (auto i = 0; i < captured; i++) {
		auto caller = (uintptr_t)callers[i];
		std::cout << "> " << caller << "\n";

		assert(caller <= upper && caller >= lower);
	}
	#endif

	return;
}

int main() {
	std::cout << std::uppercase << std::hex << std::setfill('0');
	std::cout << "func\t" << (uintptr_t)checkCaller << "\n\n";

	checkCaller();

	std::condition_variable cv; std::mutex m; std::unique_lock<std::mutex> lock(m);
	cv.wait(lock, [] {return false; });

	return 0;
}
