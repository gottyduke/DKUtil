<h1 align="center">DKUtil</h1>

Some utility headers to help with x64 native plugin development

# Documentation
See [wiki here!](https://gottyduke.github.io/DKUtil/)

# Implementations
![Config](https://img.shields.io/badge/Config-1.2.0-R.svg)
![Hook](https://img.shields.io/badge/Hook-2.6.6-R.svg)
![Logger](https://img.shields.io/badge/Logger-1.2.5-R.svg)
![Utility](https://img.shields.io/badge/Utility-1.0.1-R.svg)
![Extra(For SKSE)](https://img.shields.io/badge/Extra-1.0.0-R.svg)

+ Config
    - abstracted and contained config layer
    - `ini`, `toml`, `json` file support
    - `bool`, `int64_t`, `double`, `string` type support
    - built in array support
    - multiple file loads & generate default file
    - custom formatted string parser to c++ structure
+ Hook
    - pattern scanner
    - asm patch
    - cave hook
    - virtual method table swap
    - import address table swap
    - simple function hook (write_call/write_branch)
    - non-volatile call (LTO enabled hooks)
    - various usefully gathered utils
+ Logge
    - logging macros
+ Utility
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
+ Extra(For SKSE)
    + `CONSOLE` logging macro but for in-game console.
    + `serializable` painless, all-in-one serialization solution for SKSE plugins.(Planned to move to general support instead of strict SKSE)

---
<p align="center">MIT License, 2020-present DK</p>
