# DKUtil::Logger

Some macro style loggers with `spdlog` backend.

## Setup PROJECT_NAME

For backwards compatibility reasons, DKUtil requires `PROJECT_NAME` definition prior to including any DKUtil files.

```cpp
#define PROJECT_NAME "YourPluginName"
```

::: tip 
This is already configured if using [PluginTemplate](https://github.com/gottyduke/plugintemplate). 
:::

## Init Logger

By default the log file outputs to the current working path.  
The relative path `LOG_PATH` can be defined before including.

```cpp{4}
#define LOG_PATH "logs\\" // optional
#include "DKUtil/Logger.hpp"

DKUtil::Logger::Init(PROJECT_NAME, VersionString);
```

## Internal Debugging

DKUtil extensively logs its internal operations when building with `Debug` configuration. To disable this behavior, define `DKU_L_DISABLE_INTERNAL_DEBUGGING` explicitly prior to including any DKUtil files. 

```cpp
#define DKU_L_DISABLE_INTERNAL_DEBUGGING
#include "DKUtil/Logger.hpp"
``` 

::: tip Release Builds
`DKU_L_DISABLE_INTERNAL_DEBUGGING` is enabled on release builds.
:::
