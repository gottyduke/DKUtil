# DKUtil::Config

An all purpose configuration module for x64 native plugin projects, supports `ini`, `json` and `toml` file types out of box. No more handling these settings on our own, use simplied API from this library!

## Workflow

The configuration module in DKUtil works as "proxy per file", each `Proxy` represents one specific configuration file and manages the subsequent `Bind`, `Load`, `Write`, and `Generate` calls to this file and its bound `Data`. 

1. Declare `Data` with key names.
2. Initialize a `Proxy` for a config file on system.
3. Binds the `Data` to `Proxy` with default values.
4. Use `Proxy` to load/regenerate/output the bound `Data`.
5. Use updated `Data` in code.

## Usage

The entire module is behind the namespace `DKUtil::Config` or `dku::Config`. To use, include the header:

```cpp
#include "DKUtil/Config.hpp"
```

## Config Lookup Path

By default the config file lookup directory is the current working path.  
The relative path `CONFIG_ENTRY` can be defined before including.

```cpp
#define CONFIG_ENTRY "configs\\"
// this translates to "process_cwd/configs/"
#include "DKUtil/Config.hpp"
```
