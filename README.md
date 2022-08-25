<h1 align="center">DKUtil</h1>

Some utilitarian headers to help with SKSE64 plugin development

# Implementations
[![Config](https://img.shields.io/badge/Config-1.0.1-R.svg)](#Config)
[![Hook](https://img.shields.io/badge/Hook-2.3.2-R.svg)](#Hook)
[![Logger](https://img.shields.io/badge/Logger-1.1.0-R.svg)](#Logger)
[![Utility](https://img.shields.io/badge/Utility-untracked-R.svg)](#Utility)

# Consumption

## Requirement
+ [CMake](https://cmake.org)
+ [vcpkg](https://github.com/microsoft/vcpkg/releases)
+ `/std:c++20` or `/std:latest`
    + Config
        + [tomlplusplus](https://github.com/marzer/tomlplusplus)
        + [SimpleIni](https://github.com/brofield/simpleini)
        + [nlohmann-json](https://github.com/nlohmann/json)
    + Logger
        + [spdlog](https://github.com/gabime/spdlog)

> All dependencies should be handled by vcpkg.


## Installation
Clone a copy of `DKUtil` onto your local environment, in your target project's `CMakeLists.txt`, add:  
```CMake
add_subdirectory("Path/To/Local/Copy/Of/DKUtil" DKUtil)

target_link_libraries(
	"YOUR PROJECT NAME"
	INTERFACE
		DKUtil::DKUtil
)
```

# Config

An all purpose configuration library for SKSE plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

## Data
`DKUtil::Config` supports 3 primitive data types and string variants. All types except `Boolean` are collection enabled, to mimic the array-like behaviors if they were one.  

The data can be accessed by prepending the asterisk symbol `*` as if it were a pointer type, or the getter method `get_data()`.  
If the data is initialized as a collection, the pointer to its members can be accessed by `[]` operator with proper index. It will return the first element if the data itself is not a collection or if the index is out of bound.
```C++
#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias; // type alias

// only initialize with key field, section field is optional and will not stop DKUtil from accessing data
Integer myInt64Data{ "MyIntDataKey", "OptionalSection" }; // std::int64_t
Double myDoubleData{ "MyDoubleKey" }; // double
Boolean myBoolData{ "MyBoolKey" }; // bool
String myStringData{ "MyStringKey" }; // std::basic_string<char>
```

## Proxy
The configuration system in DKUtil works as "proxy per file", each `Proxy` is responsible for one specific configuration file and the subsequent `Load` and `Write` calls.

`Proxy` can be compile time and/or runtime determined for the given file. If the file name and type is already known at the compile time, use `COMPILE_PROXY` macro to get the corresponding parser for such file type. If the file name is runtime generated, use `RUNTIME_PROXY` macro for file type/name that is unknown at the compile time.  
```C++
#include "DKUtil/Config.hpp"

// it can be .ini, .json or .toml
auto MainConfig = COMPILE_PROXY("MyConfigFile.json"sv); // will be evaluated to json proxy at compile time

std::string FileHead = "Mysterious";
std::string FileTrail = "File";
std::string FileType = ".toml";

std::string runtimeFile = FileHead + FileTrail + FileType;
auto RuntimeConfig = RUNTIME_PROXY(runtimeFile); // will be evaluated to toml proxy at runtime
```

## Usage
The data can be declared with/without `static` keyword and initialized with the proper key name and/or optional section name, you may `Bind` the data to a `Proxy` with default value set for that data.  

File `MyConfigFile.toml`:
```TOML
[RandomSectionNameButNeedsToPresent]
MyIntDataKey = 10086
MyIntArrayKey = [99, 96, 92, 87, 71]
MyDoubleKey = 114.514
MyBoolKey = true
MyStringKey = "Hello toml"
MyStringArrayKey = [
    "Frust",
    "Sedoncst",
    "LastButNotLeast"
]
```
Code:  
```C++
#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias; // For type alias

// only initialize with key field and/or optional section field
Integer myInt64Data{ "MyIntDataKey" };
Integer myInt64ArrayData{ "MyIntArrayKey" };
Double myDoubleData{ "MyDoubleKey" };
Boolean myBoolData{ "MyBoolKey" };
String myStringData{ "MyStringKey" };
String myStringArrayData{ "MyStringArrayKey" };

// can be local variable and/or non-static
static auto MainConfig = COMPILE_PROXY("MyConfigFile.toml"sv);

// your custom config load call
void Load() noexcept 
{
    // bind configs to file with default values
    MainConfig.Bind(myInt64Data, 10); // std::int64_t 10
    MainConfig.Bind(myInt64ArrayData, 10, 78, 314, 996); // std::int64_t array
    MainConfig.Bind(myStringData, "Hello DKUtil"); // string
    MainConfig.Bind(myStringArrayData, "First", "Second", "Third"); // string array
    MainConfig.Bind(myBoolData, true); // bool
    MainConfig.Bind(myDoubleData, 3.14154); // double

    MainConfig.Load();

    DEBUG("{} {} {}", *myInt64Data, *myStringData, *myDoubleData); // 10086 hello toml 114.514

    // traverse the array
    for (auto& place : myStringArrayData.get_collection()) {
        DEBUG(place);
    } // Frust Sedoncst LastButNotLeast


    std::string FileHead = "Mysterious";
    std::string FileTrail = "File";
    std::string FileType = ".toml";

    std::string runtimeFile = FileHead + FileTrail + FileType;
    auto RuntimeConfig = RUNTIME_PROXY(runtimeFile); // will be evaluated to toml proxy at runtime
}
```

# Hook

Some personal APIs to write hooks and memory patches with enable/disable control.    
+ Assembly memory patch
+ Function cave hook
+ Virtual method table hook
+ Import address table hook 
+ (todo) Thunk function hook 

## API
```C++	
// Memory patch
Patch {
    const void* Data;
    const std::size_t Size;
}


/* Apply assembly patch in the body of execution
 * @param OffsetLow : Beginning offset of the cave
 * @param OffsetHigh : Ending offset of the cave
 * @param a_address : Address of the beginning at target body of execution
 * @param a_patch : Assembly patch
 * @returns ASMPatchHandle
 */
template <const std::ptrdiff_t OffsetLow, const std::ptrdiff_t OffsetHigh>
    requires (OffsetHigh > OffsetLow)
inline auto AddASMPatch(
    const std::uintptr_t a_address,
    const Patch* a_patch,
    const bool a_forward = true
)


/* Branch to target function in the body of execution.
 * If stack manipulation is involved, add stack offset (sizeof(std::uintptr_t) * (number of target function's arguments))
 * @param OffsetLow : Beginning offset of the cave
 * @param OffsetHigh : Ending offset of the cave
 * @param a_address : Address of the beginning at target body of execution
 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
 * @param a_prolog : Prolog patch before detouring to target function
 * @param a_epilog : Epilog patch after returning from target function
 * @returns CaveHookHandle
 */
inline auto AddCaveHook<std::ptrdiff_t OffsetLow, std::ptrdiff_t OffsetHigh>(
    const std::uintptr_t a_address,
    const FuncInfo a_func,
    const Patch* a_prolog = nullptr,
    const Patch* a_epilog = nullptr
)


/* Swaps a virtual method table function with target function
 * @param a_vtbl : Pointer to virtual method table
 * @param a_index : Index of the virtual function in the virtual method table
 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
 * @param a_prolog : Prolog patch before detouring to target function
 * @return VMTHookHandle
 */
inline auto AddVMTHook(
    void* a_vtbl,
    const std::uint16_t a_index,
    const FuncInfo a_func,
    const Patch* a_prolog = nullptr
)


/* Swaps a import address table method with target function
 * @param a_moduleName : Name of the target module that import address table resides
 * @param a_methodName : Name of the target method to be swapped
 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
 * @param a_prolog : Prolog patch before detouring to target function
 * @return VMTHookHandle
 */
inline auto AddIATHook(
    const char* a_moduleName,
    const char* a_methodName,
    const FuncInfo a_funcInfo,
    const Patch* a_prolog = nullptr
) 
```

## ASM patch example
```C++
constexpr std::uint64_t TargetAddr = 0x7ff712345678;
constexpr std::uintptr_t OffsetLow = 0x120;
constexpr std::uintptr_t OffsetHigh = 0x130;

constexpr OpCode AsmSrc[]{
    0xB8,					// mov eax,
    0x00, 0x00, 0x00, 0x00, // Imm32
    0x89, 0XC1,				// mov ecx, eax
};


void Install()
{
    using namespace DKUtil::Alias;
    HookHandle _Hook_ASM;

    static std::once_flag HookInit;
    std::call_once(HookInit, [&]()
        {
            constexpr Patch AsmPatch = {
                std::addressof(AsmSrc),
                sizeof(AsmSrc)
            };

            _Hook_UES = DKUtil::Hook::AddASMPatch<OffsetLow, OffsetHigh>(TargetAddr, &AsmPatch);
        }
    );

    _Hook_UES->Enable();
}
```
In this example the assembly from memory address `0x7ff712345678 + 0x120` to `0x7ff712345678 + 0x130` will be replaced with the contents in `AsmSrc`. The rest of the empty memory within the range will be filled with `NOP`. If this empty size if greater than half of the entire memory cave, a relative forward jmp instruction will be appended to skip the rest of `NOP`s.


## Cave hook example
```C++
// payload function
float __cdecl Hook_MyAwesomeFunc(int a_awesomeInt) {
    // awesome stuff
    return static_cast<float>(a_awesomeInt);
}

constexpr std::uintptr_t TargetAddr = 0x7ff712345678;
constexpr std::uintptr_t OffsetLow = 0x120;
constexpr std::uintptr_t OffsetHigh = 0x130;

constexpr DKUtil::Hook::Patch Epilog = {
    "\x0F\x10\xD8", // movups xmm3, xmm0
    0x3 // size of patch
};


void Install() 
{
    using namespace DKUtil::Alias;
    HookHandle _Hook_MAF;

    static std::once_flag HookInit;
    std::call_once(HookInit, [&]()
        {
            _Hook_MAF = DKUtil::Hook::AddCaveHook<OffsetLow, OffsetHigh>(TargetAddr, FUNC_INFO(Hook_MyAwesomeFunc), nullptr, &Epilog);
        }
    );

    _Hook_MAF->Enable();
}
```
In this example a detour from memory address `0x7ff712345678 + 0x120` to our own `Hook_MyAwesomeFunc` function will be applied, and return to memory address `0x7ff712345678 + 0x130`. This detour uses `rcx` as first parameter, and returns the float through `xmm0`, Function epilog is also patched to move the return value to desired register `xmm3`.

## VMT hook example
```C++
class Dummy
{
public:
    void MsgA() { INFO("Called MsgA"sv); } // index 0
    void MsgB() { INFO("Called MsgB"sv); } // index 1
};

// target vtbl
Dummy* dummy = new Dummy();

// target function sig
using MsgFunc = std::add_pointer_t<void(Dummy*)>;
MsgFunc oldMsgA;
MsgFunc oldMsgB;

// swap function
void MsgC(Dummy* a_this) { 
    INFO("Called MsgC"sv);

    // call original function
    oldMsgA(a_this);
    oldMsgB(a_this);
}


void Install()
{
    using namespace DKUtil::Alias;
    HookHandle _Hook_MsgA;
    HookHandle _Hook_MsgB;

    static std::once_flag HookInit;
    std::call_once(HookInit, [&]()
        {
            _Hook_MsgA = DKUtil::Hook::AddVMTHook(dummy, 0, FUNC_INFO(MsgC));
            _Hook_MsgB = DKUtil::Hook::AddVMTHook(dummy, 1, FUNC_INFO(MsgC));
        }
    );

    // save original function
    oldMsgA = reinterpret_cast<MsgFunc>(_Hook_MsgA->As<VMTHandle>()->OldAddress);
    oldMsgB = reinterpret_cast<MsgFunc>(_Hook_MsgB->As<VMTHandle>()->OldAddress);

    _Hook_MsgA->Enable();
    _Hook_MsgB->Enable();
}
```
In this example the memory addresses of `MsgA` and `MsgB` in virtual method table of class `Dummy` will be replaced with our custom function `MsgC`.

## IAT hook example
```C++
constexpr const char* ModuleName = "kernel32.dll"; // nullptr = host process
constexpr const char* MethodName = "Sleep";

// target function sig
using SleepFunc = std::add_pointer_t<void(WINAPI)(DWORD)>;
SleepFunc oldSleep;

// swap function
void WINAPI MySleep(DWORD dwMiliseconds) {
    DEBUG("Hooked Sleep Function Called!"av);
    DEBUG("Sleeping for: {}", dwMiliseconds);
 
    // call original function
    oldSleep(dwMiliseconds);
}

void Install()
{
    using namespace DKUtil::Alias;
    HookHandle _Hook_Sleep;

    static std::once_flag HookInit;
    std::call_once(HookInit, [&]()
        {
            _Hook_Sleep = DKUtil::Hook::AddIATHook(ModuleName, MethodName, FUNC_INFO(MsgC));
        }
    );

    // save original function
    oldSleep = reinterpret_cast<SleepFunc>(_Hook_Sleep->As<IATHandle>()->OldAddress);

    _Hook_Sleep->Enable();
}
```
In this example the address of function `Sleep` that resides in the import header of the process `kernel32.dll` will be replaced with our custom function `MySleep`.


# Logger

Some SKSE style macro loggers with `spdlog` backend.
```C++
INFO("{} {} {} {}", a, b, c, d);
DEBUG("Important debug info that shows on debug build"sv);
ERROR("This will abort the process!");
```

# Utility

Some helper functions used within other DKUtil headers.
+ IPC
    + Inter process communication implemented using custom window messages.
+ function
    + `consteval` helper functions retrieving the argument count of a function.
+ model
    + Singleton data model abstract class to save boiler plater code.
+ numbers
    + FNV-1A compile time string hashing with both 32bit/64bit implementation.
+ string
    + `to_wstring` method
    + `concat` compile time string concatenation.