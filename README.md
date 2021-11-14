# DKUtil
Backend library for my personal SKSE plugin development.
---
---
## Consumption
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
## Implementations:
+ [Config](#Config)
+ ENB (WIP)
+ [Hook](#Hook)
+ [Logger (spdlog backend)](#Logger)
+ [Utility](#Utility)
---

## Config
An all purpose configuration library for SKSE plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

### Data
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

Given file `MyConfigFile.toml`:
```TOML
[RandomSectionNameButNeedsToPresent]
myIntDataKey = 10086
myInt64ArrayData = [99, 96, 92, 87, 71]
myDoubleData = 114.514
myBoolData = true
myStringData = "Hello toml"
myStringArrayData = [
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

***
## Hook
Smart way to write hooks and memory patches with ease. Determines the best hooking/patching strategy on compile time and save you those memory bytes.

### API
```c++
template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
void BranchToID(
    std::uintptr_t a_hookFunc,          // destination function to hook
    const void* a_prePatch = nullptr,   // patch before calling destination function
    const std::uint64_t a_preSize = 0,  // size of pre patch
    const void* a_postPatch = nullptr,  // patch after returning from destination function
    const std::uint64_t a_postSize = 0, // size of post patch
    const bool a_preserveRax = false    // toggle preserving rax register
); 

// packed parameter
struct BranchInstruction
{
    const void* PrePatch;
    const std::size_t PrePatchSize;
    const void* PostPatch;
    const std::size_t PostPatchSize;
    const bool PreserveRax = false;
};

template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
void BranchToID(
    const std::uintptr_t a_hookFunc,    // destination function to hook
    BranchInstruction a_instruction     // packed parameter to reduce parameter list length
);

```
### Function detour example
```C++
// destination function, no return, no parameter
void Hook_MyAwesomeFunc() {
    // awesome stuff
}

constexpr std::uint64_t FUNC_ID = 12345;        // id of source address in bin file
constexpr std::uintptr_t OFFSET_START = 0x120;  // offset start from the FUNC_ID base address
constexpr std::uintptr_t OFFSET_END = 0x130;    // offset end from FUNC_ID base address

void InstallHooks() 
{
    DKUtil::Hook::BranchToID<FUNC_ID, OFFSET_START, OFFSET_END>(
        std::addressof(Hook_MyAwesomeFunc)
    );
}
```
In this example we simply wrote a detour from address that corresponds to id `12345` with offset `0x120`, to our own `Hook_MyAwesomeFunc` function, and return to address that corresponds to id `12345` with offset `0x130`.


### Memory patching example
```C++
// destination function, return int, 1 int parameter
int Hook_MyAwesomeFunc(int a_paramenter){
    // awesome stuff
    return a_paramenter + a_paramenter;
}

constexpr std::uint64_t FUNC_ID = 12345;
constexpr std::uintptr_t OFFSET_START = 0x120;
constexpr std::uintptr_t OFFSET_END = 0x130;

void InstallHooks() 
{
    DKUtil::Hook::BranchToFunction<FUNC_ID, OFFSET_START, OFFSET_END>(
        std::addressof(Hook_MyAwesomeFunc),
        "\x48\x8B\x0D\x30\x00\x00\x00", // mov rcx, qword ptr [ rip + 0x30 ]
        7,                              // size of pre patch
        "\x48\x89\xC1",                 // mov rcx, rax
        3                               // size of post patch
    );
}
```
In this example not only we wrote a detour from address that corresponds to id `12345` with offset `0x120`, to our own `Hook_MyAwesomeFunc` function, but also added pre patch code before detouring and post patch code upon returning to address that corresponds to id `12345` with offset `0x130`.

Use packed parameter to reduce the length of parameter list.
***
## Logger
Some SKSE style macro loggers with `spdlog` backend
```C++
INFO("{} {} {} {}", a, b, c, d);
DEBUG("Important debug info that shows on debug build"sv);
ERROR("This will abort the process!"sv);
```

***
## Utility
Currently just FNV-1 and FNV-1A compile time string hashing implementation.
