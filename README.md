# DKUtil
An utility header-only library for SKSE plugin development.
---
---
## Consumption
Clone a copy of `DKUtil` onto your local environment, in your plugin project's `CMakeLists.txt`, add
```CMake
add_subdirectory("Path/To/Local/Copy/Of/DKUtil" DKUtil)

target_link_libraries(
	"YOUR PROJECT NAME"
	INTERFACE
		DKUtil::DKUtil
)
```
---
## Implementations:
+ Hash
+ DKHook
+ Template

### Hash
Based on FNV-1 and FNV-1A constexpr string hashing implementation.

### Hook
Implements both SKSE64 and CommonLibSSE method to write hooks in x64 with ease. (With / Without address library)

#### Demonstrations:
```c++
// uses address library
template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
bool BranchToFunction(
    std::uintptr_t a_hookFunc,          // address of our own function to hook to
    const void* a_prePatch = nullptr,   // patch codes that execute before calling our hook function
    const std::uint64_t a_preSize = 0,  // size of pre patch code
    const void* a_postPatch = nullptr,  // patch codes that execute after returning from out hook function
    const std::uint64_t a_postSize = 0  // size of post patch code
); // return bool indicating success


// uses address library, packed parameters
template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
bool BranchToFunction(
    BranchInstruction a_instruction     // packed parameter to reduce parameter list length
); // return bool indicating success


// uses base address
template <std::uintptr_t ADDRESS_START, std::uintptr_t ADDRESS_END>
bool BranchToFunction(
    std::uintptr_t a_hookFunc,          // address of our own function to hook to
    const void* a_prePatch = nullptr,   // patch codes that execute before calling our hook function
    const std::uint64_t a_preSize = 0,  // size of pre patch code
    const void* a_postPatch = nullptr,  // patch codes that execute after returning from out hook function
    const std::uint64_t a_postSize = 0  // size of post patch code
); // return bool indicating success


// uses base address, packed parameters
template <std::uintptr_t ADDRESS_START, std::uintptr_t ADDRESS_END>
bool BranchToFunction(
    BranchInstruction a_instruction     // packed parameter to reduce parameter list length
); // return bool indicating success


// packed parameter
struct BranchInstruction
{
	struct PatchCode
	{
		const void* Data = nullptr;
		std::uint64_t Size = 0;
	};

	std::uintptr_t BranchTarget = 0x0;
	PatchCode PrePatch;
	PatchCode PostPatch;
};

typedef BranchInstruction DKUtil::Hook::BranchInstruction;
```

#### SKSE64 with address ID example:
```C++
#define SKSE64 // define this macro to use SKSE64 implementation explicitly
#include "DKUtil/Hook.h"


void __fastcall Hook_MyAwesomeFunc();           // our hook function


constexpr std::uint64_t FUNC_ID = 12345;        // id of address in bin file
constexpr std::uintptr_t OFFSET_START = 0x120;  // offset start from the FUNC_ID base address
constexpr std::uintptr_t OFFSET_END = 0x130;    // offset end from FUNC_ID base address
// from OFFSET_START to OFFSET_END will be our code cave, must be bigger than 0x5


bool InstallHooks() 
{
    auto success = true;

    success &= DKUtil::Hook::BranchToFunction<FUNC_ID, OFFSET_START, OFFSET_END>(
        reinterpret_cast<std::uintptr_t>(&Hook_MyAwesomeFunc)
    );

    return success;
}
```
In this example we simply wrote a detour from address that corresponds to id `12345` with offset `OFFSET_START`, to our own `Hook_MyAwesomeFunc` function.

#### SKSE64 with address ID + patch code exmaple:
```C++
#define SKSE64 // define this macro to use SKSE64 implementation explicitly
#include "DKHook.h"


int __fastcall Hook_MyAwesomeFunc(int); // our hook function, takes 1 parameter


constexpr std::uint64_t FUNC_ID = 12345;
constexpr std::uintptr_t OFFSET_START = 0x120;
constexpr std::uintptr_t OFFSET_END = 0x130;


bool InstallHooks() 
{
    auto success = true;

    success &= DKUtil::Hook::BranchToFunction<FUNC_ID, OFFSET_START, OFFSET_END>(
        reinterpret_cast<std::uintptr_t>(&Hook_MyAwesomeFunc),
        "\x48\x8B\x0D\x30\x00\x00\x00", // mov rcx, qword ptr [ rip + 0x30 ]
        7,                              // size of above code
        "\x48\x89\xC1",                 // mov rcx, rax
        3                               // size of above code
    );

    return success;
}
```
> You can use [Hex2Asm](https://defuse.ca/online-x86-assembler.htm) to generate fixed assembly code.
> Or use xbyak for runtime compiled assembly.

#### SKSE64 implementation without address ID + patch code example:
```C++
#define SKSE64 // define this macro to use SKSE64 implementation explicitly
#include "DKHook.h"


int __fastcall Hook_MyAwesomeFunc(int); // our hook function, takes 1 parameter


constexpr std::uintptr_t ADDRESS_START = 0x00007FF6514767DF;
constexpr std::uintptr_t ADDRESS_END = 0x120;


bool InstallHooks() 
{
    auto success = true;

    success &= DKUtil::Hook::BranchToFunction<ADDRESS_START, ADDRESS_END>(
        reinterpret_cast<std::uintptr_t>(&Hook_MyAwesomeFunc),
        "\x48\x8B\x0D\x30\x00\x00\x00", // mov rcx, qword ptr [ rip + 0x30 ]
        7,                              // size of above code
        "\x48\x89\xC1",                 // mov rcx, rax
        3                               // size of above code
    );

    return success;
}
```
This is mostly the same with address library example, but we have to resolve our addresses before calling BranchToFunction.

Beacuse CommonLib internally utilizes address library, so CommonLib examples will be the same as SKSE64 w/ Address ID w/ Patch code examples, except that we don't `#define SKSE64` before `#include "DKUtil/Hook.h"`.

#### Packed parameter example:
```C++
#include "DKHook.h"

int __fastcall Hook_MyAwesomeFunc(int); // our hook function, takes 1 parameter

constexpr std::uint64_t FUNC_ID = 12345;
constexpr std::uintptr_t OFFSET_START = 0x120;
constexpr std::uintptr_t OFFSET_END = 0x130;

constexpr BranchInstruction FUNC_INSTRUCTION = 
{
    reinterpret_cast<std::uintptr_t>(&Hook_MyAwesomeFunc),
    {   // pre patch code
        "\x48\x8B\x0D\x30\x00\x00\x00", // mov rcx, qword ptr [ rip + 0x30 ]
        7                               // size of above code
    }, 
    {   // post patch code
        "\x48\x89\xC1", // mov rcx, rax
        3               // size of above code
    }
};

bool InstallHooks() 
{
    auto success = true;

    success &= DKUtil::Hook::BranchToFunction<FUNC_ID, OFFSET_START, OFFSET_END>(FUNC_INSTRUCTION);

    return success;
}
```
Use packed parameter to reduce the length of parameter list.

> SKSE64 and CommonLibSSE implementation are via conditional compilation, so `DKHOOK_SKSE64` must be defined before `#include "DKHook.h"` to use SKSE64 implementation explicitly. Othersie it will use CommonLibSSE implementation by default.
