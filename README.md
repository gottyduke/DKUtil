<h1 align="center">DKUtil</h1>

Some utilitarian headers to help with x64 native plugin development

# Implementations
[![Config](https://img.shields.io/badge/Config-1.1.3-R.svg)](/docs/Config.md)
[![Hook](https://img.shields.io/badge/Hook-2.5.0-R.svg)](/docs/Hook.md)
[![Logger](https://img.shields.io/badge/Logger-1.2.1-R.svg)](/docs/Logger.md)
[![Utility](https://img.shields.io/badge/Utility-untracked-R.svg)](/docs/Utility.md)
[![Extra(For SKSE)](https://img.shields.io/badge/Extra-1.0.0-R.svg)](/docs/Extra.md)  

+ [Config](/docs/Config.md)
    - abstracted and contained config layer
    - `ini`, `toml`, `json` file support
    - `bool`, `int64_t`, `double`, `string` type support
    - built in array support
    - multiple file loads
    - generate default file
+ [Hook](/docs/Hook.md)
    - pattern scanner 
    - asm patch
    - cave hook
    - virtual method table swap
    - import address table swap
    - simple function hook (write_call/write_branch)
    - various usefully gathered utils
+ [Logger](/docs/Logger.md)
    - logging macros
+ [Utility](/docs/Utility.md)
    + function
        + `consteval` helper functions retrieving the argument count of a function.
    + model
        + `Singleton` data model abstract class to save boilerplate code.
        + `enumeration` addition to the original `RE::stl::enumeration`.
            + static reflection for enum name, type name and value name, support value_type(`n`) and flag_type(`1<<n`)
            + `std::ranges` iterator for value_range(`n`) and flag_range(`1<<n`)
        + `concepts` useful concepts for contraining function templates
        + `struct_cast`, `tuple_cast` compile time conversion for same aligned structs/tuples using structure binding (up to 9 bindable members)
        + `vector_cast`, `range_cast` constexpr conversion for `std::ranges::range` and `std::vector`
    + numbers
        + FNV-1A compile time string hashing with 32bit/64bit implementation.
    + string
        + `to_wstring` method
        + `concat` compile time string concatenation.
        + various string related functions using `std::ranges`
+ [Extra(For SKSE)](/docs/Extra.md)  
    + `CONSOLE` logging macro but for in-game console.
    + `serializable` painless, all-in-one serialization solution for SKSE plugins.(Planned to move to general support instead of strict SKSE)

# Consumption

## Requirement
+ [CMake](https://cmake.org)
+ [vcpkg](https://github.com/microsoft/vcpkg/releases)
+ `/std:c++23` or `/std:latest`
    + Config
        + [tomlplusplus](https://github.com/marzer/tomlplusplus)
        + [SimpleIni](https://github.com/brofield/simpleini)
        + [nlohmann-json](https://github.com/nlohmann/json)
    + Hook
        + [xbyak](https://github.com/herumi/xbyak)
    + Logger
        + [spdlog](https://github.com/gabime/spdlog)
    + Extra(For SKSE)
        + [CommonLibSSE](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE)

> All dependencies should be handled by vcpkg.


## Installation
Clone a copy of `DKUtil` onto your local environment, in your target project's `CMakeLists.txt`, add:  

```CMake
add_subdirectory("Path/To/Local/Copy/Of/DKUtil" DKUtil)

target_link_libraries(
	"YOUR PROJECT NAME"
	INTERFACE
		DKUtil::DKUtil
)
```
---
Or using `git submodule`, within your target project's root directory, add:

```PS
git submodule add https://github.com/gottyduke/DKUtil.git extern/DKUtil
git submodule update -f --init
```
And in your target project's `CMakeLists.txt`, add:
```CMake
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
			PATHS "extern/${DEPENDENCY}")

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

# dependencies
find_dependency_path(DKUtil include/DKUtil/Logger.hpp)
```
