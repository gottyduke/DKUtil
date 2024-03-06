# Config Examples

Given configuration file `MyConfigFile.toml`:

```toml
[SectionA]
MyIntData = 10086
MyIntArray = [99, 96, 92, 87, 71]
MyDouble = 114.514
MyBool = true
MyString = "Hello toml"
MyStringArray = [
    "First",
    "Second",
    "LastButNotLeast"
]
```

::: code-group

```cpp [Load Data]
#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias; // For type alias

class Settings : dku::model::Singleton<Settings>
{
public:
    Integer MyInt64Data{ "MyIntData" };
    Integer MyInt64ArrayData{ "MyIntArray" };
    Double MyDoubleData{ "MyDouble" };
    Boolean MyBoolData{ "MyBool" };
    String MyStringData{ "MyString" };
    String MyStringArrayData{ "MyStringArray" };

    void Load() noexcept 
    {
        _config.Bind<0, 100>(MyInt64Data, 10);
        _config.Bind<-360, 360>(MyInt64ArrayData, 10, 78, 314, 996);
        _config.Bind(MyStringData, "Hello DKUtil");
        _config.Bind(MyStringArrayData, "First", "Second", "Third");
        _config.Bind(MyBoolData, true);
        _config.Bind<0.0, 10.0>(MyDoubleData, 3.14154);

        _config.Load();
    }

private:
    // cannot use auto for class member declaration
    TomlConfig _config = COMPILE_PROXY("MyConfigFile.toml"sv);
}
```

```cpp [Use Data]
auto* setting = Settings::GetSingleton();

// access
if (setting->MyBoolData.get_data()) {
    // true
}

// access data
INFO("{} {} {}", *setting->MyInt64Data, *setting->MyStringData, *setting->MyDoubleData);

// traverse the collection
for (auto& str : setting->MyStringArrayData.get_collection()) {
    INFO(str);
}

// modify
auto& data = *setting->MyInt64Data;
int result = SomeCalc(data);
data *= result;
```

:::

::: tip
Wrapping in a static Settings class is recommended but not required.
:::

Also see [external references](references)