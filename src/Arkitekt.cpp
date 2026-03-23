
#define ZHL_LOGGING 1
#include "SigScan.h"
#include <unordered_map>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <io.h>
#include "PALMemoryProtection.h"
#include "Logging.h"
#include <filesystem>

#ifdef WIN32
#include <Windows.h>
#endif
#ifdef IM_GUI_INCL
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#endif
#include "Arkitekt.h"
namespace Organik
{
};
using namespace Arkitekt;
using namespace Organik;
size_t murmurhash(const char *Key)
{
	// https://github.com/jwerle/murmurhash.c - Licensed under MIT
	size_t len = strlen(Key);
	uint32_t c1 = 0xcc9e2d51;
	uint32_t c2 = 0x1b873593;
	uint32_t r1 = 15;
	uint32_t r2 = 13;
	uint32_t m = 5;
	uint32_t n = 0xe6546b64;
	uint32_t h = 0;
	uint32_t k = 0;
	uint8_t *d = (uint8_t *)Key; // 32 bit extract from 'key'
	const uint32_t *chunks = NULL;
	const uint8_t *tail = NULL; // tail - last 8 bytes
	int i = 0;
	int l = len / 4; // chunk length

	chunks = (const uint32_t *)(d + l * 4); // body
	tail = (const uint8_t *)(d + l * 4);	// last 8 byte chunk of `key'

	// for each 4 byte chunk of `key'
	for (i = -l; i != 0; ++i)
	{
		// next 4 byte chunk of `key'
		k = chunks[i];

		// encode next 4 byte chunk of `key'
		k *= c1;
		k = (k << r1) | (k >> (32 - r1));
		k *= c2;

		// append to hash
		h ^= k;
		h = (h << r2) | (h >> (32 - r2));
		h = h * m + n;
	}

	k = 0;

	// remainder
	switch (len & 3)
	{ // `len % 4'
	case 3:
		k ^= (tail[2] << 16);
	case 2:
		k ^= (tail[1] << 8);

	case 1:
		k ^= tail[0];
		k *= c1;
		k = (k << r1) | (k >> (32 - r1));
		k *= c2;
		h ^= k;
	}

	h ^= len;

	h ^= (h >> 16);
	h *= 0x85ebca6b;
	h ^= (h >> 13);
	h *= 0xc2b2ae35;
	h ^= (h >> 16);

	return h;
}
std::vector<FunctionHook *> &GetInstalledHooks()
{
	static std::vector<FunctionHook *> ret{};
	return ret;
};
std::shared_mutex &GetInstallMapMutex()
{
	static std::shared_mutex ret;
	return ret;
};

std::multimap<int, std::function<auto()->FunctionHook *> *, std::greater<int>> &GetHookMap()
{
	static std::multimap<int, std::function<auto()->FunctionHook *> *, std::greater<int>> ret{};
	return ret;
};
std::multimap<int, std::function<auto()->FunctionHook *> *, std::greater<int>> &GetDelayedInstallMap()
{
	static std::multimap<int, std::function<auto()->FunctionHook *> *, std::greater<int>> ret{};
	return ret;
};
static std::thread *installerThread;
std::thread *GetInstallerThread()
{
	if (installerThread)
		return installerThread;
	installerThread = new std::thread([&]()
									  {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
		while (true)
		{
			bool needToInstall = false;

			{ // ?? Scoping for lock destruction
				std::shared_lock<std::shared_mutex> mapReadLock(GetInstallMapMutex());
				needToInstall = GetHookMap().size() > 0;
			}

			if (needToInstall)
			{
				std::unique_lock<std::shared_mutex> mapInstallLock(GetInstallMapMutex());
				for (auto [priority, installfn] : GetHookMap())
				{
					GetInstalledHooks().emplace_back((*installfn)());
				}
				GetHookMap().clear();
			}
#ifdef IM_GUI_INCL
			needToInstall = false;

			if (ImGui::GetCurrentContext())
			{
				{ // ?? Scoping for lock destruction again (it's just way easier to do this than try to swap a shared lock to a unique one)
					std::shared_lock<std::shared_mutex> mapReadLock(GetInstallMapMutex());
					needToInstall = GetDelayedInstallMap().size() > 0;
				}
				if (needToInstall)
				{
					std::unique_lock<std::shared_mutex> mapInstallLock(GetInstallMapMutex());
					for (auto [priority, installfn] : GetDelayedInstallMap())
					{
						GetInstalledHooks().emplace_back((*installfn)());
					}
					GetDelayedInstallMap().clear();
				}
			}
#endif
			std::this_thread::sleep_for(100ms);
		} });
	installerThread->detach();
	return installerThread;
};

bool Arkitekt::Init()
{
	static bool initialized = false;
	if (initialized)
		return initialized;
	initialized = true;

	if (!Definition::Init())
	{
		// TODO: Switch to SDL2, probably. Do it when ImGui support goes into Arkitekt main branch b/c for linux SDL2 will already be required

		MessageBoxA(0, "Error during sigscanning.", "Error", MB_ICONERROR);
		ExitProcess(1);
	}

	Arkitekt::GetLogger()->InitLogging();
	GetInstallerThread();

	return initialized;
}

//================================================================================
// Definition

static std::vector<Definition *> &Defs()
{
	static std::vector<Definition *> defs;
	return defs;
}

std::unordered_map<size_t, Definition *> &DefsByHash()
{
	static std::unordered_map<size_t, Definition *> DefsByHash;
	return DefsByHash;
}

int Definition::Init()
{
	SigScan::Init();

	for (auto it = Defs().begin(); it != Defs().end(); ++it)
	{
		if (!(*it)->Load())
			return 0;
	}
	return 1;
}

void Definition::Add(const char *name, size_t typeHash, Definition *def)
{
	Defs().push_back(def);
	DefsByHash().insert(std::pair<size_t, Definition *>(murmurhash(name) ^ typeHash, def));
}

//================================================================================
// VariableDefinition

int VariableDefinition::Load()
{
	SigScan sig(_sig);
	if (!sig.Scan())
	{
		std::cerr << "Failed to find value for variable" << _name << "address could not be found" << std::endl;
		return 0;
	}

	if (sig.GetMatchCount() == 0)
	{
		std::cerr << "Failed to find address for variable " << _name << " no capture in input signature" << std::endl;
		return 0;
	}

	const SigScan::Match &m = sig.GetMatch();
	if (_useOffset)
	{
		/*
		 * Instruction Pointer relative addressing
		 * Determine real address of variable match
		 * RIP + var = real addr
		 * During execution (E|R)IP would be at the next instruction so instead we add the length of the match
		 * NOTE: This means it is only possible to match a variable with offset computation that is at the END of the instruction bytes
		 * i.e., operands that can take two memory addresses, only the second memory address (end of the bytes of the instruction) can be matched.
		 */
		if (!m.address || !m.length)
		{
			std::cerr << "VariableDefinition::Load(" << _name << "): m.address is NULL." << std::endl;
			return 0;
		}
		uintptr_t valueVar = 0;
		memcpy(&valueVar, m.address, m.length);
		uintptr_t realAddr = (uintptr_t)m.address;
		realAddr += m.length;
		realAddr += valueVar;
		if (!_outVar)
		{
			std::cerr << "VariableDefinition::Load(" << _name << "): _outVar is NULL." << std::endl;
			return 0;
		}
		if (!(*(void **)_outVar))
		{
			std::cerr << "VariableDefinition::Load(" << _name << "): _outVar resolves to nullptr." << std::endl;
			return 0;
		}
		*(void **)_outVar = (void *)realAddr;
	}
	else if (_useValue)
		memcpy(_outVar, m.address, m.length);
	else
		*(void **)_outVar = (void *)m.address;

	std::cout << "Found value for " << _name << ": @" << _outVar << ", dist " << std::to_string(sig.GetDistance());

	return 1;
}

//================================================================================
// NoOpDefinition

int NoOpDefinition::Load()
{
	SigScan sig(_sig);
	if (!sig.Scan())
	{
		std::cerr << "Failed to find match for no-op region " << _name << " address could not be found" << std::endl;
		return 0;
	}

	if (sig.GetMatchCount() == 0)
	{
		std::cerr << "Failed to find match for no-op region " << _name << " no capture in input signature" << std::endl;
		return 0;
	}

	const SigScan::Match &m = sig.GetMatch();

	uintptr_t ptrToCode = (uintptr_t)m.address;
	const size_t noopingSize = sizeof(uint8_t) * m.length;
	MEMPROT_SAVE_PROT(dwOldProtect);
	MEMPROT_PAGESIZE();
	MEMPROT_UNPROTECT(ptrToCode, noopingSize, dwOldProtect);
	for (int i = 0; i < m.length; i++)
	{
		*(uint8_t *)(ptrToCode++) = 0x90;
	}
	MEMPROT_REPROTECT(ptrToCode, noopingSize, dwOldProtect);

	std::cerr << "Found address for " << _name << ": " << std::to_string((uintptr_t)m.address) << ", wrote NOP's for " << std::to_string(m.length) << "bytes, dist " << std::to_string(sig.GetDistance()) << std::endl;
	_address = (void *)m.address;
	return 1;
}

//================================================================================
// FunctionDefinition

int FunctionDefinition::Load()
{
	static uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
	if (_address)
	{
		std::cout << "Function " << _name << " has predefined address: " << std::hex << _address << std::dec << std::endl;
		*_outFunc = _address;
		return 1;
	}
	SigScan sig = SigScan(_sig);

	if (!sig.Scan())
	{
		std::cerr << "Failed to find address for function " << _name << std::endl;
		return 0;
	}

	_address = sig.GetAddress<void *>();

	*_outFunc = _address;
	std::cout << "Found address for " << _name << " @ " << std::hex << _address << " after " << std::dec << sig.GetDistance() << " bytes" << std::endl;

	return 1;
}
