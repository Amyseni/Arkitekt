/*
** This file has been pre-processed with DynASM.
** https://luajit.org/dynasm.html
** DynASM version 1.5.0, DynASM x86 version 1.5.0
** DO NOT EDIT! The original file is in "include/Arkitekt/Compiler.cpp".
*/

#line 1 "include/Arkitekt/Compiler.cpp"
#include <Arkitekt/Compiler.h>
#include <dynasm/dasm_x86.h>
#include <dynasm/dasm_proto.h>

//|.arch x86
#if DASM_VERSION != 10500
#error "Version mismatch between DynASM and included encoding engine"
#endif
#line 6 "include/Arkitekt/Compiler.cpp"
//|.section code
#define DASM_SECTION_CODE	0
#define DASM_MAXSECTION		1
#line 7 "include/Arkitekt/Compiler.cpp"
//|.globals lbl_
enum {
  lbl__func,
  lbl__MAX
};
#line 8 "include/Arkitekt/Compiler.cpp"
//|.actionlist compiler_actions
static const unsigned char compiler_actions[61] = {
  252,255,21,237,255,252,255,37,237,255,254,0,250,15,248,10,85,137,229,255,
  82,80,255,81,83,86,87,255,252,255,181,233,255,129,196,239,255,95,94,91,89,
  255,88,90,255,137,252,236,93,255,194,236,255,195,255,80,82,255,90,88,255
};

#line 9 "include/Arkitekt/Compiler.cpp"



using namespace Arkitekt;
void Arkitekt::Compiler::EmitCall(void* address)
{
    dasm_State** Dst = &this->dasm;
    //| call dword [address]
    dasm_put(Dst, 0, address);
#line 17 "include/Arkitekt/Compiler.cpp"
}
void Arkitekt::Compiler::EmitJump(void* address)
{
    dasm_State** Dst = &this->dasm;
    //| jmp dword [address]
    dasm_put(Dst, 5, address);
#line 22 "include/Arkitekt/Compiler.cpp"
}


void Arkitekt::Compiler::EmitHookPrologue(std::size_t _nArgs, bool _isVoid, bool _isCdecl)
{
    dasm_State** Dst = &this->dasm;
    //|.code
    dasm_put(Dst, 10);
#line 29 "include/Arkitekt/Compiler.cpp"
    //|.align 0x10
    //|->_func:
    //| push ebp
    //| mov ebp, esp
    dasm_put(Dst, 12);
#line 33 "include/Arkitekt/Compiler.cpp"
    if (_isVoid)
    {
        //| push edx
        //| push eax
        dasm_put(Dst, 20);
#line 37 "include/Arkitekt/Compiler.cpp"
    }
    //| push ecx
    //| push ebx
    //| push esi
    //| push edi
    dasm_put(Dst, 23);
#line 42 "include/Arkitekt/Compiler.cpp"
    if (_nArgs > 0) 
    {
        for (int32_t i=_nArgs-1;i>=0;i--)
        {
            //| push dword [ebp+8+(4*i)]
            dasm_put(Dst, 28, 8+(4*i));
#line 47 "include/Arkitekt/Compiler.cpp"
        }
    }
}

void Arkitekt::Compiler::EmitHookEpilogue(std::size_t _nArgs, bool _isVoid, bool _isCdecl)
{
    dasm_State** Dst = &this->dasm;
    if (_isCdecl && _nArgs > 0)
    {
        //| add esp, 4*_nArgs
        dasm_put(Dst, 33, 4*_nArgs);
#line 57 "include/Arkitekt/Compiler.cpp"
    }
    //| pop edi
    //| pop esi
    //| pop ebx
    //| pop ecx
    dasm_put(Dst, 37);
#line 62 "include/Arkitekt/Compiler.cpp"
    if (_isVoid)
    {
        //| pop eax
        //| pop edx
        dasm_put(Dst, 42);
#line 66 "include/Arkitekt/Compiler.cpp"
    }
    //| mov esp, ebp
    //| pop ebp
    dasm_put(Dst, 45);
#line 69 "include/Arkitekt/Compiler.cpp"
    if (!_isCdecl && (_nArgs > 0))
        //| ret sizeof(uintptr_t) * _nArgs
        dasm_put(Dst, 50, sizeof(uintptr_t) * _nArgs);
#line 71 "include/Arkitekt/Compiler.cpp"
    else
        //| ret
        dasm_put(Dst, 53);
#line 73 "include/Arkitekt/Compiler.cpp"
}
void Arkitekt::Compiler::SaveStackFrame()
{
    dasm_State** Dst = &this->dasm;
    //| push ebp
    //| mov ebp, esp
    dasm_put(Dst, 16);
#line 79 "include/Arkitekt/Compiler.cpp"
}
void Arkitekt::Compiler::PopFrameAndRet(bool _isCdecl /* = false */, int _nArgs /* = 2 */)
{
    dasm_State** Dst = &this->dasm;
    //| mov esp, ebp
    //| pop ebp
    dasm_put(Dst, 45);
#line 85 "include/Arkitekt/Compiler.cpp"
    if (!_isCdecl && _nArgs > 0)
        //| ret sizeof(uintptr_t) * _nArgs
        dasm_put(Dst, 50, sizeof(uintptr_t) * _nArgs);
#line 87 "include/Arkitekt/Compiler.cpp"
    else
        //| ret
        dasm_put(Dst, 53);
#line 89 "include/Arkitekt/Compiler.cpp"
}
void Arkitekt::Compiler::CommonPrologue(bool _isVoid /* = true */)
{
    dasm_State** Dst = &this->dasm;
    if (_isVoid)
    {
        //| push eax
        //| push edx
        dasm_put(Dst, 55);
#line 97 "include/Arkitekt/Compiler.cpp"
    }
    //| push ecx
    //| push ebx
    //| push esi
    //| push edi
    dasm_put(Dst, 23);
#line 102 "include/Arkitekt/Compiler.cpp"
}
void Arkitekt::Compiler::CommonEpilogue(bool _isVoid /* = true */)
{
    dasm_State** Dst = &this->dasm;
    //| pop edi
    //| pop esi
    //| pop ebx
    //| pop ecx
    dasm_put(Dst, 37);
#line 110 "include/Arkitekt/Compiler.cpp"
    if (_isVoid)
    {
        //| pop edx
        //| pop eax
        dasm_put(Dst, 58);
#line 114 "include/Arkitekt/Compiler.cpp"
    }
}

uint32_t Arkitekt::Compiler::DefineLabel(const std::string_view& _name)
{
    
}

Arkitekt::FnBlock* Arkitekt::FnBlock::Add(const std::string_view& name, void* _mem, std::size_t _sz, void* _fn)
{
    ::new (&Arkitekt::Compiler::Get()->_blocks[CHashMapCalculateHash(name.data())]) FnBlock(_mem, _sz, _fn);
}

Arkitekt::Compiler* Arkitekt::Compiler::Get() {
    static Arkitekt::Compiler C;
    if (!C._blocks.size()) C.Init();
    return &C;
}

FnBlock* Arkitekt::Compiler::FinalizeFunction(const std::string_view& name, void**& _labels)
{
    const CHashMapHash _hash = CHashMapCalculateHash(name.data());
    size_t _outSz;
    dasm_link(&this->dasm, &_outSz);
    void *buf;
#ifdef _WIN32
    buf = VirtualAlloc(0, _outSz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    buf = mmap(0, _outSz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    dasm_encode(&this->dasm, buf);
#ifdef _WIN32
    { DWORD dwOld; VirtualProtect(buf, _outSz, PAGE_EXECUTE_READ, &dwOld); }
#else
    mprotect(buf, _outSz, PROT_READ | PROT_EXEC);
#endif
    return FnBlock::Add(name.data(), buf, _outSz, _labels[lbl__func]);
}
void Arkitekt::Compiler::Begin(void*** _labels)
{
    void *labels[lbl__MAX];
    dasm_init(&this->dasm, DASM_MAXSECTION);
    dasm_setupglobal(&this->dasm, labels, lbl__MAX);
    *_labels = labels;
    dasm_setup(&this->dasm, compiler_actions);
}

void Arkitekt::Compiler::End()
{
    dasm_free(&this->dasm);
}

bool Arkitekt::Compiler::Init()
{
    this->_blocks = std::unordered_map<CHashMapHash, FnBlock>();
    return true;
}
