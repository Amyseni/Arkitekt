#pragma once

#include <functional>
#include <concepts>
#include <array>
#include <assert.h>
#include <cstdint>
#include <string>
#include <map>
#include <Windows.h>

#ifndef MAX_DEFINITIONS_COUNT
#define MAX_DEFINITIONS_COUNT 16384
#endif
#define GAME_EXE "SYNTHETIK.exe"
#define FUNC_NAKED __declspec(naked)
#define JUMP_INSTRUCTION(target) jmp [target]

class Definition {
public:
    virtual ~Definition() {}
    virtual const std::string_view GetName() const noexcept = 0;
    virtual void* GetAddress() noexcept = 0;
};
class FunctionDefinition : public Definition {
public:
    virtual const std::string_view GetName() const noexcept = 0;
    virtual void* GetAddress() noexcept = 0;
    virtual constexpr size_t GetArgCount() const noexcept = 0;
    virtual constexpr short *GetArgData() const noexcept = 0;
    virtual constexpr bool isMemPassedStructPointer() const noexcept = 0;
    virtual constexpr bool IsVoid() const noexcept = 0;
    virtual constexpr bool IsLongLong() const noexcept = 0;
    virtual constexpr bool IsThiscall() const noexcept = 0;
    virtual constexpr bool NeedsCallerCleanup() const noexcept = 0;
    virtual constexpr bool forceDetourSize() const noexcept { 
        return false;
    }
};

template<typename FnType>
class FnBinding;

#define MaxSize MAX_DEFINITIONS_COUNT

template <typename T>
class VariableDefinition : public Definition
{
public:
    using variable_type = T;
    using ptr_type = std::add_pointer_t<T>;
private: 
    std::string_view m_Name;
    uint32_t m_Offset;
    variable_type* _var = nullptr;
public:
    constexpr VariableDefinition(std::string_view name, uint32_t offset) : m_Name(name), m_Offset(offset), _var(nullptr) {}

    virtual constexpr const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    inline variable_type& operator*() noexcept
    {
        if (_var && *_var) return *_var;
        GetAddress();
        return *_var;
    }
    inline const variable_type& operator*() const noexcept
    {
        if (_var && *_var) return *_var;
        GetAddress();
        return *_var;
    }
    virtual void* GetAddress() noexcept final override
    {
        if (_var && *_var) return reinterpret_cast<void*>(_var);
        static void* AppBaseAddr = GetModuleHandleA(GAME_EXE);
        static void* CalculatedAddress = nullptr;
        if (!CalculatedAddress) CalculatedAddress = reinterpret_cast<void*>((uint32_t) AppBaseAddr + (uint32_t) m_Offset);
        return _var = reinterpret_cast<ptr_type>(CalculatedAddress);
    }
};


struct DefinitionMap {
    static inline constinit std::array<Definition*, MaxSize> _defs;
    static inline constinit std::size_t offset = 0;
    
    constexpr static void Add(FunctionDefinition* def, size_t _offset)
    {
        
        // _defs[_offset] = (Definition*) def; // (Definition*)(malloc(sizeof(DefType)));
        // ::new(_defs[offset]) DefType(std::forward<Args&&>(args)...);
    }

    inline static std::unordered_map<std::string_view, Definition*> _definitionMap = []() -> auto {
        static auto ret = std::unordered_map<std::string_view, Definition*>();
        if (ret.size()) return ret;
        
        for (size_t i=0;i<offset;i++)
        {
            ret.insert(std::make_pair(_defs[i]->GetName(), _defs[i]));
        }
        return ret;
    }();
};

// constinit inline SigScanner _Scanner;

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
    void* _internalHook = std::_Allocate<16>(128u);
    void* _internalSuper = std::_Allocate<16>(128u);
    void** _outInternalSuper = nullptr;
    void* address = nullptr;
    void* _hook;
    void* _detour;

    unsigned int _hSize = 0;
    unsigned int _sSize = 0;
    
    FunctionHook(std::string_view _name, FunctionDefinition* _def, void* Hook, void** outInternalSuper, int32_t priority)
        : name(_name), _outInternalSuper(outInternalSuper), _hook(Hook), def(_def)  {
            address = _def->GetAddress();
            *_outInternalSuper = _internalSuper;
            GetHookMap().insert(std::pair(priority, this));
        }
    template <typename FnType>
    FunctionHook(std::string_view _name, void* _def, void* Hook, void** outInternalSuper, int32_t priority)
        : name(_name), _outInternalSuper(outInternalSuper), _hook(Hook), def(_def)  {
            def = malloc(sizeof(FnBinding<FnType>));
            ::new(def) FnBinding<FnType>(_name, _def, "aa");
            address = _def->GetAddress();
            *_outInternalSuper = _internalSuper;
            GetHookMap().insert(std::pair(priority, this));
        }
    
    const std::string_view& GetName() const noexcept {
        return name;
    }

    void * GetAddress() const noexcept {
        return address;
    }

    int Install();

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
    intptr_t m_Offset;
    std::string_view m_Signature;

public:
    constexpr FnBinding(const char* name, intptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {
        DefinitionMap::Add(this, 0);
    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {}
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return 0;
    }
    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    constexpr short * GetArgData() const noexcept {
        static std::array<short, 0> argData{};

        return argData.data();
    }
    constexpr bool isMemPassedStructPointer() const noexcept {
        return false;
    }    
    constexpr bool NeedsCallerCleanup() const noexcept {
        return true;
    }
    constexpr bool IsVoid() const noexcept {
        return std::is_same_v<Ret, void>;
    }

    constexpr bool IsLongLong() const noexcept {
        if constexpr (typeid(Ret) == typeid(double)) return true;
        return false;
    }

    constexpr bool IsThiscall() const noexcept {
        return false;
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

    virtual void* GetAddress() noexcept final override
    {
        if (m_Address) return m_Address;
        static void* AppBaseAddr = (void*) GetModuleHandleA(GAME_EXE);
        static void* CalculatedAddress = nullptr;
        if (!CalculatedAddress) CalculatedAddress = reinterpret_cast<void*>((uint32_t) AppBaseAddr + (uint32_t) m_Offset);
        return m_Address = CalculatedAddress;
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
    intptr_t m_Offset;
    std::string_view m_Signature;

public:
    constexpr FnBinding(const char* name, intptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {
        DefinitionMap::Add(this, 0);
    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {

    }
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return (sizeof...(Args));
    }

    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    constexpr bool NeedsCallerCleanup() const noexcept {
        return true;
    }

    constexpr short* GetArgData() const noexcept {
        constexpr size_t ArgsLen = sizeof...(Args);
        static std::array<short, ArgsLen> argData{};
        if (argData.empty())
        {
            argData.fill(0x1ff); // stack args
        }

        return argData.data();
    }
    
    constexpr bool isMemPassedStructPointer() const noexcept {
        return false;
    }
    constexpr bool IsVoid() const noexcept {
        return std::is_same_v<Ret, void>;
    }

    constexpr bool IsLongLong() const noexcept {
        if constexpr(typeid(Ret) == typeid(double)) return true;
        return false;
    }

    constexpr bool IsThiscall() const noexcept {
        return false;
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

    virtual void* GetAddress() noexcept final override
    {
        if (m_Address) return m_Address;
        static void* AppBaseAddr = (void*) GetModuleHandleA(GAME_EXE);
        static void* CalculatedAddress = nullptr;
        if (!CalculatedAddress) CalculatedAddress = reinterpret_cast<void*>((uint32_t) AppBaseAddr + (uint32_t) m_Offset);
        return m_Address = CalculatedAddress;
    }
};

template <typename Ret, class classname>
class FnBinding<Ret(classname::*)()> : public FunctionDefinition
{
public:
    using function_type = Ret(classname::*)(void);
private:
    void *m_Address = nullptr;
    std::string_view m_Name;
    intptr_t m_Offset;
    std::string_view m_Signature;

    class Proxy {
        classname* m_Instance;
        const FnBinding* m_Binding;

        constexpr Proxy(classname* _instance, const FnBinding* _binding) 
            : m_Instance(_instance), m_Binding(_binding) { }
        
        inline auto operator()() const -> 
            std::invoke_result_t<function_type, classname*, void>
        {
            return (m_Instance->*(m_Binding->operator function_type()))();
        }

        template <typename...>
        requires std::is_same_v<Ret, void>
        inline void operator()() const
        {
            (m_Instance->*(m_Binding->operator function_type()))();
        }
    };

public:
    constexpr FnBinding(const char* name, intptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {
        DefinitionMap::Add(this, 0);
    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {}
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return 1;
    }
    constexpr bool NeedsCallerCleanup() const noexcept {
        return false;
    }
    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }
    constexpr short* GetArgData() const noexcept {
        static std::array<short, 1> argData{0x101};

        return argData.data();
    }
    constexpr bool isMemPassedStructPointer() const noexcept {
        return false;
    }
    constexpr bool IsVoid() const noexcept {
        return std::is_same_v<Ret, void>;
    }

    constexpr bool IsLongLong() const noexcept {
        if constexpr(typeid(Ret) == typeid(double)) return true;
        return false;
    }

    constexpr bool IsThiscall() const noexcept {
        return true;
    }

    inline operator function_type() noexcept
    {
        GetAddress();
        return *reinterpret_cast<function_type*>(&m_Address);
    }

    inline Proxy operator->*(classname* instance) const
    {
        return Proxy(instance, this);
    }

    inline auto operator()(classname* _self) const
    {
        return (_self->*(this->operator function_type()))();
    }

    template <typename...>
    requires std::is_same_v<void, Ret>
    inline void operator()(classname* _self) const
    {
        (_self->*(this->operator function_type()))();
    }

    inline auto operator()(classname* _self)
    {
        return (_self->*(this->operator function_type()))();
    }

    template <typename...>
    requires std::is_same_v<void, Ret>
    inline void operator()(classname* _self)
    {
        (_self->*(this->operator function_type()))();
    }

    virtual void* GetAddress() noexcept final override
    {
        if (m_Address) return m_Address;
        static void* AppBaseAddr = (void*) GetModuleHandleA(GAME_EXE);
        static void* CalculatedAddress = nullptr;
        if (!CalculatedAddress) CalculatedAddress = reinterpret_cast<void*>((uint32_t) AppBaseAddr + (uint32_t) m_Offset);
        return m_Address = CalculatedAddress;
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
    intptr_t m_Offset;
    std::string_view m_Signature;

    class Proxy {
        classname* m_Instance;
        const FnBinding* m_Binding;

        constexpr Proxy(classname* _instance, const FnBinding* _binding) 
            : m_Instance(_instance), m_Binding(_binding) { }
        
        inline auto operator()(Args... args) const -> 
            std::invoke_result_t<function_type, classname*, Args...>
        {
            return (m_Instance->*(m_Binding->GetAddress()))(std::forward<Args>(args)...);
        }

        template <typename...>
        requires std::is_same_v<Ret, void>
        inline void operator()(Args... args) const
        {
            (m_Instance->*(m_Binding->GetAddress()))(std::forward<Args>(args)...);
        }
    };

public:
    constexpr FnBinding(const char* name, intptr_t offset, const char* sig) : m_Name(name), m_Offset(offset), m_Signature(sig) {
        DefinitionMap::Add(this, 0);
    }
    FnBinding(const FnBinding& other) : m_Name(other.name), m_Offset(other.offset), m_Signature(other.sig) {}
    constexpr ~FnBinding() = default;

    constexpr size_t GetArgCount() const noexcept {
        return (sizeof...(Args) + 1); // 1 extra for 'this'
    }
    virtual const std::string_view GetName() const noexcept final override {
        return m_Name;
    }

    constexpr short* GetArgData() const noexcept {
        static std::array<short, (sizeof...(Args) + 1)> argData{};
        if (argData.empty())
        {
            argData.fill(0x1ff);// stack args
            argData[0] = 0x101; // this <ecx>
        }

        return argData.data();
    }

    inline operator function_type() noexcept
    {
        GetAddress();
        return *reinterpret_cast<function_type*>(&m_Address);
    }

    constexpr bool isMemPassedStructPointer() const noexcept {
        return false;
    }

    constexpr bool IsVoid() const noexcept {
        return std::is_same_v<Ret, void>;
    }

    constexpr bool NeedsCallerCleanup() const noexcept {
        return false;
    }

    constexpr bool IsLongLong() const noexcept {
        if constexpr(typeid(Ret) == typeid(double)) return true;
        return false;
    }

    constexpr bool IsThiscall() const noexcept {
        return true;
    }

    inline Proxy operator->*(classname* instance) const
    {
        return Proxy(instance, this);
    }

    inline auto operator()(classname* _self, Args... args) const
    {
        return (_self->*(this->operator function_type()))(std::forward<Args>(args)...);
    }

    template <typename...>
    requires std::is_same_v<void, Ret>
    inline void operator()(classname* _self, Args... args) const
    {
        (_self->*(this->operator function_type()))(std::forward<Args>(args)...);
    }
    
    inline auto operator()(classname* _self, Args... args)
    {
        return (_self->*(this->operator function_type()))(std::forward<Args>(args)...);
    }

    template <typename...>
    requires std::is_same_v<void, Ret>
    inline void operator()(classname* _self, Args... args)
    {
        (_self->*(this->operator function_type()))(std::forward<Args>(args)...);
    }
    virtual void* GetAddress() noexcept final override
    {
        if (m_Address) return m_Address;
        static void* AppBaseAddr = (void*) GetModuleHandleA(GAME_EXE);
        static void* CalculatedAddress = nullptr;
        if (!CalculatedAddress) CalculatedAddress = reinterpret_cast<void*>((uint32_t) AppBaseAddr + (uint32_t) m_Offset);
        return m_Address = CalculatedAddress;
    }
};


//=================================================================================================
#define _DEFINE_METHOD_HOOK1(_id, _classname, _name, _priority, ...) \
	namespace { namespace Hook_##_classname##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		struct wrapper : public _classname { \
			auto hook __VA_ARGS__ ; \
			auto super __VA_ARGS__ ; \
		}; \
		static FunctionHook hookObj = FunctionHook(#_classname "::" #_name, typeid(auto (_classname::*) __VA_ARGS__), &wrapper::hook, &internalSuper, _priority); \
	} } \
	auto FUNC_NAKED Hook_##_classname##_name##_id##_priority :: wrapper::super __VA_ARGS__ {__asm {JUMP_INSTRUCTION(Hook_##_classname##_name##_id##_priority ::internalSuper)}; } \
	auto Hook_##_classname##_name##_id##_priority ::wrapper::hook __VA_ARGS__

#define _DEFINE_METHOD_HOOK0(_id, _classname, _name, _priority, ...) _DEFINE_METHOD_HOOK1(_id, _classname, _name, _priority, __VA_ARGS__)

#define HOOK_METHOD(_classname, _name, ...) _DEFINE_METHOD_HOOK0(__LINE__, _classname, _name, 0, __VA_ARGS__)
#define HOOK_METHOD_PRIORITY(_classname, _name, _priority, ...) _DEFINE_METHOD_HOOK0(__LINE__, _classname, _name, _priority, __VA_ARGS__)

//=================================================================================================

#define _DEFINE_STATIC_HOOK1(_id, _classname, _name, _priority, _type) \
	namespace { namespace Hook_##_classname##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		struct wrapper : public _classname { \
			static auto __stdcall hook _type ; \
			static auto __stdcall super _type ; \
		}; \
		static FunctionHook hookObj(#_classname "::" #_name, typeid(auto (*) _type), &wrapper::hook, &internalSuper, _priority); \
	} } \
	auto FUNC_NAKED Hook_##_classname##_name##_id##_priority :: wrapper::super _type {__asm {JUMP_INSTRUCTION(Hook_##_classname##_name##_id##_priority::internalSuper)}; } \
	auto Hook_##_classname##_name##_id##_priority ::wrapper::hook _type

#define _DEFINE_STATIC_HOOK0(_id, _classname, _name, _priority, _type) _DEFINE_STATIC_HOOK1(_id, _classname, _name, _priority, _type)

#define HOOK_STATIC(_classname, _name, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, 0, _type)
#define HOOK_STATIC_PRIORITY(_classname, _name, _priority, _type) _DEFINE_STATIC_HOOK0(__LINE__, _classname, _name, _priority, _type)

//====================================================== ===========================================

#define _DEFINE_GLOBAL_HOOK1(_id, _name, _priority, _type) \
	namespace { namespace Hook_##_name##_id##_priority { \
		static void *internalSuper = NULL; \
		static auto __stdcall hook _type ; \
		static auto __stdcall super _type ; \
		\
		static FunctionHook hookObj(#_name, &_name, (void*) &hook, &internalSuper, _priority); \
	} } \
	auto FUNC_NAKED __stdcall Hook_##_name##_id##_priority ::super _type {__asm {JUMP_INSTRUCTION(Hook_##_name##_id##_priority::internalSuper)}; } \
	auto __stdcall Hook_##_name##_id##_priority ::hook _type

#define _DEFINE_GLOBAL_HOOK0(_id, _name, _priority, _type) _DEFINE_GLOBAL_HOOK1(_id, _name, _priority, _type)

#define HOOK_GLOBAL(_name, _type) _DEFINE_GLOBAL_HOOK0(__LINE__, _name, 0, _type)
#define HOOK_GLOBAL_PRIORITY(_name, _priority, _type) _DEFINE_GLOBAL_HOOK0(__LINE__, _name, _priority, _type)


// struct CInstance;
// constinit inline auto LOCK_RVALUE_MUTEX = FnBinding<void (__cdecl*)()>("LOCK_RVALUE_MUTEX", 0x41414141, "abc123");
// constinit inline auto someBoundCondition = FnBinding<bool (__cdecl*)(int, int, int)>("test_function", 0x41414141, "abc123");

// struct Test;

// struct Test {
//     struct {
//         void (Test::*t)(int a1, int a2, int a3) = 0;
//     }* _vtable;

//     inline Test() {
//         (this->*(_vtable->t))(1,2,3);
        
//         _vtable = (decltype(_vtable)) malloc(sizeof(_vtable));
//     }

//     template <typename...Args>
//     inline void TestFunc(Args&&...args) {
//         static FnBinding<void (Test::*)(int, int, int)> _func = FnBinding<void (Test::*)(int, int, int)>("LOCK_RVALUE_MUTEX", 0x41414141, "abc123");
//         _func.operator()(this, std::forward<Args>(args)...);
//     };

// };

// void test()
// {
    
//     LOCK_RVALUE_MUTEX();
    
//     someBoundCondition(1,2,3);
//     Test *t;
//     t->TestFunc(1, 2, 4);
//     Test t2;
//     t2.TestFunc(1, 2, 3);
// }