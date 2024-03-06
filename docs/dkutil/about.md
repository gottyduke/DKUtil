# DKUtil

Started as a helper header for personal projects, improved on demand with more and more features.

**platform**: Windows  
**standard**: c++20  

## Usage Requirement

+ [CMake](https://cmake.org)
+ [vcpkg](https://github.com/microsoft/vcpkg/releases)
+ `/std:c++20` or `/std:latest`
    + Config
        + [SimpleIni](https://github.com/brofield/simpleini)
        + [nlohmann-json](https://github.com/nlohmann/json)
    + Hook
        + [xbyak](https://github.com/herumi/xbyak)
    + Logger
        + [spdlog](https://github.com/gabime/spdlog)
    + Extra(For SKSE)
        + [CommonLibSSE](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE)

::: warning Dependencies
All dependencies will be handled by vcpkg & CMake.
:::

## Installation

::: code-group
```ps [local copy]
git clone https://github.com/gottyduke/DKUtil.git
```

```ps [git submodule]
git submodule add https://github.com/gottyduke/DKUtil.git extern/DKUtil
git submodule update --init --recursive -f
git submodule update --remote -f
```
:::

::: tip Environment
Adding path to DKUtil to environment variable `DKUtilPath` can help with consuming the library.
:::

::: info PluginTemplate
If this is your first time using DKUtil and starting a fresh plugin project, consider using [PluginTemplate](https://github.com/gottyduke/plugintemplate) and skip DKUtil setup process.
:::

## Consume in Projects

### CMake

Add to your target project's `CMakeLists.txt`:

::: code-group
```CMake [local tree]
add_subdirectory("Path/To/DKUtil" DKUtil)
```

```CMake{33} [global dependency]
# dependency macros
macro(find_dependency_path DEPENDENCY FILE)
	# searches extern for dependencies and if not checks the environment variable
	if(NOT ${DEPENDENCY} STREQUAL "")
		# Check extern
		message(
			STATUS
			"Searching for ${DEPENDENCY} using file ${FILE}"
		)
		find_path("${DEPENDENCY}Path"
			${FILE}
			PATHS "extern/${DEPENDENCY}"
		)

		if("${${DEPENDENCY}Path}" STREQUAL "${DEPENDENCY}Path-NOTFOUND")
			# Check path
			message(
				STATUS
				"Getting environment for ${DEPENDENCY}Path: $ENV{${DEPENDENCY}Path}"
			)
			set("${DEPENDENCY}Path" "$ENV{${DEPENDENCY}Path}")
		endif()

		message(
			STATUS
			"Found ${DEPENDENCY} in ${${DEPENDENCY}Path}; adding"
		)
		add_subdirectory("${${DEPENDENCY}Path}" ${DEPENDENCY})
	endif()
endmacro()

# find DKUtil with environment DKUtilPath or "extern/DKUtil"
find_dependency_path(DKUtil include/DKUtil/Logger.hpp)
```
:::

### VCPKG Port

Planned, low priority due to no demand
