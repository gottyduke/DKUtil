<h1 align="center">DKUtil::Logger</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Logger Source](/include/DKUtil/Logger.hpp)

Some SKSE style macro loggers with `spdlog` backend.

## Init
```C++
DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
    // this
	DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());
	
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	return true;
}
// or during SKSEPlugin_Query if developing for SE exclusively.
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

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
