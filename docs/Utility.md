<h1 align="center">DKUtil::Utility</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Utility Source](/include/DKUtil/Utility.hpp)

Some helper functions used within other DKUtil headers.
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
  
## enumeration
On the basis of original `stl::enumeration`, `DKUtil::enumeration` adds the following:
* static reflection for enum value name, type name and class name.
* `std::ranges` iterator adaptor for value_range/flag_range.
* expanded ctor with concept restraint auto templates.
> static reflection is not implemented using external lib, DKUtil wraps a lightweight compile time nasty macro inside.

```C++
enum class Color : std::uint32_t
{
    red,
    yellow,
    white,
};

// print
Color redColor = Color::red;
INFO("this enum is {}", dku::print_enum(redColor));

// cast
std::string colorStr = "yellow";
auto& colorTbl = dku::static_enum<Color>();
Color yellowColor = colorTbl.from_string(colorStr);

// iterate
for (const Color c : colorTbl.value_range(Color::red, Color::white)) {
    INFO("color is {}", colorTbl.to_string(c));
}

// iterate flag type enum ( 1 << 1, 1 << 2 etc..)
enum class ColorFlag : std::uint32_t
{
    red = 1 << 1,
    yellow = 1 << 2,
    white = 1 << 3,
};
auto& flagTbl = dku::static_enum<ColorFlag>();

for (const ColorFlag c : flagTbl.flag_range(ColorFlag::red, ColorFlag::white)) {
    INFO("color is {}", flagTbl.to_string(c));
}
```

## struct_cast, tuple_cast, concepts
compile time conversion for same aligned struct/tuple
```C++
struct AggregateType
{
	int i;
	std::string s;
	char c;
	bool b;
};

auto tv = dku::model::tuple_cast(AggregateType{});
static_assert(std::is_same_v<decltype(tv), std::tuple<int, std::string, char, bool>>);
auto sv = dku::model::struct_cast<AggregateType>(tv);
static_assert(std::is_same_v<decltype(sv), AggregateType>);

// bindables
static_assert(dku::model::number_of_bindables<AggregateType>() == 4);
static_assert(dku::model::number_of_bindables<decltype(tv)>() == 4);
static_assert(dku::model::number_of_bindables<decltype(av)>() == 4);

// concepts	
int av[] = { 1, 2, 3, 4 };
static_assert(dku::model::concepts::dku_aggregate<AggregateType>);
static_assert(dku::model::concepts::dku_bindable<decltype(tv)>);
static_assert(dku::model::concepts::dku_ranges<decltype(av)>);
static_assert(dku::model::concepts::dku_trivial_ranges<decltype(av)>);
```

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
