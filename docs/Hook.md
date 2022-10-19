<h1 align="center">DKUtil::Hook</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Hook Source](/include/DKUtil/Hook.hpp)

Some personal APIs to write hooks and memory patches with enable/disable control.    
+ Assembly memory patch
+ Function cave hook
+ Virtual method table hook
+ Import address table hook 
+ For NG-integration, scroll below

## Forewords
Since this library is mainly used to help with SKSE64 plugin developments, and to clarify the usage between `DKUtil::Hook` and `CommonLibSSE::REL`:  
They are designed for different use scenarios. To just replace a game `call/jmp subroutine` instruction with a `call/jmp` to designated hook function, by all means, use the `trampoline::write_call/write_branch`. To weave a function call in assembly that do not have `call/jmp` type of instructions, then use `DKUtil::Hook`.  

## API

### Memory editing
`DKUtil::Hook` offers some memory writing methods like other library does.
```C++
// write raw data with size
void WriteData(std::uintptr_t& a_dst, const void* a_data, const std::size_t a_size, bool a_forwardPtr = false, bool a_requestAlloc = false) noexcept;
// write immediate value
void WriteImm(std::uintptr_t& a_dst, const dku_h_pod_t auto& a_data, bool a_forwardPtr = false, bool a_requestAlloc = false) noexcept;
// write packed memory structure
void WritePatch(std::uintptr_t& a_dst, const Patch* a_patch, bool a_forwardPtr = false, bool a_requestAlloc = false) noexcept;
```
`forwardPtr` parameter will adjust the destination address by size of memory written, default is `false`. 
`requestAlloc` parameter will request allocation from trampoline, this is meant to increase the internal trampoline pointer of next operable assembly space.

### HookHandle
`HookHandle` class is the result object of a succesful hook operation. `HookHandle` is used to access the hook related data and enable/disable the hook that created it.
```C++
HookHandle handle;

// hook control
handle->Enable();
handle->Disable();
```

---
### Memory Structure
`DKUtil::Hook` supports a few ways of passing memory patches/data to the hook. Using example of `mov rcx, qword ptr [rsp+0x20]`:
```C++
struct XbyakPatch : Xbyak::CodeGenerator
{
    XbyakPatch
    {
        mov(rcx, rsp[0x20]);
    }
};

DKUtil::Patch DKUPatch {
    "\x48\x8B\x4C\x24\x20"; // data pointer
    5, // size
};

std::array<std::uint8_t, 5> RawPatch{ 0x48, 0x8B, 0x4C, 0x24, 0x20 };
```
The `DKUtil::Hook` functions are overloaded to accept above memory structure pointers in place of `Patch*`.

---
### ASM Patch
Apply assembly patch in the body of execution
- `address` : memory address of the **BEGINNING** of target function
- `offsets` : pair containing the offsets of the begining and the end of operable assembly
- `patch` : pointer to the memory patch data structure, it ultimately unpacks into `std::pair<const void*, std::size_t>`
- `forward` : bool value indicating skipping the rest of `NOP` space
```C++
/* API */
ASMPatchHandle AddASMPatch(
    std::uintptr_t a_address,
    std::pair<std::ptrdiff_t, std::ptrdiff_t> a_offset,
    std::pair<const void*, std::size_t> a_patch,
    bool a_forward = true
) noexcept

/* example */

using namespace DKUtil::Alias;

const auto funcAddr = REL::Relocation<std::uintptr_t>(REL::RelocationID(SE_FuncID, AE_FuncID)).address();
// or 
const auto funcAddr = DKUtil::Hook::IDToAbs(AE_FuncID, SE_FuncID); // DKUtil uses alphabetical order so AE ID goes first
// or
constexpr std::uintptr_t funcAddr = 0x7FF712345678;

constexpr std::uintptr_t OffsetL = 0x120;
constexpr std::uintptr_t OffsetH = 0x130;

constexpr OpCode AsmSrc[]{
    0xB8,					// mov eax,
    0x00, 0x00, 0x00, 0x00, // Imm32
    0x89, 0XC1,				// mov ecx, eax
};

HookHandle _Hook_ASM; // can be defined out of scope

_Hook_UES = DKUtil::Hook::AddASMPatch(funcAddr, { OffsetL, OffsetH }, { &AsmPatch, sizeof(AsmSrc) }); // using in-place raw data
// various ways of calling
_Hook_UES = DKUtil::Hook::AddASMPatch(funcAddr, { OffsetL, OffsetH }, &DKUPatch); // using wrapper
_Hook_UES = DKUtil::Hook::AddASMPatch(funcAddr, { OffsetL, OffsetH }, &XbyakPatch); // using xbyak
_Hook_UES = DKUtil::Hook::AddASMPatch(funcAddr, { OffsetL, OffsetH }, { RawPatch.data(), RawPatch.size() }); // using raw data

_Hook_UES->Enable();
```
If the given assembly space defined by `offsets` is less than the size of assembly patch, a trampoline will be utilized to fulfill the space, this action requires a minimal assembly space of `0x5`.  
The bool paramter `forward` indicates whether to skip the rest of `NOP` after applying the patch.

---
### Cave Hook
Branch to hook function in the body of execution from target function.
* `address` : memory address of the **BEGINNING** of target function
* `offsets` : pair containing the offsets of the begining and the end of operable assembly
* `funcInfo` : FUNC_INFO or RT_INFO wrapper of hook function
* `prolog` : memory patch **before** detouring to hook function
* `epilog` : memory patch **after** returning from hook function
* `flag` : specifies special operation on cave hook  
```C++
/* API */
enum class HookFlag : std::uint32_t
{
    kNoFlag,

    kSkipNOP,				// skip NOPs
    kRestoreBeforeProlog,	// apply stolens before prolog
    kRestoreAfterProlog,	// apply stolens after prolog
    kRestoreBeforeEpilog,	// apply stolens before epilog
    kRestoreAfterEpilog,	// apply stolens after epilog
};

CaveHookHandle AddCaveHook(
    std::uintptr_t a_address,
    std::pair<std::ptrdiff_t, std::ptrdiff_t> a_offset,
    FuncInfo a_funcInfo,
    std::pair<const void*, std::size_t> a_prolog,
    std::pair<const void*, std::size_t> a_epilog,
    DKUtil::model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP
) noexcept

/* example */
using namespace DKUtil::Alias;

// hook function
float __cdecl Hook_MyAwesomeFunc(int a_awesomeInt) {
    // do awesome stuff
    return static_cast<float>(a_awesomeInt);
}

const auto funcAddr = REL::Relocation<std::uintptr_t>(REL::RelocationID(SE_FuncID, AE_FuncID)).address();
// or 
const auto funcAddr = DKUtil::Hook::IDToAbs(AE_FuncID, SE_FuncID); // DKUtil uses alphabetical order so AE ID goes first
// or
constexpr std::uintptr_t funcAddr = 0x7FF712345678;

constexpr std::uintptr_t OffsetL = 0x120;
constexpr std::uintptr_t OffsetH = 0x130;

constexpr DKUtil::Hook::Patch Epilog = {
    "\x0F\x10\xD8", // movups xmm3, xmm0
    0x3 // size of patch
};

HookHandle _Hook_MAF; // can be defined out of scope

_Hook_MAF = DKUtil::Hook::AddCaveHook(funcAddr, { OffsetL, OffsetH }, FUNC_INFO(Hook_MyAwesomeFunc), nullptr, &Epilog);// various ways of calling
_Hook_MAF = DKUtil::Hook::AddCaveHook(funcAddr, { OffsetL, OffsetH }, FUNC_INFO(Hook_MyAwesomeFunc)); // same as trampoline.write_call<5>
_Hook_MAF = DKUtil::Hook::AddCaveHook(funcAddr, { OffsetL, OffsetH }, FUNC_INFO(Hook_MyAwesomeFunc), nullptr, &Epilog); // epilog only
_Hook_MAF = DKUtil::Hook::AddCaveHook(funcAddr, { OffsetL, OffsetH }, FUNC_INFO(Hook_MyAwesomeFunc), &Prolog, nullptr); // prolog only
_Hook_MAF = DKUtil::Hook::AddCaveHook(funcAddr, { OffsetL, OffsetH }, FUNC_INFO(Hook_MyAwesomeFunc), &Prolog, &Epilog, { HookFlag::kSkipNOP, HookFlag::kRestoreBeforeProlog }); // prolog & epilog, and apply stolen bytes before applying prolog

_Hook_MAF->Enable();
```
NOTE: If manually relocating fifth argument or more for hook function, use `mov [rsp-0x8*(argc-4)], [...]` instead of `push` because the stack space is pre-allocated within `DKUtil::Hook`.

---
### Virtual method table swap
Swaps a virtual method table function with target function
* `vtbl` : pointer to virtual method table (base address of class object)
* `index` : index of the virtual function in the virtual method table
* `funcInfo` : FUNC_INFO or RT_INFO wrapped function
* `patch` : prolog patch before detouring to target function
```C++
/* API */
VMTHookHandle AddVMTHook(
    void* a_vtbl,
    std::uint16_t a_index,
    FuncInfo a_funcInfo,
    std::pair<const void*, std::size_t> a_patch
) noexcept

/* example */
using namespace DKUtil::Alias;

class Dummy
{
public:
    void MsgA() { INFO("Called MsgA"sv); } // 0
    void MsgB() { INFO("Called MsgB"sv); } // 1
};

// target dummy vtbl
Dummy* dummy = new Dummy();

// target function signature
using MsgFunc = std::add_pointer_t<void(Dummy*)>;
MsgFunc oldMsgA;
MsgFunc oldMsgB;
// or
REL::Relocation<decltype(Dummy::MsgA)> oldMsgA;
REL::Relocation<decltype(Dummy::MsgB)> oldMsgB;

// swap function
void MsgC(Dummy* a_this) { 
    INFO("Called MsgC"sv);

    // call original function
    oldMsgA(a_this);
    oldMsgB(a_this);
}

auto _Hook_MsgA = DKUtil::Hook::AddVMTHook(dummy, 0, FUNC_INFO(MsgC));
auto _Hook_MsgB = DKUtil::Hook::AddVMTHook(dummy, 1, FUNC_INFO(MsgC));
// save original function
oldMsgA = reinterpret_cast<MsgFunc>(_Hook_MsgA->OldAddress);
oldMsgB = reinterpret_cast<MsgFunc>(_Hook_MsgB->OldAddress);

_Hook_MsgA->Enable();
_Hook_MsgB->Enable();
```

---
### Import address table swap
Swaps a import address table method with target function
* `moduleName` : name of the target module that import address table resides
* `methodName` : name of the target method to be swapped
* `funcInfo` : FUNC_INFO or RT_INFO wrapped function
* `patch` : prolog patch before detouring to target function
```C++
/* API */
inline auto AddIATHook(
    const char* a_moduleName,
    const char* a_methodName,
    const FuncInfo a_funcInfo,
    std::pair<const void*, std::size_t> a_patch
) 

/* example */
using namespace DKUtil::Alias;

constexpr const char* ModuleName = "kernel32.dll";
// or 
constexpr const char* ModuleName = GetProcessName(0); // Or optional HMODULE of target module
constexpr const char* MethodName = "Sleep";

// target function signature
using SleepFunc = std::add_pointer_t<void(WINAPI)(DWORD)>;
SleepFunc oldSleep;

// swap function
void WINAPI MySleep(DWORD dwMiliseconds) {
    DEBUG("Hooked Sleep Function Called!"av);
    DEBUG("Sleeping for: {}", dwMiliseconds);
 
    // call original function
    oldSleep(dwMiliseconds);
}

auto _Hook_Sleep = DKUtil::Hook::AddIATHook(ModuleName, MethodName, FUNC_INFO(MsgC));
// save original function
oldSleep = reinterpret_cast<SleepFunc>(_Hook_Sleep->OldAddress);

_Hook_Sleep->Enable();
```

## NG integration
To complement CommonLibSSE-NG's runtime multitargeting feature that assigns different address depends on the current runtime, `DKUtil::Hook` also offers runtime counterparts of:  
```C++
DKUtil::ID2Abs(std::uintptr_t AE_ID, std::uintptr_t SE_ID, std::uintptr_t Optional_VR_ID); // runtime address based on ID
DKUtil::RuntimeOffset(
    std::pair<std::ptrdiff_t, std::ptrdiff_t> AE_Offset_Low_High, 
    std::pair<std::ptrdiff_t, std::ptrdiff_t> SE_Offset_Low_High,
    std::pair<std::ptrdiff_t, std::ptrdiff_t> Optional_VR_Offset_Low_High); // runtime offset pairs
DKUtil::RuntimePatch(DKUtil::Patch* AE_Patch, DKUtil::Patch* SE_Patch, DKUtil::Patch* Optional_VR_Patch); // runtime memory patches
// Patch* is overloaded with Xbyak::CodeGenerator* and std::pair<const void*, std::size_t>
```

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
