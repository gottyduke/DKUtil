# CaveHook

Detour to hook function from target memory.

## Syntax

```cpp
CaveHookHandle AddCaveHook(
    std::uintptr_t address,
    offset_pair offset,
    FuncInfo funcInfo,
    Patch* prolog = nullptr,
    Patch* epilog = nullptr,
    HookFlag flag = HookFlag::kSkipNOP
);
```

## Parameter

+ `address` : target instruction address.
+ `offsets` : pair containing the {begin, end} offsets from target instruction to patch.
+ `funcInfo` : FUNC_INFO macro wrapper of hook function.
+ `prolog` : **optional**, memory patch **before** detouring to hook function, `nullptr` if none.
+ `epilog` : **optional**, memory patch **after** returning from hook function, `nullptr` if none.
+ `flag` : **optional**, special flag for cave hook operation.

## HookFlag

```cpp
enum class HookFlag
{
    kNoFlag = 0,                     // default

    kSkipNOP = 1u << 0,              // skip NOPs
    kRestoreBeforeProlog = 1u << 1,  // apply stolens before prolog
    kRestoreAfterProlog = 1u << 2,   // apply stolens after prolog
    kRestoreBeforeEpilog = 1u << 3,  // apply stolens before epilog
    kRestoreAfterEpilog = 1u << 4,   // apply stolens after epilog
};
```

You can set multiple flags by passing `{HookFlag, HookFlag, ...}`.

## HookHandle

A `CaveHookHandle` object will be returned:

```cpp
class CaveHookHandle
{
    const offset_pair    Offset;
    const std::size_t    CaveSize;
    const std::uintptr_t CaveEntry;
    std::uintptr_t       CavePtr{ 0x0 };
    std::vector<OpCode>  OldBytes{};
    std::vector<OpCode>  CaveBuf{};
};
```

## Workflow

CaveHook will first fill the target block of memory defined by `address` + `{offset_pair}` with `NOP`, then write its detour into trampoline, which then applies user prolog patch(if any), detour to user function, applies user epilog patch(if any), then return to original block. Original bytes of target block can will be applied depending on the `HookFlag` parameter(if set).

## Example

```cpp
using namespace DKUtil::Alias;

// hook function
float Hook_MyAwesomeFunc(int a_awesomeInt) {
    // do awesome stuff
    return static_cast<float>(a_awesomeInt);
}

std::uintptr_t funcAddr = 0x7FF712345678;
// or offset from module base
std::uintptr_t funcAddr = dku::Hook::Module::get().base() + 0x345678;

// mark the begin and the end of target code to patch
// starts at funcAddr + 0x120
// ends at funcAddr + 0x130
auto offset = std::make_pair(0x120, 0x130);

// this is DKUtil::Hook::Patch, you can also use xbyak or raw patch
// move return value to xmm3
DKUtil::Hook::Patch Epilog = {
    "\x0F\x10\xD8", // movups xmm3, xmm0
    0x3 // size of patch
};

auto Hook_MAF = dku::Hook::AddCaveHook(
    funcAddr, 
    offset, 
    FUNC_INFO(Hook_MyAwesomeFunc), 
    nullptr, 
    &Epilog, 
    dku::HookFlag::kRestoreAfterEpilog);

Hook_MAF->Enable();
```

::: details Workflow for This Example
Under the hood it'll `NOP` all bytes from `funcAddr + 0x120` to `funcAddr + 0x130`, set a branch call to `Hook_MyAwesomeFunc`, apply custom epilog patch, apply original bytes taken from 0x120 to 0x130(`kRestoreAfterEpilog`), then finally return to `funcAddr + 0x130`.
:::

## Custom Prolog/Epilog

When composing arguments for custom cave functions, do follow [x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170).
