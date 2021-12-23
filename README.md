<h1 align="center">DKUtil</h1>

Some utilitarian headers to help with SKSE64 plugin development

## Implementations
| [Config](#Config) - [GUI](#GUI) - [Hook](#Hook) - [Logger](#Logger) - [Utility](#Utility) |
:--:

## Consumption

### Requirement
+ [CMake](https://cmake.org)
+ [vcpkg](https://github.com/microsoft/vcpkg/releases)
+ std::c++20 or std::latest

### Installation
Clone a copy of `DKUtil` onto your local environment, in your target project's `CMakeLists.txt`, add
```CMake
add_subdirectory("Path/To/Local/Copy/Of/DKUtil" DKUtil)

target_link_libraries(
	"YOUR PROJECT NAME"
	INTERFACE
		DKUtil::DKUtil
)
```

## Config

An all purpose configuration library for SKSE plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

### Data
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

### Proxy
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

### Usage
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

## Hook

Some personal APIs to write hooks and memory patches with enable/disable control.    
+ Code cave hook
+ Virtual method table hook
+ (todo) Thunk function hook 
+ (todo) Import address table hook 

### API
```c++	
// Memory patch
Patch {
    const void* Data;
    const std::size_t Size;
}

// empty a code cave in the body of target function and branch to trampoline
// accepts a prolog patch before invoking payload and a epilog patch after returning from payload
inline auto AddCaveHook<std::ptrdiff_t OffsetLow, std::ptrdiff_t OffsetHigh>(
    const std::uintptr_t a_src, // target function address
    const FuncInfo a_func, // helper struct describing function info
    const Patch* a_prolog = nullptr,
    const Patch* a_epilog = nullptr
) noexcept // return std::unique_ptr<CaveHookHandle>

// swaps a virtual method table address with target function address
// accepts a prolog patch before invoking payload
inline auto AddVMTHook(
    void* a_vtbl, // pointer to class/vtbl
    const FuncInfo a_func, // helper struct describing function info
    const std::uint16_t a_index = 0, // index of target function in VMT
    const Patch* a_prolog = nullptr
) noexcept // return std::unique_ptr<VMTHookHandle>
```

### Cave hook example
```C++
// payload function
float __cdecl Hook_MyAwesomeFunc(int a_awesomeInt) {
    // awesome stuff
    return static_cast<float>(a_awesomeInt);
}

constexpr std::uintptr_t FuncAddr = 0x7ff712345678; // target function address
constexpr std::uintptr_t OffsetLow = 0x120; // offset start from target function address
constexpr std::uintptr_t OffsetHigh = 0x130; // offset end from target function address
// OffsetHigh must be greater than OffsetLow by 0x5 bytes

constexpr DKUtil::Hook::Patch Epilog = {
    "\x0F\x10\xD8", // movups xmm3, xmm0
    0x3 // size of patch
};

void Install() 
{
    using namespace DKUtil::Alias;
    HookHandle _Hook_MAF; // hook handle to Hook_MyAwesomeFunc()


    static std::once_flag HookInit;
    std::call_once(HookInit, [&]()
        {
            _Hook_MAF = DKUtil::Hook::AddCaveHook<OffsetLow, OffsetHigh>(FuncAddr, FUNC_INFO(Hook_MyAwesomeFunc), nullptr, &Epilog);
        }
    );


    _Hook_MAF->Enable();

    INFO("Hooks installed"sv);
}
```
In this example we simply wrote a detour from address `0x7ff712345678` with offset `0x120`, to our own `Hook_MyAwesomeFunc` function, and return to address `0x7ff712345678` with offset `0x130`. This detour uses `rcx` as first parameter, and returns the float through `xmm0`, so we also patched the function epilog to move the return value to our desired register `xmm3`.

### VMT hook example
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
void MsgC(Dummy*) { INFO("Called MsgC"sv); }


void Install()
{
    auto _Hook_MsgA = DKUtil::Hook::AddVMTHook(dummy, FUNC_INFO(MsgC), 0);
    auto _Hook_MsgB = DKUtil::Hook::AddVMTHook(dummy, FUNC_INFO(MsgC), 1);

    _Hook_MsgA->Enable();
    _Hook_MsgB->Enable();

    // you may still invoke original function
    oldMsgA = reinterpret_cast<MsgFunc>(_Hook_MsgA->OldAddress);
    oldMsgB = reinterpret_cast<MsgFunc>(_Hook_MsgB->OldAddress);

    oldMsgA(dummy);
    oldMsgB(dummy);
}
```
In this example we swapped the addresses of `MsgA` and `MsgB` in virtual method table of class `Dummy` with our custom function `MsgC`.


## Logger

Some SKSE style macro loggers with `spdlog` backend
```C++
INFO("{} {} {} {}", a, b, c, d);
DEBUG("Important debug info that shows on debug build"sv);
ERROR("This will abort the process!"sv);
Dump(container, fmt_callback);
```

## Utility

+ FNV-1A compile time string hashing with both 32bit/64bit implementation.
+ `lvalue_cast`
+ Singleton data model abstract class to save boiler plater code.
+ Some random helper functions.