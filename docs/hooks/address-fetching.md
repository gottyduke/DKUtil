# Address Fetching

`DKUtil::Hook` provides some address fetching helpers at runtime.

## Pattern Scan

To do a pattern scan, call:

```cpp
auto* addr = dku::Hook::Assembly::search_pattern<
				  "40 57 " // each pattern is separated by whitespace " "
				  "48 83 EC 30 "
				  "48 8B 0D ?? ?? ?? ?? " // wildcard is ??
				  "48 8B FA "
				  "48 81 C1 D0 00 00 00 "
				  "E8 ?? ?? ?? ?? "
				  "48 8B C8 "
				  "E8 ?? ?? ?? ??">();
INFO("found address at {:X}", AsAddress(addr));

// delayed match
auto TestAlByte = dku::Hook::Assembly::make_pattern<"84 C0">();
auto addr = 0x7FF712345678;
if (TestAlByte.match(addr)) {}
```

::: warning Planned Change
This API has been planned for breaking changes.
:::

## Rip Addressing

To get the actual address of a rip-relative displacement used in an instruction.  

Given target assembly:

```asm
0x141234567:   call [rip + 0x30]
0x14123456D:   lea rax, ds: [rip + 0x1110]
0x141234574:   mov rax, ds: [rip + 0x114514]
```

We want the final address of these rip displacements:

```cpp
std::uintptr_t funcAddr = dku::Hook::GetDisp(0x141234567);
auto actorSingleton = dku::Hook::GetDisp<void**>(0x14123456D);
bool significance = *dku::Hook::GetDisp<bool*>(0x141234574);
```

## Adjust Pointer

Offset a pointer with type cast.

```cpp
// read bool member value at 0x220 from a class pointer
auto& member = *dku::Hook::adjust_pointer<bool>(actor, 0x220);
```

## Module IAT

Get import address of method in a library loaded by module.

```cpp
void* GetImportAddress(
    std::string_view moduleName, 
    std::string_view libraryName, 
    std::string_view importName)
```

## Class VTable

Get the address of n-th function in class virtual function table.

```cpp
size_t n = 0x8;             // get 8th function
Actor* actor = new Actor(); // class pointer, also vptr
auto func = dku::Hook::TblToAbs(actor, n);
```
