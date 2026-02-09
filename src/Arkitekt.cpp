#include <Arkitekt/Shared.hpp>
#include <Arkitekt/Compiler.h>
#include <Arkitekt.h>
#include "detours.h"
#include <shared_mutex>

const std::string_view& FunctionHook::GetName( ) const noexcept
{
	return name;
}
HMODULE hModule_Dupe;
void* FunctionHook::GetAddress( ) const noexcept
{
	return def->GetAddress( );
}

void FunctionHook::Install( )
{
	_detour = new MologieDetours::Detour<void*>(def->GetAddress( ), def->GetHook(_hook));
	*_outInternalSuper = _detour->GetOriginalFunction( );
}

FunctionHook::~FunctionHook( )
{
	if (_detour)
		delete _detour;
};
using namespace Arkitekt;
_NODISCARD_LOCK std::pair<std::unique_lock<std::shared_mutex>&&, std::ofstream&> infoLog(const std::string_view& name, std::ios_base::openmode mode)
{
	static FILE* f = fopen(name.data( ), "w");
	static std::ofstream _stream = std::ofstream(f);
	static std::shared_mutex infoMutex;

	return std::pair<std::unique_lock<std::shared_mutex>&&, std::ofstream&>(
		std::move(std::unique_lock(infoMutex)), std::forward<std::ofstream&>(_stream)
	);
}

_NODISCARD_LOCK std::pair<std::unique_lock<std::shared_mutex>&&, std::ofstream&> errorLog(const std::string_view& name, std::ios_base::openmode mode)
{
	static FILE* f = fopen(name.data( ), "w");
	static std::ofstream _stream = std::ofstream(f);
	static std::shared_mutex errorMutex;

	return std::pair<std::unique_lock<std::shared_mutex>&&, std::ofstream&>(
		std::move(std::unique_lock(errorMutex)), std::forward<std::ofstream&>(_stream)
	);
}

std::shared_mutex& ArkitektHookMutex( )
{
	static std::shared_mutex m;
	return m;
}

void Arkitekt::Init( )
{
	for (auto& [key, value] : FunctionHook::GetHookMap( ))
	{
		GetLogger( )->LogFormatted("Registering hook %s address %p hook %p", value->GetName( ), value->def->GetAddress( ), value->_hook);
		value->Install( );
	}
}
