# Assembly Patch

Replace a block of target memory with patch.

## Syntax

```cpp
ASMPatchHandle AddASMPatch(
    std::uintptr_t address,
    offset_pair offsets,
    Patch* patch,
    bool forward = true
);
```

## Paramter

+ `address` : target instruction address.
+ `offsets` : pair containing the {begin, end} offsets from target instruction to patch.
+ `patch` : pointer to the memory patch data structure(see also: [patch structure](memory-edit#patch-structure)).
+ `forward` : **optional**, whether to skip the rest of `NOP` fillers.

## HookHandle

A `ASMPatchHandle` object will be returned:

```cpp
class ASMPatchHandle
{
    const offset_pair   Offset;
    const std::size_t   PatchSize;
    std::vector<OpCode> OldBytes{};
    std::vector<OpCode> PatchBuf{};
};
```

## Example

Prepare the hook:

```cpp
using namespace DKUtil::Alias;

std::uintptr_t mem_addr = 0x7FF712345678;
// or offset from module base
std::uintptr_t mem_addr = dku::Hook::Module::get().base() + 0x345678;
```

Mark the begin and the end of target code region to patch:

```cpp
// starts at mem_addr + 0x120
// ends at mem_addr + 0x130
// entire memory region size to patch is 0x10
auto offset = std::make_pair(0x120, 0x130);
```

::: tip Offset Pair
Sometimes your patch begins at your memory address, which is a pair of `{0x0, size}`.
:::

Commit the hook:

::: code-group

```cpp [Raw Patch]
OpCode AsmSrc[]{
    0xB8,                   // mov eax,
    0x00, 0x00, 0x00, 0x00, // Imm32
    0x89, 0XC1,             // mov ecx, eax
};

auto Hook = DKUtil::Hook::AddASMPatch(funcAddr, offset, { &AsmPatch, sizeof(AsmSrc) });
Hook->Enable();
```

```cpp [DKUtil Patch]
Patch AsmSrc{
    "\xB8"             // mov eax
    "\x00\x00\x00\x00" // Imm32
    "\x89\xC1",        // mov ecx eax
    7
};

auto Hook = DKUtil::Hook::AddASMPatch(funcAddr, offset, &AsmSrc);
Hook->Enable();
```

```cpp [Xbyak]
struct ChangeEcxPatch : 
    public Xbyak::CodeGenerator
{
    ChangeEcxPatch()
    {
        mov(eax, static_cast<Imm32>(0x0));
        mov(ecx, eax);
    }
};

ChangeEcxPatch patch{};
patch.ready();

auto Hook = DKUtil::Hook::AddASMPatch(funcAddr, offset, &patch);
Hook->Enable();
```

:::

## Auto Trampoline

If the given target memory region size defined by `offsets` is less than the size of assembly patch, a trampoline will be utilized to fulfill the patch and setup the auto detour/return. This action requires a minimal target memory space of `0x5`.  
