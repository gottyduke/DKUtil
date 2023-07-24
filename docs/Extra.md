<h1 align="center">DKUtil::Extra</h1>
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>

[Extra Source](/include/DKUtil/Extra.hpp)

On expanding small additions that requires `CommonLibSSE` to function.
+ `CONSOLE` logging macro but for in-game console.
+ `serializable` painless, all-in-one serialization solution for SKSE plugins.

## CONSOLE
logging macro for in game console
```C++
CONSOLE("{} {} {}", a, b, c);
CONSOLE("This will be logged to in game console", a, b, c);
```

## serializable
All in one, painless serialization solution

### declaration
```C++
struct MyDataType
{
    int a;
    bool b;
    char c;
    std::string s;
    std::vector<int> arr;
};
using MyComplexDataType = std::map<RE::FormID, MyDataType>;

// empty
dku::serializable<MyDataType, "SimpleData"> mySimpleData;
// or initializer list
dku::serializable<MyDataType, "SimpleData"> mySimpleData(
    {1, true, 'a', "Elo", {1,2,3,4,5}});
// or assignment
dku::serializable<MyDataType, "SimpleData"> mySimpleData = MyDataType{
    {1, true, 'a', "Elo", {1,2,3,4,5}}};

dku::serializable<MyComplexDataType, "ComplexDataMap"> myDataMap;
// ...
```

### using, modifying
The data can be accessed by prepending the asterisk symbol `*` as if it were a pointer type, or the getter method `get()`, or using arrow operator `->`  
```C++
for (auto& [id, data] : *myDataMap) {
    // ...
}

myDataMap->emplace(...);
```

### registering
During plugin load, simply call it once and you are done:
```C++
DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());
	
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

    // this
	dku::serialization::api::RegisterSerializable();

	return true;
}
```
You can also pass in optional plugin identifier, although the internal hash function already makes an unique id.

### custom resolver
You can attach custom resolver callbacks to the `dku::serializable<>` types and they'll be called during `Save`, `Load`, `Revert` state.
```C++
dku::serializable<MyComplexDataType, "ComplexDataMap"> myDataMap;
using type = decltype(myDataMap)::type;

// resolver function takes its underlying data
void CustomCallback(type& a_data, dku::serialization::ResolveOrder a_order)
{
    if (a_order == dku::serialization::ResolveOrder::kSave) {
        INFO("Saving");
        for (auto& [f, d] : a_data) {
            INFO("{} {}", f, d.s);
        }
    } else if (a_order == dku::serialization::ResolveOrder::kLoad) {
        INFO("Loading");
        for (auto& [f, d] : a_data) {
            INFO("{} {}", f, d.s);
        }
    } else {
        INFO("Reverting");
        for (auto& [f, d] : a_data) {
            INFO("{} {}", f, d.s);
        }
    }
}
```

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
