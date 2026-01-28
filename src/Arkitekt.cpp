// #include <dynasm/dasm_proto.h>
// #include <dynasm/dasm_x86.h>
#include <fstream>
#include <Windows.h>
#include <shared_mutex>
#include <deque>
#include <Arkitekt/Shared.hpp>
#include <Arkitekt/Compiler.h>
#include <Arkitekt.h>
#include "SigScan.h"
#include "detours.h"
#include <string>
#include <thread>
#ifndef __i386__
#define __i386__ 1
#endif
#ifdef __amd64__
#undef __amd64__
#endif

const std::string_view& FunctionHook::GetName() const noexcept {
	return name;
}
HMODULE hModule_Dupe;
void * FunctionHook::GetAddress() const noexcept {
	return def->GetAddress();
}

void FunctionHook::Install ()
{
	_detour = new MologieDetours::Detour<void*>(def->GetAddress(), def->GetHook(_hook));
	*_outInternalSuper = _detour->GetOriginalFunction();
}

FunctionHook::~FunctionHook()
{
	if (_detour)
		delete _detour;
};
using namespace Arkitekt;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		if (hModule_Dupe == nullptr)
		{
            GetModuleHandleEx(NULL, L"LibArkitekt.dll", &hModule_Dupe);
            std::thread(Arkitekt::Init).detach();
        }
    }
    return true;
}
static std::ofstream outCsv("out.csv", std::ios_base::out);
static std::ofstream infoLog("info.log", std::ios_base::out);
static std::ofstream errorLog("error.log", std::ios_base::out);

inline static std::shared_mutex& ArkitektHookMutex() {
	static std::shared_mutex m;
	return m;
}

void Arkitekt::Init()
{
	for (auto &[key,value] : FunctionHook::GetHookMap())
	{
		GetLogger()->LogFormatted("Registering hook %s address %p hook %p",value->GetName(),value->def->GetAddress(), value->_hook);
		value->Install();
	}
/* 	
    Arkitekt::InitSigList();
    static auto _sigList = GetSigList();
    if (_sigList.size() <= 0)
    {
        errorLog << "SigList 0 size" << std::endl;
        exit(1);
    }
// 
    static int32_t listEntry = 0;
    static void* baseAddress = nullptr;
    static std::deque<std::int32_t> removeIndexes;
    // 
    static int32_t iterationCount = 0;
    while (iterationCount < 10 && _sigList.size())
    {
        listEntry = 0;
        for (const auto& sig : _sigList)
        {
        // 
            SigScan s(std::get<0>(sig).c_str());
            s.Init();
            s.Scan([](SigScan* scanner) -> auto {
                baseAddress = baseAddress ? baseAddress : scanner->GetBaseAddress();
                int32_t count = scanner->GetMatchCount();
                // 
                if (count == 1)
                {
                    auto match = scanner->GetMatch(0);
                    removeIndexes.emplace_back(listEntry);
// 
                    outCsv << std::hex 
                            << "0x" << scanner->GetDistance() 
                            << "|0x" << (void*) match.address
                            << "|" << std::dec << match.length 
                            << "|" << std::get<1>(_sigList[listEntry])
                            << "|" << std::get<2>(_sigList[listEntry])
                            << std::endl;
                    // 
// 
                    
                    SigScan::AddFunction((void*)match.address, match.length);
                    // 
                    infoLog << "Found function " << std::get<2>(_sigList[listEntry]) << std::endl;
                    return;
                }
                infoLog << "No (unique) match for " << std::get<2>(_sigList[listEntry]) << ", matches found: " << count << std::endl;
            });
            listEntry++;
        }
        exit(0);
    } */
}
