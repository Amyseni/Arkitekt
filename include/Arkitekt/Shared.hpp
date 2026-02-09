#pragma once


#ifndef WIN32
#define WIN32 1
#endif
#ifndef _WIN32
#define _WIN32 1
#endif
#if defined(WIN64)
#undef WIN64
#endif
#if defined(_WIN64)
#undef _WIN64
#endif
#if defined(AMD64)
#undef AMD64
#endif
#if defined(_AMD64)
#undef _AMD64
#endif
#ifndef __i386__
#define __i386__ 1
#endif
#ifdef __amd64__
#undef __amd64__
#endif
#ifdef __AMD64__
#undef __AMD64__
#endif
#include "Windows.h"
#include <Arkitekt/Compiler.h>
#include <stdint.h>
#include <stack>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <map>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <shared_mutex>
#include <concepts>
#include <print>
#include <deque>
#include <thread>
struct Action;
namespace Arkitekt
{
    struct FnBlock;
    struct Assembler;
};
using namespace Arkitekt;

static uintptr_t GameModuleBase = [ ] ( ) -> auto
{
    return (uintptr_t) GetModuleHandleA(nullptr);
}();
#define GAME_EXE "SYNTHETIK.exe"
#define FUNC_NAKED __declspec(naked)
#define JUMP_INSTRUCTION(target) jmp [target]

// forward declaring
namespace MologieDetours
{
    template <class FnType> class Detour;
};
namespace Arkitekt
{
    struct Logger
    {
    public:
        ~Logger();
        bool Init(const std::string& filename);
        void Cleanup(void);

        bool LogSimple(const char* text, bool flushLine = true);
        bool LogFormatted(const char* fmt, ...);
        std::string ParseFormatting(const char* fmt, ...);
        std::string ParseFormatting(const char* fmt, va_list args);
        Logger(std::filesystem::path path = std::filesystem::current_path( ), const char* filename = "organik.log");
    private:
        std::ofstream outFile;
        static std::shared_mutex logMutex;
        bool WriteToLog(const std::string& message, bool flushLine = true);
    };
    extern Arkitekt::Logger* GetLogger( );
    extern void Init( );
    extern void Done(bool status);
}
enum tCallFlags : uint32_t
{
    _CDECL = 1u << 0,
    _STDCALL = 1u << 1,
    _THISCALL = 1u << 2,
    _FASTCALL = 1u << 3,
    _RETVOID = 1u << 8,
    _RETLONGLONG = 1u << 9,
};

struct TCallFlags
{
    tCallFlags value;
    constexpr operator uint32_t( ) const noexcept
    {
        return (value);
    }
    constexpr operator tCallFlags( ) const noexcept
    {
        return (value);
    }

    template<uint32_t v>
    consteval TCallFlags( ) : value(static_cast<tCallFlags>(v)) { }

    template<tCallFlags v>
    consteval TCallFlags( ) : value(v) { }

    constexpr TCallFlags(uint32_t v) noexcept : value(static_cast<tCallFlags>(v)) { }
    constexpr TCallFlags(tCallFlags v) noexcept : value(v) { }
    constexpr TCallFlags& operator=(const tCallFlags& v) noexcept
    {
        this->value = v;
        return (*this);
    }
    constexpr TCallFlags& operator=(const uint32_t& v) noexcept
    {
        this->value = static_cast<tCallFlags>(v);
        return (*this);
    }
    constexpr TCallFlags& operator|(this TCallFlags& me, const tCallFlags& other) noexcept
    {
        me.value = static_cast<tCallFlags>(static_cast<uint32_t>(me.value) | static_cast<uint32_t>(other));
        return me;
    }
    constexpr const TCallFlags& operator|(this const TCallFlags& me, const tCallFlags& other) noexcept
    {
        return TCallFlags((static_cast<uint32_t>(me.value) | static_cast<uint32_t>(other)));
    }
    constexpr TCallFlags& operator|=(const tCallFlags& other) noexcept
    {
        this->value = static_cast<tCallFlags>(static_cast<uint32_t>(this->value) | static_cast<uint32_t>(other));
        return (*this);
    }
    constexpr TCallFlags& operator|=(const uint32_t& other) noexcept
    {
        this->value = static_cast<tCallFlags>(static_cast<uint32_t>(this->value) | other);
        return (*this);
    }

};

struct Definition
{
public:
    virtual ~Definition( ) { }
    virtual constexpr const std::string_view& GetName( ) const noexcept = 0;
    virtual void*& GetAddress( ) noexcept = 0;
};
struct FunctionDefinition : public Definition
{
public:
    virtual constexpr const std::string_view& GetName( ) const noexcept = 0;
    virtual void*& GetAddress( ) noexcept = 0;
    virtual std::vector<std::pair<void*, size_t>>& GetAllocatedHooks( )
    {
        static auto ret = std::vector<std::pair<void*, size_t>>();
        return ret;
    };
    virtual constexpr tCallFlags GetCallFlags( ) const noexcept = 0;
    virtual void* GetHook(void* hook) = 0;
    virtual void* GetSuper(void* super) = 0;
    virtual constexpr size_t GetArgCount() const noexcept = 0;
};

template<typename FnType>
struct FnBinding;

#define MaxSize MAX_DEFINITIONS_COUNT

template <typename T>
struct VariableDefinition;

template <typename T>
struct VariableDefinition : public Definition
{
public:
    using type = T;
    using ptr_type = std::add_pointer_t<T>;
    using reference_type = std::unwrap_ref_decay_t<ptr_type>;
private:
    std::string_view m_Name;
    uint32_t m_Offset;
    void* m_Address;
public:
    constexpr VariableDefinition(std::string_view name, uint32_t offset) : m_Name(name), m_Offset(offset), m_Address(nullptr) { }

    virtual constexpr const std::string_view& GetName( ) const noexcept final override
    {
        return m_Name;
    }
    inline decltype(auto) operator->( ) noexcept
    {
        GetLogger( )->LogFormatted("%s(%d): %p", __FUNCTION__, __LINE__, m_Address);
        if (m_Address) return (ptr_type) m_Address;
        GetAddress();
        return (ptr_type) m_Address;
    }
    inline decltype(auto) operator* ( )
    {
        GetLogger( )->LogFormatted("%s(%d): %p", __FUNCTION__, __LINE__, m_Address);
        if (m_Address) return *(ptr_type) m_Address;
        GetAddress();
        return *(ptr_type) m_Address;
    }
    inline decltype(auto) operator->( ) const noexcept
    {
        GetLogger( )->LogFormatted("%s(%d): %p", __FUNCTION__, __LINE__, m_Address);
        if (m_Address) return (ptr_type) m_Address;
        GetAddress( );
        return  (ptr_type) m_Address;
    }
    inline decltype(auto) operator* ( ) const noexcept
    {
        GetLogger( )->LogFormatted("%s(%d): %p", __FUNCTION__, __LINE__, m_Address);
        if (m_Address) return *(ptr_type) m_Address;
        GetAddress( );
        return *(ptr_type) m_Address;
    }
    virtual void*& GetAddress( ) noexcept final override
    {
        GetLogger( )->LogFormatted("%s(%d): %p", __FUNCTION__, __LINE__, m_Address);
        if (m_Address) return m_Address;
        static auto AppBaseAddr = (uintptr_t) GameModuleBase;
        m_Address = reinterpret_cast<void*>(AppBaseAddr + m_Offset);
        GetLogger( )->LogFormatted("%s(%d): %p", __FUNCTION__, __LINE__, m_Address);
        return m_Address;
    }
};

struct FunctionHook
{
public:
    FunctionHook( ) = default;
    static std::multimap<int, FunctionHook*, std::greater<int>>& GetHookMap()
    {
        static std::multimap<int, FunctionHook*, std::greater<int>> HookMap;
        return HookMap;
    }
    FunctionDefinition* def;
    std::string_view name;
    void* address;
    void* _hook;
    void** _outInternalSuper;
    MologieDetours::Detour<void*>* _detour;
    unsigned long lblHook;

    template <class T> FunctionHook(std::string_view _name, FunctionDefinition* _def, T Hook, void*& outInternalSuper, int32_t priority)
        : name(_name), def(_def), _detour(nullptr), _outInternalSuper(&outInternalSuper), _hook(*(void**) &Hook), address(_def->GetAddress( ))
    {

        GetHookMap( ).emplace(std::pair(priority, this));
    }

    const std::string_view& GetName() const noexcept;
    void* GetAddress( ) const noexcept;
    void Install( );
    virtual ~FunctionHook();

};
template <typename Ret>
struct FnBinding<Ret(__cdecl*)()> : public FunctionDefinition
{
public:
    using function_type = Ret(__cdecl*)();
    using wrapper_type = Ret(__stdcall*)();
private:
    void* m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_ModuleName;
protected:
    friend struct FunctionHook;
    inline explicit operator function_type( )
    {
        return reinterpret_cast<function_type>(GetAddress( ));
    }
    virtual void*& GetAddress( ) noexcept final override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GameModuleBase + (uint32_t) m_Offset);
    }
public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }

    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }

    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_ModuleName(other.m_ModuleName) { }
    constexpr ~FnBinding( ) = default;

    constexpr tCallFlags GetCallFlags( ) const noexcept final override
    {
        TCallFlags flags = TCallFlags(tCallFlags::_CDECL);
        if constexpr (std::is_same_v<Ret, void>)
            flags |= tCallFlags::_RETVOID;
        else if constexpr (sizeof(std::declval<Ret>( )) == sizeof(long long))
            flags |= tCallFlags::_RETLONGLONG;

        return flags;
    }
    constexpr size_t GetArgCount( ) const noexcept
    {
        return 0;
    }
    virtual constexpr const std::string_view& GetName( ) const noexcept final override
    {
        return m_Name;
    }
    virtual void* GetSuper(void* super) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitSuperWrapper(this, super);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("super_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End();
        return result->m_Fn;
    }
    virtual void* GetHook(void* hook) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookWrapper(this, hook);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("hook_%s_%p", GetName().data(), this->GetAddress()), labels);
        _compiler->End();
        return result->m_Fn;
    }

    inline decltype(auto) __stdcall operator()( ) noexcept
    {
        return (this->operator function_type( ))();
    }

};
static inline std::map<std::string_view, Definition*>& GetDefinitions()
{
    static std::map<std::string_view, Definition*> _defs = std::map<std::string_view, Definition*>();
    return _defs;
}

template <typename Ret, typename... Args>
struct FnBinding<Ret(__cdecl*)(Args...)> : public FunctionDefinition
{
public:
    using function_type = Ret(__cdecl*)(Args...);
    using void_type = void(__cdecl*)(Args...);
private:
    void* m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_ModuleName;

protected:
    friend struct FunctionHook;
    template <class>
        requires (!std::same_as<Ret, void>)
    inline explicit operator function_type( )
    {
        return *reinterpret_cast<function_type*>(&GetAddress( ));
    }
    template <class>
        requires (std::same_as<Ret, void>)
    inline explicit operator void_type( )
    {

    }
    virtual void*& GetAddress( ) noexcept final override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GameModuleBase + (uint32_t) m_Offset);
    }
public:
    FnBinding( ) = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_ModuleName(other.m_ModuleName) { }
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount( ) const noexcept
    {
        return (sizeof...(Args));
    }

    constexpr tCallFlags GetCallFlags( ) const noexcept final override
    {
        TCallFlags flags = TCallFlags(tCallFlags::_CDECL);
        if constexpr (std::is_same_v<Ret, void>)
            flags |= tCallFlags::_RETVOID;
        else if constexpr (sizeof(std::declval<Ret>( )) == sizeof(long long))
            flags |= tCallFlags::_RETLONGLONG;

        return flags;
    }
    virtual void* GetSuper(void* super) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitSuperWrapper(this, super);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("super_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End();
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookWrapper(this, hook);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("hook_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);

        _compiler->End();
        return result->m_Fn;
    }
    template <class>
        requires (std::same_as<void, Ret>)
    inline void __stdcall operator()(Args... args) noexcept
    {
        if (!m_Address) GetAddress( );
        ((void_type) (this->m_Address))(std::forward<Args>(args)...);
    }
    inline Ret __stdcall operator()(Args... args)
    {
        if (!m_Address) GetAddress( );
        return (((function_type) (this->m_Address))(std::forward<Args>(args)...));
    }

/*
    template <class = Ret>
    requires std::is_same_v<Ret, void>
    inline void __stdcall operator()(Args... args) noexcept
    {
        (this->operator void_type())(std::forward<Args>(args)...);
    } */
    virtual constexpr const std::string_view& GetName( ) const noexcept final override
    {
        return m_Name;
    }
};
template <typename Ret, typename... Args>
struct FnBinding<Ret(__stdcall*)(Args...)> : public FunctionDefinition
{
public:
    using function_type = Ret(__stdcall*)(Args...);
private:
    void* m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_ModuleName;
protected:
    friend struct FunctionHook;

    inline explicit operator function_type( )
    {
        return reinterpret_cast<function_type>(GetAddress( ));
    }
    virtual void*& GetAddress( ) noexcept final override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GameModuleBase + (uint32_t) m_Offset);
    }


public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_ModuleName(other.m_ModuleName) { }
    constexpr ~FnBinding( ) = default;
    constexpr tCallFlags GetCallFlags( ) const noexcept final override
    {
        TCallFlags flags = TCallFlags(tCallFlags::_STDCALL);
        if constexpr (std::is_same_v<Ret, void>)
            flags |= tCallFlags::_RETVOID;
        else if constexpr (sizeof(std::declval<Ret>( )) == sizeof(long long))
            flags |= tCallFlags::_RETLONGLONG;

        return flags;
    }

    virtual void* GetSuper(void* super) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitSuperWrapper(this, super);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("super_%s_%p", this->GetName( ).data( ), this->GetAddress( )), labels);        _compiler->End( );
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookWrapper(this, hook);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("hook_%s_%p", this->GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End();
        return result->m_Fn;
    }
    constexpr size_t GetArgCount( ) const noexcept
    {
        return (sizeof...(Args));
    }

    virtual constexpr const std::string_view& GetName( ) const noexcept final override
    {
        return m_Name;
    }
    inline decltype(auto) __stdcall operator()(Args... args) noexcept
    {
        return (this->operator function_type( ))(std::forward<Args>(args)...);
    }
};

template <typename Ret, typename classname>
struct FnBinding<Ret(classname::*)()> : public FunctionDefinition
{
public:
    using function_type = Ret(__thiscall classname::*)(void);
private:
    void* m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_ModuleName;
protected:
    friend classname;
    friend struct FunctionHook;

    inline operator function_type( )
    {
        GetAddress( );
        return *reinterpret_cast<function_type*>(&m_Address);
    }
    virtual void*& GetAddress( ) noexcept final override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GameModuleBase + (uint32_t) m_Offset);
    }
public:
    FnBinding( ) = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_ModuleName(other.m_ModuleName) { }
    constexpr ~FnBinding( ) = default;

    constexpr tCallFlags GetCallFlags( ) const noexcept final override
    {
        TCallFlags flags = TCallFlags(tCallFlags::_THISCALL);
        if constexpr (std::is_same_v<Ret, void>)
            flags |= tCallFlags::_RETVOID;
        else if constexpr (sizeof(std::declval<Ret>( )) == sizeof(long long))
            flags |= tCallFlags::_RETLONGLONG;

        return flags;
    }

    virtual void* GetSuper(void* super) final override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitSuperWrapper(this, super);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("super_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End( );
        return result->m_Fn;
    }
    virtual void* GetHook(void* hook) final override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookWrapper(this, hook);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("hook_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End( );
        return result->m_Fn;
    }
    constexpr size_t GetArgCount( ) const noexcept
    {
        return 0;
    }
    virtual constexpr const std::string_view& GetName( ) const noexcept final override
    {
        return m_Name;
    }


};

template <typename Ret, typename classname, typename... Args>
struct FnBinding<Ret(classname::*)(Args...)> : public FunctionDefinition
{
public:
    using function_type = Ret(__thiscall classname::*)(Args...);
private:
    void* m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_ModuleName;
protected:
    friend classname;
    friend struct FunctionHook;

    inline operator function_type( )
    {
        GetAddress( );
        return *reinterpret_cast<function_type*>(&m_Address);
    }
    virtual void*& GetAddress( ) noexcept final override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GameModuleBase + (uint32_t) m_Offset);
    }
public:
    FnBinding( ) = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) { }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_ModuleName(other.m_ModuleName) { }
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount( ) const noexcept
    {
        return (sizeof...(Args));
    }
    virtual constexpr const std::string_view& GetName( ) const noexcept final override
    {
        return m_Name;
    }
    constexpr tCallFlags GetCallFlags( ) const noexcept final override
    {
        TCallFlags flags = TCallFlags(tCallFlags::_THISCALL);
        if constexpr (std::is_same_v<Ret, void>)
            flags |= tCallFlags::_RETVOID;
        else if constexpr (sizeof(std::declval<Ret>( )) == sizeof(long long))
            flags |= tCallFlags::_RETLONGLONG;

        return flags;
    }

    virtual void* GetSuper(void* super) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitSuperWrapper(this, super);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("super_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End( );
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {
        auto _compiler = Arkitekt::Assembler::Get( );
        static void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookWrapper(this, hook);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger( )->ParseFormatting("hook_%s_%p", GetName( ).data( ), this->GetAddress( )), labels);
        _compiler->End( );
        return result->m_Fn;
    }
};


//
/* The following (until EOF) is from LibARK by Kilburn (@FixItVinh) */
/* It is licensed under MIT 2.0 */
/* More details available in LICENSE.md */
//

//=================================================================================================
#define _DEFINE_METHOD_HOOK1(_id, _classname, _name, _priority, _type) \
	namespace { namespace Hook_##_classname##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		struct wrapper : public _classname { \
			auto hook _type; \
			auto super _type ; \
		}; \
		static FunctionHook hookObj = FunctionHook(#_classname "::" #_name, &( _classname:: _func ## _ ## _name ), &wrapper::hook, internalSuper, _priority); \
	} } \
	auto FUNC_NAKED Hook_##_classname##_name##_id##_priority :: wrapper::super _type {__asm {JUMP_INSTRUCTION(Hook_##_classname##_name##_id##_priority ::internalSuper)}; } \
	auto Hook_##_classname##_name##_id##_priority ::wrapper::hook _type

#define _DEFINE_METHOD_HOOK0(_id, _classname, _name, _priority, ...) _DEFINE_METHOD_HOOK1(_id, _classname, _name, _priority, __VA_ARGS__)

#define HOOK_METHOD(_classname, _name, ...) _DEFINE_METHOD_HOOK0(__LINE__, _classname, _name, 0, __VA_ARGS__)
#define HOOK_METHOD_PRIORITY(_classname, _name, _priority, ...) _DEFINE_METHOD_HOOK0(__LINE__, _classname, _name, _priority, __VA_ARGS__)

//=================================================================================================

#define _DEFINE_STATIC_HOOK1(_id, _classname, _name, _callConv, _priority, _type) \
	namespace { namespace Hook_##_classname##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		struct wrapper : public _classname { \
			static auto __stdcall hook _type ; \
			static auto __stdcall super _type ; \
		}; \
		static FunctionHook hookObj(#_classname "::" #_name, &_classname ## :: ## _name, &wrapper::hook, internalSuper, _priority); \
	} } \
	auto FUNC_NAKED __stdcall Hook_##_classname##_name##_id##_priority :: wrapper::super _type {__asm {JUMP_INSTRUCTION(Hook_##_classname##_name##_id##_priority::internalSuper)}; } \
	auto __stdcall Hook_##_classname##_name##_id##_priority ::wrapper::hook _type

#define _DEFINE_STATIC_HOOK0(_id, _classname, _name, _callConv, _priority, _type) _DEFINE_STATIC_HOOK1(_id, _classname, _name, _callConv, _priority, _type)

#define HOOK_STATIC_CONV(_classname, _name, _callConv, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, _callConv, 0, _type)
#define HOOK_STATIC(_classname, _name, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, __cdecl, 0, _type)
#define HOOK_STATIC_PRIORITY(_classname, _name, _priority, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, __cdecl, _priority, _type)

//====================================================== ===========================================

#define _DEFINE_GLOBAL_HOOK1(_id, _name, _callConv, _priority, _type) \
	namespace { namespace Hook_##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		static auto __stdcall hook _type ; \
		static auto __stdcall super _type ; \
		\
		static FunctionHook hookObj(#_name, &_name, &hook, internalSuper, _priority); \
	} } \
	auto FUNC_NAKED __stdcall Hook_##_name##_id##_priority ::super _type {__asm {JUMP_INSTRUCTION(Hook_##_name##_id##_priority::internalSuper)}; } \
	auto __stdcall Hook_##_name##_id##_priority ::hook _type

#define _DEFINE_GLOBAL_HOOK0(_id, _name, _callConv, _priority, _type) _DEFINE_GLOBAL_HOOK1(_id, _name, _callConv, _priority, _type)

#define HOOK_GLOBAL_CONV(_name, _callConv, _type) _DEFINE_GLOBAL_HOOK0(__LINE__, _name, _callConv, 0, _type)
#define HOOK_GLOBAL(_name, _type) HOOK_GLOBAL_CONV(_name, __cdecl, _type)
#define HOOK_GLOBAL_PRIORITY(_name, _priority, _type) _DEFINE_GLOBAL_HOOK0(__LINE__, _name, __cdecl, _priority, _type)
