<h1 align="center">DKUtil::Logger</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Logger Source](/include/DKUtil/Logger.hpp)

Some macro style loggers with `spdlog` backend.

## Init
```C++
// by default the log directory is current process path
// this can be manually overriden
#define LOG_PATH "logs\\"
#include "DKUtil/Logger.hpp"

DKUtil::Logger::Init(NameString, VersionString);
```

## macro
```C++
TRACE("trace {}", debugVal);
INFO("{} {} {} {}", a, b, c, d);
DEBUG("Important debug info that shows on debug build"sv);
WARN("Generate warnings in log");
ERROR("This will prompt for exit!");
FATAL("This will abort with message.");

ENABLE_DEBUG
// debug logs from here will be printed
DISABLE_DEBUG
// debug logs from here will be omitted

// or change log level manually
DKUtil::Logger::SetLevel(spdlog::level::level_enums);
```
> For fmt syntax, refer to [fmtlib syntax](https://fmt.dev/latest/syntax.html).

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
