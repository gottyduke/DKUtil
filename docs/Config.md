<h1 align="center">DKUtil::Config</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Config Source](/include/DKUtil/Config.hpp)

An all purpose configuration library for SKSE plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

## Data
`DKUtil::Config` supports 3 primitive data types and string variants. All types except `Boolean` are collection-enabled, to mimic the array-like behaviors as if they were one.  

The data can be accessed by prepending the asterisk symbol `*` as if it were a pointer type, or the getter method `get_data()`.  
If the data is initialized as a collection, the pointer to its members can be accessed by `[]` operator with proper index. It will return the first element if the data itself is not a collection, or the last element if the index is out of bound.  

To declare configuration varaibles:
```C++
#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias; // type alias

// initialize with key field, section field is optional and will not stop DKUtil from accessing data
Integer myInt64Data{ "MyIntDataKey", "OptionalSection" }; // std::int64_t
Double myDoubleData{ "MyDoubleKey" }; // double
Boolean myBoolData{ "MyBoolKey" }; // bool
String myStringData{ "MyStringKey" }; // std::basic_string<char>
```

## Proxy
The configuration system in DKUtil works as "proxy per file", each `Proxy` represents one specific configuration file and the subsequent `Load` and `Write` calls.

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
The data can be declared with/without `static` keyword and initialized with the proper key name and/or optional section name. You may `Bind` the data to a `Proxy` with default value set for that data.  

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
    MainConfig.Bind(myBoolData, true); // bool, no bool array support
    MainConfig.Bind(myDoubleData, 3.14154); // double
    // or optionally, you can bind with minimal/maximal range
    MainConfig.Bind<0, 3.3>(myDoubleData, 3.14154); // double with range of 0 ~ 3.3

    MainConfig.Load();

    INFO("{} {} {}", *myInt64Data, *myStringData, *myDoubleData); // 10086 hello toml 114.514

    // traverse the array
    for (auto& place : myStringArrayData.get_collection()) {
        INFO(place);
    } // Frust Sedoncst LastButNotLeast


    std::string FileHead = "Mysterious";
    std::string FileTrail = "File";
    std::string FileType = ".toml";

    std::string runtimeFile = FileHead + FileTrail + FileType;
    auto RuntimeConfig = RUNTIME_PROXY(runtimeFile); // will be evaluated to toml proxy at runtime
}
```

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
