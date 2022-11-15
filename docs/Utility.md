<h1 align="center">DKUtil::Utility</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Utility Source](/include/DKUtil/Utility.hpp)


# Utility

Some helper functions used within other DKUtil headers.
+ function
    + `consteval` helper functions retrieving the argument count of a function.
+ model
    + `Singleton` data model abstract class to save boiler plater code.
    + `enumeration` addition to the original `RE::stl::enumeration`.
+ numbers
    + FNV-1A compile time string hashing with 32bit/64bit implementation.
+ string
    + `to_wstring` method
    + `concat` compile time string concatenation.
    + various string related functions taken from CommonLibSSE.

## enumeration
On the basis of original `stl::enumeration`, `DKUtil::enumeration` adds the following:
* static reflection for enum value name, type name and class name.
* `std::ranges` iterator adaptor for value_range/flag_range.
* expanded ctor with concept restraint auto templates.

```C++

    enum class ContiguousFlag
    {
        NONE = 0u,

        RAX = 1u << 0,
        RCX = 1u << 1,
        RDX = 1u << 2,
        RBX = 1u << 3,
        RSP = 1u << 4,
        RBP = 1u << 5,
        RSI = 1u << 6,
        RDI = 1u << 7,
        RF = 1u << 8,
        R8 = 1u << 9,
        R9 = 1u << 10,
        R10 = 1u << 11,
        R11 = 1u << 12,
        R12 = 1u << 13,
        R13 = 1u << 14,
        R14 = 1u << 15,
        R15 = 1u << 16,
    };

    enum class SparseFlag
    {
        NONE = 0u,

        RAX = 1u << 0,
        RCX = 1u << 1,
        //RDX = 1u << 2,
        RBX = 1u << 3,
        RSP = 1u << 4,
        //RBP = 1u << 5,
        RSI = 1u << 6,
        RDI = 1u << 7,
        RF = 1u << 8,
        //R8 = 1u << 9,
        R9 = 1u << 10,
        //R10 = 1u << 11,
        R11 = 1u << 12,
        //R12 = 1u << 13,
        R13 = 1u << 14,
        R14 = 1u << 15,
        R15 = 1u << 16,
    };

    enum class ContiguousValue
    {
        NONE = 0u,

        RAX = 1,
        RCX = 2,
        RDX = 3,
        RBX = 4,
        RSP = 5,
        RBP = 6,
        RSI = 7,
        RDI = 8,
        RF = 9,
        R8 = 10,
        R9 = 11,
        R10 = 12,
        R11 = 13,
        R12 = 14,
        R13 = 15,
        R14 = 16,
        R15 = 17,
    };

    enum class SparseValue
    {
        NONE = 0u,

        RAX = 1,
        RCX = 2,
        //RDX = 3,
        RBX = 4,
        RSP = 5,
        //RBP = 6,
        RSI = 7,
        RDI = 8,
        RF = 9,
        //R8 = 10,
        R9 = 11,
        //R10 = 12,
        R11 = 13,
        //R12 = 14,
        R13 = 15,
        R14 = 16,
        R15 = 17,
    };
} // namespace Enum


void TestEnum() noexcept
{		
    using namespace Enum;

    // concept-restraint auto ctor
    dku::enumeration<ContiguousValue UNDERLYING> cValues{ 0, 2, 4, 5, 9, 15 };
    dku::enumeration<SparseValue UNDERLYING> sValues{ 0, 2, 4, 5, 9, 15 };
    dku::enumeration<ContiguousFlag UNDERLYING> cFlags{ 0, 2, 4, 5, 9, 15 };
    dku::enumeration<SparseFlag UNDERLYING> sFlags{ SparseFlag::NONE, SparseFlag::RCX, SparseFlag::RBX, SparseFlag::RSI, SparseFlag::R9, SparseFlag::R14 };
    
    /**/
    // static reflections
    // 1) check for value-type enum reflection
    // 2) check for flag-type enum reflection
    // 3) check for mixed sparse enum reflection
    // 4) check for mixed contiguous enum reflection
    // 5) direct invocation between raw underlying value and enum value
    INFO("Reflecting enum value names");
    for (auto i : std::views::iota(0, 17)) {
        INFO("{}{} {}", cValues.is_flag() ? "1<<" : "", i, cValues.to_string(static_cast<ContiguousValue>(i)));
        INFO("{}{} {}", sValues.is_flag() ? "1<<" : "", i, sValues.to_string(i));
        INFO("{}{} {}", cFlags.is_flag() ? "1<<" : "", i, cFlags.to_string(i));
        INFO("{}{} {}", sFlags.is_flag() ? "1<<" : "", i, sFlags.to_string(i));
    }

    // 5) check for enum class name reflection
    INFO("Enum name:\n{}\n{}\n{}\n{}", cValues.enum_name(), sValues.enum_name(), cFlags.enum_name(), sFlags.enum_name());
    // 6) check for enum underlying type name
    INFO("Type name:\n{}\n{}\n{}\n{}", cValues.type_name(), sValues.type_name(), cFlags.type_name(), sFlags.type_name());
    
    // 7) value range iterator
    INFO("Iterating by value_range");
    for (const auto e : cValues.value_range(ContiguousValue::NONE, ContiguousValue::R15)) {
        INFO("{}", cValues.to_string(e));
    }

    INFO("Iterating by flag_range");
    for (const auto e : cFlags.flag_range(ContiguousFlag::NONE, ContiguousFlag::R15)) {
        INFO("{}", cFlags.to_string(e));
    }
    
    // 8) string to enum cast
    std::string nameStr1{ "rsi" };
    std::string nameStr2{ "RaX" };
    std::string nameStr3{ "Rdx" };

    auto nameEnum1 = cValues.from_string(nameStr1);
    auto nameEnum2 = cValues.from_string(nameStr2);
    auto nameEnum3 = cValues.from_string(nameStr3);

    INFO("String {} -> Enum {}", nameStr1, std::to_underlying(nameEnum1.value()));
    INFO("String {} -> Enum {}", nameStr2, std::to_underlying(nameEnum2.value()));
    INFO("String {} -> Enum {}", nameStr3, std::to_underlying(nameEnum3.value()));
    ContiguousValue Enum1 = nameEnum1.value();
    /**/
}
```
   
---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
