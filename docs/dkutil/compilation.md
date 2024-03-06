# Compilation

DKUtil is a header only library (so far), when incorporated into CMake projects, header only libraries (a.k.a INTERFACE target) won't be visible in generated msvc solution.

There are sneaky build config to enable DKUtilDebugger target in CMake, however, currently it requires SKSE workspace set up. If you are not a user of [SKSEPlugins](https://github.com/gottyduke/SKSEPlugins), it will be difficult to build this way.

```ps
cmake -B build -S . --preset=REL -DPLUGIN_MODE:BOOL=TRUE -DDKUTIL_DEBUG_BUILD:BOOL=TRUE
```