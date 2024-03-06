# Schema Parser

`Config::Schema` is a special type of `Proxy` that allows you to parse strings into custom c++ data structure.

## Example

Given a struct `CustomData`:

```cpp
struct CustomData
{
    std::uint64_t form;
    std::string   name;
    std::string   payload;
    bool          excluded;
};
```

And expect schema string with delimiter `|`:

```
"0x12345|Random|None|true"
```

## One Liner

For the sake of ease of use, `DKUtil::Config` provides static function to parse strings as you go:

```cpp
std::string s{ "0x12345|Random|None|true" };

CustomData data = dku::Config::ParseSchemaString<CustomData>(s, "|");
INFO("{} {} {} {}", data.form, data.name, data.payload, data.excluded);
```

## Use Proxy

To parse the entire schema file that consists lines of schema strings:

```cpp
auto schema = SCHEMA_PROXY("Schema_SC.txt");
schema.Load();
auto& parser = schema.get_parser();

std::vector<CustomData> data;
while (true) {
    auto entry = parser.ParseNextLine<CustomData>("|");
    if (!entry.has_value()) {
        break;
    }

    data.push_back(entry.value());
}
```

::: tip
You can also utilize [`dku::Config::GetAllFiles<recursive>(...)`](file-helpers) to collect files at runtime.
:::

## Aggregate Conversion

All schema functions support user defined type and varadic template arguments:

```cpp
std::string s{ "0x12345|1.5|true" };

auto [a,b,c] = dku::Config::ParseSchemaString<int, double, bool>(s, "|");
```

## Special Values

**spaces**: all whitespaces for data type except `std::string` will be trimmed before parsing.

```cpp
" 1000 " == "1000 " == "1000" 
```

**bool**: `true`/`false` is case insensitive, `1`/`0` is also accepted.

```cpp
" True     " == "True" == "true" == "1"
```

**hex**: to guarantee a hex number conversion, regardless of prefix `0x` in string segment, use `dku::numbers::hex` in place of `std::uint64_t`.

```cpp
dku::numbers::hex formID;            // special hex number holder
formID = "0x14";                     // 0x14
formID = "100";                      // 0x100
formID = 100;                        // 0x64
std::uint64_t num = formID;          // implicit conversion to numbers
fmt::format("{} {}", formID, num);   // formats to "0x64 100"
```
