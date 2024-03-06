# Proxy

`Proxy` supports `json`, `ini`, and `toml` files. Its type can be known at compile time and/or runtime determined for the given file. 

If the file name and type is already known at the compile time, use `COMPILE_PROXY` macro to get the corresponding parser for such file type. 

If the file name is runtime generated, such as file iterator from a directory, use `RUNTIME_PROXY` macro for file type/name that is unknown at the compile time.  

```cpp
#include "DKUtil/Config.hpp"

JsonConfig MainConfig = COMPILE_PROXY("MyConfigFile.json"sv);
// or to omit the type
auto MainConfig = COMPILE_PROXY("MyConfigFile.json"sv);

std::string runtimeFileName = SomeLoadFileCall();
auto RuntimeConfig = RUNTIME_PROXY(runtimeFile);
```

## Bind

`Proxy::Bind` is used for binding data to proxy **with** default value.

```cpp
Integer myInt64Data{ "MyIntData" };

auto Proxy = COMPILE_PROXY("MyConfigFile.toml"sv);

Proxy.Bind(myInt64Data, 100);
```

To bind data with default collection value, pass in multiple values:

```cpp
Proxy.Bind(myInt64Data, 100, 200, 300);
```

To bind data with a min/max numeric range, pass in template parameters. Range limits collection values as well.

```cpp
Proxy.Bind<-100, 100>(myInt64Data, -20, 50);
```

::: tip Mismatch
The actual data value/collection will be updated after `Proxy::Load`, a default singular value will become a collection of values if the actual config is array.
:::

## Load

After binding data to proxy, `Proxy::Load` can be used to load, parse, and update bound data values.

::: code-group

```cpp [File]
Proxy.Load();
```

```cpp [String]
std::ifstream reader{ filename };
std::string file{};
reader >> file;

Proxy.Load(file.data());
```

:::

::: warning Missing File
If the file cannot be found, a default configuration file with default values will be written in place.
:::

## Write

`Proxy::Write` can be used to output to the file with current bound data values.

```cpp
Proxy.Write()
```

`Proxy::Write` also accepts an optional output filename parameter instead.

## Append

To append new data entries to an existing config file(retroactive update):

```cpp
Proxy.Load();
// generate new entries with default value
Proxy.Generate();
// write new config file
Proxy.Write();
```
