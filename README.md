<h1 align="center">DKUtil</h1>
Some utilitarian headers to help with SKSE64 plugin development

---
<h2 align="center">Consumption</h2>

Clone a copy of `DKUtil` onto your local environment, in your target project's `CMakeLists.txt`, add
```CMake
add_subdirectory("Path/To/Local/Copy/Of/DKUtil" DKUtil)

target_link_libraries(
	"YOUR PROJECT NAME"
	INTERFACE
		DKUtil::DKUtil
)
```

---
<h2 align="center">Implementations</h2>

+ [Config](#Config)
+ [Hook](#Hook)
+ [Logger (spdlog backend)](#Logger)
+ [Utility](#Utility)

---
# Config

An all purpose configuration library for SKSE plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

## Data

`DKUtil::Config` supports 3 primitive data types and string variants. All types except `Boolean` are collection enabled, to mimic the array-like behaviors if they were one.  

The data can be accessed by prepending the asterisk symbol `*` as if it were a pointer type, or the getter method `get_data()`.  
If the data is initialized as a collection, the pointer to its members can be accessed by `[]` operator with proper index. It will return the first element if the data itself is not a collection or the index is out of bound.
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

Given file `MyConfigFile.toml`:
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
    MainConfig.Bind(myInt64Data, 10); // std::int64_t 10
    MainConfig.Bind(myInt64ArrayData, 10, 78, 314, 996); // integer array
    MainConfig.Bind(myStringData, "Hello DKUtil"); // string
    MainConfig.Bind(myStringArrayData, "First", "Second", "Third"); // string collection
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

---
# Hook

Some APIs to write hooks and memory patches with ease.

## API

```c++	
// Empty a code cave in the body of target function and branch to trampoline
// Accepts a prolog patch before invoking payload and a epilog patch after returning from payload
template <
    const std::ptrdiff_t OffsetLow, 
    const std::ptrdiff_t OffsetHigh, 
    const CaveReturnPoint ReturnPoint = CaveReturnPoint::kSkipOP
    > 
inline auto AddCaveHook(
    const std::uintptr_t a_src, // target function address
    const FuncInfo a_func, // helper struct describing function info
    const Patch* a_prolog = nullptr,
    const Patch* a_epilog = nullptr
) noexcept // return CaveHookHandle

// Memory patch
Patch {
    const void* Data;
    const std::size_t Size;
}
```
## Example

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

---
## Logger

Some SKSE style macro loggers with `spdlog` backend
```C++
INFO("{} {} {} {}", a, b, c, d);
DEBUG("Important debug info that shows on debug build"sv);
ERROR("This will abort the process!"sv);
Dump(container, fmt);
```

---
## Utility

+ FNV-1A compile time string hashing with both 32bit/64bit implementation.
+ `lvalue_cast`
+ Singleton data model abstract class to save boiler plater code.