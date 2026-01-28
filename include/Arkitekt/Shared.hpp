#pragma once

#include <functional>
#include <concepts>
#include <array>
#include <string>
#include <map>
#include <fstream>
#include <shared_mutex>
#include <string>
#include <Arkitekt/Compiler.h>
#include <filesystem>

#define GAME_EXE "SYNTHETIK.exe"
#define FUNC_NAKED __declspec(naked)
#define JUMP_INSTRUCTION(target) jmp [target]


namespace MologieDetours{
    template <class FnType> class Detour;
};
namespace Arkitekt{
    class Logger 
    {
    public:
        ~Logger();
        bool Init(const std::string& filename);
        void Cleanup(void);
        
        bool LogSimple(const char* text, bool flushLine = true);
        bool LogFormatted(const char* fmt, ...);
        std::string ParseFormatting(const char* fmt, ...);
        std::string ParseFormatting(const char* fmt, va_list args);
        Logger(std::filesystem::path path=std::filesystem::current_path(), const char* filename="organik.log");
    private:
        std::ofstream outFile;
        static std::shared_mutex logMutex;
        bool WriteToLog(const std::string& message, bool flushLine=true);
    };
    Logger* GetLogger();
}
using namespace Arkitekt;


class Definition {
public:
    virtual ~Definition() {}
    virtual const std::string_view GetName() const noexcept = 0;
    virtual void *__stdcall GetAddress() noexcept = 0;
};
class FunctionDefinition : public Definition {
public:
    virtual const std::string_view GetName() const noexcept = 0;
    virtual void *__stdcall GetAddress() noexcept = 0;
    virtual std::vector<std::pair<void*, size_t>>& GetAllocatedHooks() 
    {
        static auto ret = std::vector<std::pair<void*, size_t>>();
        return ret;
    };
    virtual void* GetHook(void* hook) = 0;
    virtual void* GetSuper(void* super) = 0;
    virtual constexpr size_t GetArgCount() const noexcept = 0;
};

template<typename FnType>
class FnBinding;

#define MaxSize MAX_DEFINITIONS_COUNT

template <typename T>
class VariableDefinition;

template <typename T>
class VariableDefinition<T*> : public Definition
{
public:
    using variable_type = std::add_lvalue_reference_t<std::remove_pointer_t<T*>>;
    using ptr_type = T;
private: 
    std::string_view m_Name;
    uint32_t m_Offset;
    ptr_type *_var = nullptr;
public:
    constexpr VariableDefinition(std::string_view name, uint32_t offset) : m_Name(name), m_Offset(offset), _var(nullptr) {}

    virtual constexpr const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    inline ptr_type* operator*() noexcept
    {
        if (_var) return _var;
        GetAddress();
        return _var;
    }
    inline const ptr_type* operator*() const noexcept
    {
        if (_var) return _var;
        GetAddress();
        return _var;
    }
    virtual void *__stdcall GetAddress() noexcept override
    {
        if (_var) return reinterpret_cast<void*>(_var);
        static auto AppBaseAddr = (uintptr_t) GetModuleHandleA(nullptr);
        memcpy_s(&_var, sizeof(ptr_type), reinterpret_cast<void*>(AppBaseAddr + m_Offset), sizeof(ptr_type));
        return _var;
    }
};

class FunctionHook
{
public:
    static std::multimap<int, FunctionHook*, std::greater<int>>& GetHookMap()
    {
        static std::multimap<int, FunctionHook*, std::greater<int>> HookMap;
        return HookMap;
    }
    FunctionDefinition* def;
    std::string_view name;
    void* address;
    void* _hook;
    void **_outInternalSuper;
    MologieDetours::Detour<void*>* _detour;
    unsigned long lblHook;
    
    template <class T> FunctionHook(std::string_view _name, FunctionDefinition* _def, T Hook, void*& outInternalSuper, int32_t priority)
        : name(_name), def(_def), _detour(nullptr), _outInternalSuper(&outInternalSuper), _hook(*(void**)&Hook)  {
            GetHookMap().emplace(std::pair(priority, this));
        }

    const std::string_view& GetName() const noexcept;
    void * GetAddress() const noexcept;
    void Install () ;
    virtual ~FunctionHook();

};
template <typename Ret>
class FnBinding<Ret(__cdecl*)()> : public FunctionDefinition
{
public:
    using function_type = Ret(__cdecl*)();
private:
    void *m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_Signature;

public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) {

    }

    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) {

    }
    
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_ModuleName(other.sig) {}
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return 0;
    }
    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    virtual void* GetSuper(void* super) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookPrologue(0, std::is_same<Ret,void>::value, true);
        _compiler->EmitCall(super);
        _compiler->EmitHookEpilogue(0, std::is_same<Ret,void>::value, true);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("super_%s_%p", GetName().data(), this->GetAddress()));
        _compiler->End();
        return result->m_Fn;
    }
    virtual void* GetHook(void* hook) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookPrologue(0, std::is_same<Ret,void>::value, true);
        _compiler->EmitCall(hook);
        _compiler->EmitHookEpilogue(0, std::is_same<Ret,void>::value, true);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("hook_%s_%p", GetName().data(), this->GetAddress()), labels);
        _compiler->End();
        return result->m_Fn;
    }

    inline operator function_type() noexcept
    {
        return reinterpret_cast<function_type>(GetAddress());
    }

    inline auto operator()() const
    {
        return this->operator function_type()();
    }

    template <typename...>
    requires std::is_same_v<void, Ret>
    inline void operator()() const
    {
        this->operator function_type()();
    }

    inline auto operator()()
    {
        return this->operator function_type()();
    }

    template <typename...>
    requires std::is_same_v<void, Ret>
    inline void operator()()
    {
        this->operator function_type()();
    }

    virtual void *__stdcall GetAddress() noexcept override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GetModuleHandleA(nullptr) + (uint32_t) m_Offset);
    }

};
static inline std::map<std::string_view, Definition*>& GetDefinitions()
{
    static std::map<std::string_view, Definition*> _defs = std::map<std::string_view, Definition*>();
    return _defs;
}

template <typename Ret, typename... Args>
class FnBinding<Ret(__cdecl*)(Args...)> : public FunctionDefinition
{
public:
    using function_type = Ret(__cdecl*)(Args...);
private:
    void *m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_ModuleName;
public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_ModuleName(sig) {

    }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {

    }
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return (sizeof...(Args));
    }

    virtual void* GetSuper(void* super) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        constexpr size_t argsSize = sizeof...(Args);
        _compiler->EmitHookPrologue(argsSize, std::is_same<Ret,void>::value, true);
        _compiler->EmitCall(super);
        _compiler->EmitHookEpilogue(argsSize, std::is_same<Ret,void>::value, true);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("super_%s_%p", GetName().data(), this->GetAddress()));
        _compiler->End();
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        constexpr size_t argsSize = sizeof...(Args);
        _compiler->EmitHookPrologue(argsSize, std::is_same<Ret,void>::value, true);
        _compiler->EmitCall(hook);
        _compiler->EmitHookEpilogue(argsSize, std::is_same<Ret,void>::value, true);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("hook_%s_%p", GetName().data(), this->GetAddress()));

        _compiler->End();
        return result->m_Fn;
    }

    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }

    inline operator function_type() noexcept
    {
        return reinterpret_cast<function_type>(GetAddress());
    }

    inline auto operator()(Args &&...args) const
    {
        return this->operator function_type()(std::forward<Args&&>(args)...);
    }

    template <std::enable_if_t<std::is_invocable_r_v<void, function_type, Args...>, bool> = true>
    requires std::is_same_v<void, Ret>
    inline void operator()(Args &&...args) const
    {
        this->operator function_type()(std::forward<Args&&>(args)...);
    }

    inline auto operator()(Args &&...args)
    {
        return this->operator function_type()(std::forward<Args&&>(args)...);
    }

    template <std::enable_if_t<std::is_invocable_r_v<void, function_type, Args...>, bool> = true>
    requires std::is_same_v<void, Ret>
    inline void operator()(Args &&...args)
    {
        this->operator function_type()(std::forward<Args&&>(args)...);
    }

    virtual void *__stdcall GetAddress() noexcept override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GetModuleHandleA(nullptr) + (uint32_t) m_Offset);
    }
};
template <typename Ret, typename... Args>
class FnBinding<Ret(__stdcall*)(Args...)> : public FunctionDefinition
{
public:
    using function_type = Ret(__stdcall*)(Args...);
private:
    void *m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_Signature;

public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {

    }
    constexpr ~FnBinding() = default;
    virtual void* GetSuper(void* super) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        constexpr size_t argsSize = sizeof...(Args);
        _compiler->EmitHookPrologue(argsSize, std::is_same<Ret,void>::value, false);
        _compiler->EmitCall(super);
        _compiler->EmitHookEpilogue(argsSize, std::is_same<Ret,void>::value, false);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("super_%s_%p", GetName().data(), this->GetAddress()));        _compiler->End();
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        constexpr size_t argsSize = sizeof...(Args);
        _compiler->EmitHookPrologue(argsSize, std::is_same<Ret,void>::value, false);
        _compiler->EmitCall(hook);
        _compiler->EmitHookEpilogue(argsSize, std::is_same<Ret,void>::value, false);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("hook_%s_%p", GetName().data(), this->GetAddress()));
        _compiler->End();
        return result->m_Fn;
    }
    constexpr size_t GetArgCount() const noexcept {
        return (sizeof...(Args));
    }

    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }

    inline operator function_type() noexcept
    {
        return reinterpret_cast<function_type>(GetAddress());
    }

    inline auto operator()(Args &&...args) const
    {
        return this->operator function_type()(std::forward<Args&&>(args)...);
    }

    template <std::enable_if_t<std::is_invocable_r_v<void, function_type, Args...>, bool> = true>
    requires std::is_same_v<void, Ret>
    inline void operator()(Args &&...args) const
    {
        this->operator function_type()(std::forward<Args&&>(args)...);
    }

    inline auto operator()(Args &&...args)
    {
        return this->operator function_type()(std::forward<Args&&>(args)...);
    }

    template <std::enable_if_t<std::is_invocable_r_v<void, function_type, Args...>, bool> = true>
    requires std::is_same_v<void, Ret>
    inline void operator()(Args &&...args)
    {
        this->operator function_type()(std::forward<Args&&>(args)...);
    }

    virtual void *__stdcall GetAddress() noexcept override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GetModuleHandleA(nullptr) + (uint32_t) m_Offset);
    }
};

template <typename Ret, class classname>
class FnBinding<Ret(classname::*)()> : public FunctionDefinition
{
public:
    using function_type = Ret(classname::*)(void);
    using function_type_const = Ret(classname::*)(void) const;
private:
    void *m_Address = nullptr;
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_Signature;

public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {}
    constexpr ~FnBinding() = default;
    virtual void* GetSuper(void* super) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookPrologue(0, std::is_same<Ret,void>::value, false);
        _compiler->EmitCall(super);
        _compiler->EmitHookEpilogue(0, std::is_same<Ret,void>::value, false);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("super_%s_%p", GetName().data(), this->GetAddress()));        _compiler->End();
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        _compiler->EmitHookPrologue(0, std::is_same<Ret,void>::value, false);
        _compiler->EmitCall(hook);
        _compiler->EmitHookEpilogue(0, std::is_same<Ret,void>::value, false);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("hook_%s_%p", GetName().data(), this->GetAddress()));        _compiler->End();
        return result->m_Fn;
    }    
    constexpr size_t GetArgCount() const noexcept {
        return 1;
    }
    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }

    inline operator function_type() noexcept
    {
        GetAddress();
        return *reinterpret_cast<function_type*>(&m_Address);
    }

    virtual void *__stdcall GetAddress() noexcept override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GetModuleHandleA(nullptr) + (uint32_t) m_Offset);
    }

};

template <typename Ret, class classname, typename... Args>
class FnBinding<Ret(classname::*)(Args...)> : public FunctionDefinition
{
public:
    using function_type = Ret(classname::*)(Args...);
    void *m_Address = nullptr;
private:
    std::string_view m_Name;
    uintptr_t m_Offset;
    std::string_view m_Signature;

public:
    FnBinding() = default;
    constexpr FnBinding(const char* name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    constexpr FnBinding(const std::string_view& name, uintptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {

    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {}
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return (sizeof...(Args) + 1); // 1 extra for 'this'
    }
    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    virtual void* GetSuper(void* super) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        constexpr size_t argsSize = sizeof...(Args);
        _compiler->EmitHookPrologue(argsSize, std::is_same<Ret,void>::value, false);
        _compiler->EmitCall(super);
        _compiler->EmitHookEpilogue(argsSize, std::is_same<Ret,void>::value, false);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("super_%s_%p", GetName().data(), this->GetAddress()));        _compiler->End();
        return result->m_Fn;
    }

    virtual void* GetHook(void* hook) override
    {  
        auto _compiler = Arkitekt::Compiler::Get();
        void** labels;
        _compiler->Begin(&labels);
        constexpr size_t argsSize = sizeof...(Args);
        _compiler->EmitHookPrologue(argsSize, std::is_same<Ret,void>::value, false);
        _compiler->EmitCall(hook);
        _compiler->EmitHookEpilogue(argsSize, std::is_same<Ret,void>::value, false);
        FnBlock* result = _compiler->FinalizeFunction(GetLogger()->ParseFormatting("hook_%s_%p", GetName().data(), this->GetAddress()));        _compiler->End();
        return result->m_Fn;
    }    
    inline operator function_type() noexcept
    {
        GetAddress();
        return *reinterpret_cast<function_type*>(&m_Address);
    }

    virtual void *__stdcall GetAddress() noexcept override
    {
        if (m_Address) return m_Address;
        return m_Address = reinterpret_cast<void*>((uint32_t) GetModuleHandleA(nullptr) + (uint32_t) m_Offset);
    }
};


//
/* The following (until EOF) is from LibZHL by Kilburn (@FixItVinh) */
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
		static FunctionHook hookObj = FunctionHook(#_classname "::" #_name, &(_classname:: _name ## _func), &wrapper::hook, internalSuper, _priority); \
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
			static auto _callConv hook _type ; \
			static auto _callConv super _type ; \
		}; \
		static FunctionHook hookObj(#_classname "::" #_name, &_classname ## :: ## _name, &wrapper::hook, internalSuper, _priority); \
	} } \
	auto FUNC_NAKED _callConv Hook_##_classname##_name##_id##_priority :: wrapper::super _type {__asm {JUMP_INSTRUCTION(Hook_##_classname##_name##_id##_priority::internalSuper)}; } \
	auto _callConv Hook_##_classname##_name##_id##_priority ::wrapper::hook _type

#define _DEFINE_STATIC_HOOK0(_id, _classname, _name, _callConv, _priority, _type) _DEFINE_STATIC_HOOK1(_id, _classname, _name, _callConv, _priority, _type)

#define HOOK_STATIC_CONV(_classname, _name, _callConv, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, _callConv, 0, _type)
#define HOOK_STATIC(_classname, _name, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, __cdecl, 0, _type)
#define HOOK_STATIC_PRIORITY(_classname, _name, _priority, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, __cdecl, _priority, _type)

//====================================================== ===========================================

#define _DEFINE_GLOBAL_HOOK1(_id, _name, _callConv, _priority, _type) \
	namespace { namespace Hook_##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		static auto _callConv hook _type ; \
		static auto _callConv super _type ; \
		\
		static FunctionHook hookObj(#_name, &_name, &hook, internalSuper, _priority); \
	} } \
	auto FUNC_NAKED _callConv Hook_##_name##_id##_priority ::super _type {__asm {JUMP_INSTRUCTION(Hook_##_name##_id##_priority::internalSuper)}; } \
	auto _callConv Hook_##_name##_id##_priority ::hook _type

#define _DEFINE_GLOBAL_HOOK0(_id, _name, _callConv, _priority, _type) _DEFINE_GLOBAL_HOOK1(_id, _name, _callConv, _priority, _type)

#define HOOK_GLOBAL_CONV(_name, _callConv, _type) _DEFINE_GLOBAL_HOOK0(__LINE__, _name, _callConv, 0, _type)
#define HOOK_GLOBAL(_name, _type) HOOK_GLOBAL_CONV(_name, __cdecl, _type)
#define HOOK_GLOBAL_PRIORITY(_name, _priority, _type) _DEFINE_GLOBAL_HOOK0(__LINE__, _name, __cdecl, _priority, _type)
