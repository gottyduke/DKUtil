# Data Types

`DKUtil::Config` supports 3 primitive data types, string variants and their array type(collection).

+ Integer
+ Double
+ Boolean
+ String

## Declaring

To declare configuration data, initialize them with key field and optional section field.

```cpp
using namespace DKUtil::Alias;                    // type alias

Integer myInt64Data{ "MyIntDataKey", "General" }; // std::int64_t
Double myDoubleData{ "MyDoubleKey", "General" };  // double
Boolean myBoolData{ "MyBoolKey" };                // bool
String myStringData{ "MyStringKey" };             // std::basic_string<char>
```

::: tip Section
Section field is optional and unnamed sections will be put under `Global`. Section is also ignored for JSON.
:::

## Accessing

The data can be accessed by operator `*` as if it were a pointer, or the const getter method `get_data()`/`get_collection()`.  

::: warning Collection on Singular Data
When the data is singular but called with `get_collection()`, a size-1 collection will be returned with the singular data only.
:::

If the data is a collection, the reference of its members can be accessed by operator `[]` with index. There are two exceptions to this:

+ The **first** element will be returned if the data is not a collection.
+ The **last** element will be returned if the index is out of bound.  

`is_collection()` can be used, or `get_size()` which will return `0` if it's not a collection.

::: warning
This behavior is planned to change with internal exception handling
:::
