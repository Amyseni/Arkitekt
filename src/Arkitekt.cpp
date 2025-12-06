#include <Arkitekt/Shared.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <filesystem>
#include <string>
#include "detours.h"
#include <Arkitekt/Logging.h>
#include <Windows.h>
#include <Arkitekt.h>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include "PALMemoryProtection.h"

using namespace Arkitekt;
HINSTANCE hModule_Dupe = nullptr;
#define OUR_OWN_FUNCTIONS_CALLEE_DOES_CLEANUP 1

static std::ofstream outCsv("out.csv", std::ios_base::out);
static std::ofstream infoLog("info.log", std::ios_base::out);
static std::ofstream errorLog("error.log", std::ios_base::out);
static int32_t threadsRunning;

inline static std::shared_mutex& ArkitektHookMutex() {
    static std::shared_mutex m;
    return m;
}

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

void Arkitekt::Init()
{
    for (auto &[key,value] : FunctionHook::GetHookMap())
    {
        value->Install();
    }
}

//
/* The following (until EOF) is from LibZHL by Kilburn (@FixItVinh) */
/* It is licensed under MIT 2.0 */
/* More details available in LICENSE.md */
//

#define P(x) *(ptr++) = (x)
#define PS(x) *(((unsigned short*&)ptr)++) = (x)
#define PL(x) *(((unsigned int*&)ptr)++) = (x)


FunctionHook::~FunctionHook()
{
    std::unique_lock<std::shared_mutex> lock(ArkitektHookMutex());
	if(_detour)
		delete (MologieDetours::Detour<void*>*)_detour;
}

int FunctionHook::Install()
{
	if(!def)
	{
		std::cerr << "Failed to install hook for " << GetName() << ": Function not found."<< std::endl;
		return 0;
	}

	struct ArgData
	{
		char r;
		char s;
	};

	const ArgData *argd = (const ArgData*)def->GetArgData();
	int argc = def->GetArgCount();
#ifdef __i386__
	unsigned char *ptr;
	int stackPos;
#ifdef USE_STACK_ALIGNMENT
	unsigned int stackAlignPosition;
	unsigned int stackAlignOffset;
	unsigned int registersUsedSize;
#endif // USE_STACK_ALIGNMENT
	int k;
    std::unique_lock<std::shared_mutex> lock(ArkitektHookMutex());
    MEMPROT_SAVE_PROT(oldProtect);
	MEMPROT_PAGESIZE();
#endif // __i386__

	//==================================================
	// Internal hook
	// Converts userpurge to thiscall to call the user
	// defined hook

#ifdef __i386__
	ptr = reinterpret_cast<unsigned char*> (_internalHook);

	// Prologue
	P(0x55);					// push ebp
	P(0x89); P(0xe5);			// mov ebp, esp

	// Not sure yet if this is different on 64-bit, I think it is because of push EBP + CALL so maybe?
	// Compute stack size
	#ifdef USE_STACK_ALIGNMENT
	registersUsedSize = 0;
	#endif // USE_STACK_ALIGNMENT
	stackPos = 8;
	for(int i=0 ; i<argc ; ++i)
	{
		if(argd[i].r < 0)
			stackPos += 4 * argd[i].s;
        #ifdef USE_STACK_ALIGNMENT
        else
            registersUsedSize += 4; // Must also include arguments that come in on registers that we push to the stack for proper alignment computation
        #endif // USE_STACK_ALIGNMENT
	}

	#ifdef USE_STACK_ALIGNMENT
        stackAlignPosition = stackPos + registersUsedSize;
        /* We need to account for everything pushed onto the stack so we can compute the proper 16-byte alignment per System V ABI specification */
        if(def->IsVoid() || !def->IsLongLong())
            stackAlignPosition += 4;
        if(def->IsVoid())
            stackAlignPosition += 4;
        stackAlignPosition += 4 * 4; // Because of the regular ECX/EBX/ESI/EDI registers we always push (or their R equivalents for 64-bit),

        // Modulo to get amount of padding we need to add to ensure the correct alignment
        stackAlignOffset = (STACK_ALIGNMENT_SIZE - (stackAlignPosition % STACK_ALIGNMENT_SIZE)) % STACK_ALIGNMENT_SIZE;
        
        // Reserve some extra space to ensure proper stack alignment
        if(stackAlignOffset != 0) {
            P(0x83); P(0xec); P(stackAlignOffset); // sub esp, N8
        }
	#endif // USE_STACK_ALIGNMENT

	// Push general purpose registers
	if(def->IsVoid() || !def->IsLongLong())
		P(0x52);	// push edx
	if(def->IsVoid())
		P(0x50);	// push eax
	P(0x51);	// push ecx
	P(0x53);	// push ebx
	P(0x56);	// push esi
	P(0x57);	// push edi

	// Copy arguments to their appropriate location
	int sizePushed = 0;
	k = stackPos;
	for(int i=argc-1 ; i>=0 ; --i)
	{
		if(def->IsThiscall() && i == 0)
		{
			// special case: this (shouldn't be pushed to the stack but copied to ecx)
			if(argd[i].r >= 0)
			{
				if(argd[i].r != 1) // no need to do mov ecx, ecx
				{
					P(0x89); P(0xc1 | ((argd[i].r & 7) << 3));	// mov ecx, XXX
				}
			}
			else
			{
				k -= 4;
				P(0x8b); P(0x4d); P(k);							// mov ecx, [ebp+8+4*X]
			}
		}
		else
		{
			if(argd[i].r >= 0)
			{
				P(0x50 + argd[i].r);							// push XXX
				sizePushed += 4;
			}
			else
			{
				for(int j=0 ; j<argd[i].s ; ++j)
				{
					k -= 4;
					P(0xff); P(0x75); P(k);						// push [ebp+8+4*X]
					sizePushed += 4;
				}
			}
		}
	}

	if(def->isMemPassedStructPointer())
        sizePushed -= 4;


	// Call the hook
	P(0xE8); PL((uintptr_t)_hook - (uintptr_t)ptr - 4);	// call _hook

#if OUR_OWN_FUNCTIONS_CALLEE_DOES_CLEANUP == 0
    if(sizePushed != 0)
    {
        // If our hook function requires caller cleanup, increment the stack pointer here. This will only be true on non-Windows platforms where our hook will be generated as __cdecl
        if(sizePushed < 128)	{ P(0x83); P(0xc4); P(sizePushed); }	// add esp, N8
        else				{ P(0x81); P(0xc4); PL(sizePushed); }	// add esp, N32
    }
#endif // OUR_OWN_FUNCTIONS_CALLEE_DOES_CLEANUP

	// Restore all saved registers
	P(0x5f);	// pop edi
	P(0x5e);	// pop esi
	P(0x5b);	// pop ebx
	P(0x59);	// pop ecx
	if(def->IsVoid())
		P(0x58);	// pop eax
	if(def->IsVoid() || !def->IsLongLong())
		P(0x5a);	// pop edx

	// Epilogue
	P(0x89); P(0xec);				// mov esp, ebp
	P(0x5d);					// pop ebp
	if(stackPos > 8 && !def->NeedsCallerCleanup())
	{
		P(0xc2); PS(stackPos - 8);	// ret 4*N
	}
	else if(def->isMemPassedStructPointer()) // TODO: Might need to limit this only to Sys V i386 ABI and not Windows also?
	{
        P(0xc2); PS(4);
	}
	else
		P(0xc3);					// ret

	_hSize = ptr - (unsigned char*) _internalHook;
	MEMPROT_UNPROTECT(_internalHook, _hSize, oldProtect);
#endif // __i386__

	// Install the hook with MologieDetours
	try
	{
		
        if(def->forceDetourSize()) // This flag should be used with caution as it'll allow it to skip past RET when writing the detour, if used on a function that was larger it'll create garbage code.
            _detour = new MologieDetours::Detour<void*>(def->GetAddress(), _internalHook, MOLOGIE_DETOURS_DETOUR_SIZE);
        else
            _detour = new MologieDetours::Detour<void*>(def->GetAddress(), _internalHook);
	}
	catch(MologieDetours::DetourException &e)
	{
		std::cerr <<"Failed to install hook for" << GetName() << ": " << e.what() << std::endl;
		return 0;
	}
	void *original = ((MologieDetours::Detour<void*>*)_detour)->GetOriginalFunction();

	//==================================================
	// Internal super
	// Converts thiscall to userpurge to call the
	// original function from the user defined hook
#ifdef __i386__
	ptr = reinterpret_cast<unsigned char*>(_internalSuper);

	// Prologue
	P(0x55);					// push ebp
	P(0x89); P(0xe5);			// mov ebp, esp

	// TODO: I think this needs to change for 64-bit
	// Compute stack size
	#ifdef USE_STACK_ALIGNMENT
	registersUsedSize = 0;
	#endif // USE_STACK_ALIGNMENT
	stackPos = 8;
	for(int i=0 ; i<argc ; ++i)
	{
		if(!def->IsThiscall() || i != 0)
			stackPos += 4 * argd[i].s;

        #ifdef USE_STACK_ALIGNMENT
        if(argd[i].r >= 0)
            registersUsedSize += 4;
        #endif // USE_STACK_ALIGNMENT
	}

	#ifdef USE_STACK_ALIGNMENT
        stackAlignPosition = stackPos - registersUsedSize; // Need to ignore arguments currently on the stack that are destined for register storage as they will not be pushed back to the stack.
        /* We need to account for everything pushed onto the stack so we can compute the proper 16-byte alignment per System V ABI specification */
        if(def->IsVoid() || !def->IsLongLong())
            stackAlignPosition += 4;
        if(def->IsVoid())
            stackAlignPosition += 4;
        stackAlignPosition += 4 * 4; // Because of the regular ECX/EBX/ESI/EDI registers we always push (or their R equivalents for 64-bit),

        // Modulo to get amount of padding we need to add to ensure the correct alignment
        stackAlignOffset = (STACK_ALIGNMENT_SIZE - (stackAlignPosition % STACK_ALIGNMENT_SIZE)) % STACK_ALIGNMENT_SIZE;
        
        // Reserve some extra space to ensure proper stack alignment
        if(stackAlignOffset != 0) {
            P(0x83); P(0xec); P(stackAlignOffset); // sub esp, N8
        }
	#endif // USE_STACK_ALIGNMENT

	// Push general purpose registers
	if(def->IsVoid() || !def->IsLongLong())
		P(0x52);	// push edx
	if(def->IsVoid())
		P(0x50);	// push eax
	P(0x51);	// push ecx
	P(0x53);	// push ebx
	P(0x56);	// push esi
	P(0x57);	// push edi

	// Copy arguments to their appropriate location
	// Stack arguments first
    sizePushed = 0;
	k = stackPos;
	for(int i=argc-1 ; i>=0 ; --i)
	{
		if(def->IsThiscall() && i == 0)
		{
			if(argd[i].r < 0)
			{
				// special case: this (shouldn't be taken from the stack but copied from ecx)
				P(0x51);								// push ecx
				sizePushed += 4;
			}
		}
		else
		{
			if(argd[i].r < 0)
			{
				for(int j=0 ; j<argd[i].s ; ++j)
				{
					k -= 4;
					P(0xff); P(0x75); P(k);				// push [ebp+8+4*X]
					sizePushed += 4;
				}
			}
			else
				k -= 4 * argd[i].s;
		}
	}

	// Now register based arguments
	k = 8;
	for(int i=0 ; i<argc ; ++i)
	{
		if(def->IsThiscall() && i == 0)
		{
			// special case: this (shouldn't be taken from the stack but copied from ecx)
			if(argd[i].r >= 0 && argd[i].r != 1) // no need to do mov ecx, ecx
			{
				P(0x89); P(0xc8 | (argd[i].r & 7));		// mov XXX, ecx
			}
		}
		else
		{
			if(argd[i].r >= 0)
			{
				P(0x8b); P(0x45 | ((argd[i].r & 7)<<3));
				P(k);							// mov XXX, [ebp+8+4*X]
			}
			k += 4 * argd[i].s;
		}
	}
	
	if(def->isMemPassedStructPointer())
        sizePushed -= 4;

	// Call the original function
	P(0xE8); PL((uintptr_t)original - (uintptr_t)ptr - 4);	// call original

	// If the function requires caller cleanup, increment the stack pointer here
	if(def->NeedsCallerCleanup() && sizePushed != 0)
	{
		if(sizePushed < 128)	{ P(0x83); P(0xc4); P(sizePushed); }	// add esp, N8
		else				{ P(0x81); P(0xc4); PL(sizePushed); }	// add esp, N32
	}

	// Pop general purpose registers
	P(0x5f);	// pop edi
	P(0x5e);	// pop esi
	P(0x5b);	// pop ebx
	P(0x59);	// pop ecx
	if(def->IsVoid())
		P(0x58);	// pop eax
	if(def->IsVoid() || !def->IsLongLong())
		P(0x5a);	// pop edx

	// Epilogue
	P(0x89); P(0xec);				// mov esp, ebp
	P(0x5d);					// pop ebp

	if(stackPos > 8 && OUR_OWN_FUNCTIONS_CALLEE_DOES_CLEANUP)
	{
		P(0xc2); PS(stackPos - 8);	// ret 4*N
	}
	else if(def->isMemPassedStructPointer()) // TODO: Might need to limit this only to Sys V i386 ABI and not Windows also? (but then again, the OUR_OWN_FUNCTIONS_CALLEE_DOES_CLEANUP thing might already take care of this for Windows)
	{
        P(0xc2); PS(4);
	}
	else
		P(0xc3);					// ret

	_sSize = ptr - (unsigned char*) _internalSuper;
	MEMPROT_UNPROTECT(_internalSuper, _sSize, oldProtect);

	// Set the external reference to internalSuper so it can be used inside the user defined hook

	*_outInternalSuper = _internalSuper;
#endif // __i386__
#ifdef __amd64__
	// Set the external reference to the original function so it can be used inside the user defined hook
	*_outInternalSuper = original;
#endif // __amd64__

#ifdef __amd64__
GetLogger()->LogFormatted("HookAddress: " PTR_PRINT_F ", SuperAddress: " PTR_PRINT_F "\n\n", (uintptr_t) _hook, (uintptr_t) original);
#endif // __amd64__
#ifdef __i386__
#define DEBUG 1
#ifdef DEBUG
	GetLogger()->LogFormatted("Successfully hooked function %s\n",GetName().data());
    GetLogger()->LogFormatted("InternalHookAddress: %p\n", _internalHook);
	GetLogger()->LogFormatted("%s - internalHook:\n", GetName().data());
    
	for(unsigned int i=0 ; i<_hSize ; ++i)
		GetLogger()->LogFormatted("%02x ", static_cast<char>((reinterpret_cast<unsigned char*>(_internalHook))[i]));
    GetLogger()->LogFormatted("\n");
#endif // DEBUG

#ifdef DEBUG
    GetLogger()->LogFormatted("InternalSuperAddress: %p\n", &(reinterpret_cast<unsigned char*>(_internalSuper)[0]));
#endif // DEBUG

	// for(unsigned int i=0 ; i<_sSize ; ++i)
	// 	GetLogger()->LogFormatted("%02x ", _internalSuper[i]);
    // GetLogger()->LogFormatted("\n");
#endif // __i386__


	return 1;
}
