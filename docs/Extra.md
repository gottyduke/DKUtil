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
All in one, painless serialization solution. All serializables are automatically registered to internal handler, and unregistered upon destruction(although probably not needed). All reasonable data types are supported, they are flattened first with internal resolvers, then serialized with SKSE.  
Common types:
+ STL containers
+ user defined aggregate types
+ all trivial types
+ string

### declaration
Wrap the data type in `dku::serializable<>`, with the second template parameter giving it a unique hash identifier. This identifier is used in the future for variant updating/backwards compatibility feature.
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
The data can be accessed by prepending the asterisk symbol `*` or using arrow operator `->` as if it were a pointer type, or the getter method `get()`.  
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
You can also pass in optional plugin identifier, although the internal hash function already makes a unique id.

### custom resolver
You can attach custom resolver callbacks to the `dku::serializable<>` types and they'll be called during `Save`, `Load`, `Revert` state, **after** they are resolved.
```C++
dku::serializable<MyComplexDataType, "ComplexDataMap"> myDataMap;
using type = decltype(myDataMap)::type;

// resolver function takes its underlying data
void CustomCallback(type& a_data, dku::serialization::ResolveOrder a_order)
{
    if (a_order == dku::serialization::ResolveOrder::kSave) {
        INFO("Saved");
    } else if (a_order == dku::serialization::ResolveOrder::kLoad) {
        INFO("Loaded");
        for (auto& [f, d] : a_data) {
            INFO("{} {}", f, d.s);
        }
    } else {
        INFO("Reverted");
    }
}

myDataMap.add_resolver(CustomCallback);
```

### flattened layout map
Tweak the preprocessor definitions a bit, define `DKU_X_MOCK` will redirect all read/write calls to its internal buffer for testing, and generate a flattened layout map of the data type it's operating on.  

---
<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>
