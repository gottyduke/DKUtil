<h1 align="center">DKUtil::Logger</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Logger Source](/include/DKUtil/Logger.hpp)

Some SKSE style macro loggers with `spdlog` backend.
```C++
INFO("{} {} {} {}", a, b, c, d);
DEBUG("Important debug info that shows on debug build"sv);
ERROR("This will abort the process with a messagebox!");

ENABLE_DEBUG
// debug logs from here will be printed
DISABLE_DEBUG
// debug logs from here will be omitted

// or change log level manually
DKUtil::Logger::SetLevel(spdlog::level::level_enums);
```

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
