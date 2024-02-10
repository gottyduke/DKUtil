<h1 align="center">DKUtil::Config</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Config Source](/include/DKUtil/Config.hpp)

An all purpose configuration library for x64 native plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

## Config Path
By default the config file lookup directory is in current process path.  
This can be manually overriden by defining a relative `CONFIG_PATH` before including.
```cpp
#define CONFIG_PATH "configs\\"
#include "DKUtil/Config.hpp"
```
> For SKSE, SFSE, and F4SE plugin projects, their paths are automatically provided.

## Data
`DKUtil::Config` supports 3 primitive data types and string variants. All types except `Boolean` are collection-enabled, to mimic the array-like behaviors as if they were one.  

The data can be accessed by prepending the asterisk symbol `*` as if it were a pointer type, or the getter method `get_data()`/`get_collection()`.  

If the data is initialized as a collection, the pointer to its members can be accessed by `[]` operator with proper index. It will return the first element if the data itself is not a collection, or the last element if the index is out of bound.  

To declare configuration data, initialize them with key field.
```cpp
using namespace DKUtil::Alias; // type alias

Integer myInt64Data{ "MyIntDataKey", "General" }; // std::int64_t
Double myDoubleData{ "MyDoubleKey", "General" }; // double
Boolean myBoolData{ "MyBoolKey" }; // bool
String myStringData{ "MyStringKey" }; // std::basic_string<char>
```
> Section field is optional and unnamed sections will be put under `Global`. Section is ignored for JSON.

## Proxy
The configuration system in DKUtil works as "proxy per file", each `Proxy` represents one specific configuration file and the subsequent `Load`, `Write`, and `Generate` calls.

`Proxy` can be compile time and/or runtime determined for the given file. If the file name and type is already known at the compile time, use `COMPILE_PROXY` macro to get the corresponding parser for such file type. If the file name is runtime generated, use `RUNTIME_PROXY` macro for file type/name that is unknown at the compile time.  
```cpp
#include "DKUtil/Config.hpp"

// it can be .ini, .json or .toml
auto MainConfig = COMPILE_PROXY("MyConfigFile.json"sv); // will be evaluated to json proxy at compile time

std::string runtimeFileName = SomeLoadFileCall();
auto RuntimeConfig = RUNTIME_PROXY(runtimeFile); // will be evaluated to appropriate proxy at runtime
```

## Load
File `MyConfigFile.toml`:
```toml
[RandomSectionNameButNeedsToPresent]
MyIntData = 10086
MyIntArray = [99, 96, 92, 87, 71]
MyDouble = 114.514
MyBool = true
MyString = "Hello toml"
MyStringArray = [
    "Frust",
    "Sedoncst",
    "LastButNotLeast"
]
```
Code (wrapping in a class is not required):  
```cpp
#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias; // For type alias

class Settings : dku::model::Singleton<Settings>
{
public:
    Integer myInt64Data{ "MyIntData" };
    Integer myInt64ArrayData{ "MyIntArray" };
    Double myDoubleData{ "MyDouble" };
    Boolean myBoolData{ "MyBool" };
    String myStringData{ "MyString" };
    String myStringArrayData{ "MyStringArray" };

    // your custom config load call
    void Load() noexcept 
    {
        static std::once_flag Bound;
        std::call_once(Bound, [&]() {
            // bind configs to file with default values
            MainConfig.Bind(myInt64Data, 10); // std::int64_t 10
            MainConfig.Bind(myInt64ArrayData, 10, 78, 314, 996); // std::int64_t array
            MainConfig.Bind(myStringData, "Hello DKUtil"); // string
            MainConfig.Bind(myStringArrayData, "First", "Second", "Third"); // string array
            MainConfig.Bind(myBoolData, true); // bool, no bool array support
            MainConfig.Bind(myDoubleData, 3.14154); // double
            // or optionally, you can bind with minimal/maximal range
            MainConfig.Bind<0, 3.3>(myDoubleData, 3.14154); // double with range of 0 ~ 3.3
        });

        MainConfig.Load();

        INFO("{} {} {}", *myInt64Data, *myStringData, *myDoubleData); // 10086 hello toml 114.514

        // traverse the array
        for (auto& place : myStringArrayData.get_collection()) {
            INFO(place);
        } // Frust Sedoncst LastButNotLeast
    }

private:
    // cannot use auto for class member declaration
    TomlConfig MainConfig = COMPILE_PROXY("MyConfigFile.toml"sv);
}
```

## Write
During `Proxy.Load` call, a default config file will be generated if missing.

To update an existing config file with modified data:
```cpp
Proxy.Write();
```

To append new data entries an existing config file(retroactive update):
```cpp
// load existing entries
Proxy.Load();
// generate new entries with default value
Proxy.Generate();
// write new config file
Proxy.Write();
```


---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
