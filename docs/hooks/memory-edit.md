# Memory Editing

`DKUtil::Hook` offers some memory writing methods like other library does.

::: code-group

```cpp [Arbitrary Data]
void WriteData(std::uintptr_t dst, const void* data, const std::size_t size);
```

```cpp [Immediate Value]
// any trivial, integral or standard layout(pod) types
void WriteImm(std::uintptr_t dst, const dku_h_pod_t auto& data);
```

```cpp [DKUtil Patch]
void WritePatch(std::uintptr_t dst, const Patch* patch);
void WritePatch(std::uintptr_t dst, const Xbyak::CodeGenerator* xbyak);
```

:::

## Patch Structure

Anything that has data pointer and data size are accepted. Xbyak is also supported.

`DKUtil::Patch` is a wrapper for data pointer and data size, and provides extended functionalities. 

```cpp
struct Patch
{
    const void*  Data;
    const size_t Size;
    bool         Managed; // ignore unless you know what you are doing
}
```

The `Managed` field indicates whether or not this Patch owns the `Data` resource. This field should not be changed, it's auto set on internal operations.

## Common Practices

::: code-group

```cpp [Hex String]
using namespace DKUtil::Alias;

static constexpr Patch RadiusPatch{
    // lahf
    "\x9F\x50"
    // mov rdx, rdi
    "\x48\x89\xFA"
    // movss xmm2, xmm6
    "\xF3\x0F\x10\xD6"
    // mov r9, rbp
    "\x49\x89\xE9"
    // mov [rsp-0x8], rbx
    "\x48\x89\x5C\x24\xF8",
    17
};
```

```cpp [Read Memory]
using namespace DKUtil::Alias;

Patch RadiusPatch{
    AsPointer(mem_address), // macro provided by DKUtil
    10                      // read 10 bytes from mem_address
};
```

```cpp [Xbyak]
struct MyPatch : 
    public Xbyak::CodeGenerator
{
    MyPatch()
    {
        lahf();
        mov(rdx, rdi);
        movss(xmm2, xmm6);
        mov(r9, rbp);
        mov(ptr[rsp-0x8], rbx);
    }
};
```

```cpp [Raw Patch]
std::array<std::uint8_t, 5> RawPatch{ 0x48, 0x8B, 0x4C, 0x24, 0x20 };
```

:::

## Combining Patches

```cpp
Patch prolog;
Patch epilog;
Patch extra;
prolog.Append(epilog).Append(extra);
```
