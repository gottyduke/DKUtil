<h1 align="center">DKUtil</h1>

Some utilitarian headers to help with SKSE64 plugin development

# Implementations
[![Config](https://img.shields.io/badge/Config-1.1.3-R.svg)](/docs/Config.md)
[![Hook](https://img.shields.io/badge/Hook-2.5.0-R.svg)](/docs/Hook.md)
[![Logger](https://img.shields.io/badge/Logger-1.2.1-R.svg)](/docs/Logger.md)
[![Utility](https://img.shields.io/badge/Utility-untracked-R.svg)](/docs/Utility.md)
[![Extra](https://img.shields.io/badge/Extra-1.0.0-R.svg)](/docs/Extra.md)  

+ [Config](/docs/Config.md)
    - `ini`, `toml`, `json` file support
    - `bool`, `int64_t`, `double`, `string` type support
    - built in array support
    - multiple file loads
+ [Hook](/docs/Hook.md)
    - ASM Patch
    - Cave Hook
    - Virtual Method Table Swap
    - Import Address Table Swap
+ [Logger](/docs/Logger.md)
    - Logging macros
+ [Utility](/docs/Utility.md)
    + function
        + `consteval` helper functions retrieving the argument count of a function.
    + model
        + `Singleton` data model abstract class to save boiler plater code.
        + `enumeration` addition to the original `RE::stl::enumeration`.
            + static reflection for enum name, type name and value name, support value_type(`n`) and flag_type(`1<<n`)
            + `std::ranges` iterator for value_range(`n`) and flag_range(`1<<n`)
            + concept auto templated functions
        + `concepts` useful concepts for contraining function templates
        + `struct_cast`, `tuple_cast` compile time conversion for same aligned structs/tuples using structure binding (up to 9 bindable members)
        + `vector_cast`, `range_cast` constexpr conversion for `std::ranges::range` and `std::vector`
    + numbers
        + FNV-1A compile time string hashing with 32bit/64bit implementation.
    + string
        + `to_wstring` method
        + `concat` compile time string concatenation.
        + various string related functions using `std::ranges`
+ [Extra](/docs/Extra.md)  
    + `CONSOLE` logging macro but for in-game console.
    + `serializable` painless, all-in-one serialization solution for SKSE plugins.

# Consumption

## Requirement
+ [CMake](https://cmake.org)
+ [vcpkg](https://github.com/microsoft/vcpkg/releases)
+ `/std:c++23` or `/std:latest`
    + Config
        + [tomlplusplus](https://github.com/marzer/tomlplusplus)
        + [SimpleIni](https://github.com/brofield/simpleini)
        + [nlohmann-json](https://github.com/nlohmann/json)
    + Logger
        + [spdlog](https://github.com/gabime/spdlog)
    + Extra
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
