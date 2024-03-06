# Relocation

Replace `call/jmp` instruction with a hook.

## Syntax

```cpp
RelHookHandle Hook::AddRelHook<N, bool>(address, function);
```

## Parameter

+ `N` : length of target instruction, e.g. `call Game.exe+0x1999DF` is `E8 0C 77 FA FF`, which is 5 bytes in length. `FF 15 12 34 56 78` is 6 bytes.
+ `bool` : whether to return or not, `call` returns, `jmp` branches thus no return.
+ `address` : target instruction address.
+ `function` : hook function.

## HookHandle

A `RelHookHandle` object will be returned:

```cpp
class RelHookHandle
{
    const std::size_t   OpSeqSize;
    const Imm64         OriginalFunc;
    Imm64               Destination;
    std::vector<OpCode> OldBytes{};
    std::vector<OpCode> Detour{};
};
```

`RelHookHandle` can be implicitly convereted to address of original function.

## Wrapper

For the ease of use, you can use `Hook::write_call<N>` and `Hook::write_branch<N>` for convenience. These wrappers will enable itself and return the original function address. However, you lose control of disabling the hook.

## Example

Given target assembly:

```asm
0x140345675:   mov rcx, rax
0x140345678:   call Game.exe+0x123456
```

We want to replace the `call Game.exe+0x123456` to `call HookFunc`:

::: code-group

```cpp [Class Style]
class Hook
{
    // hook function
    static bool Hook_123456(void* a_instance)
    {
        // do something
        return func(a_instance);
    }

    // original function
    static inline std::add_pointer_t<decltype(Hook_123456)> func;

public:
    static void Install()
    {
        // absolute
        auto addr = 0x7FF712345678;
        // or offset from module base
        auto addr = dku::Hook::Module::get().base() + 0x345678;

        // save original function
        func = dku::Hook::write_call<5>(addr, Hook_123456);
    }
};
```

```cpp [Free Functions]
// forward
bool Hook_123456(void* a_instance);
// original function
std::add_pointer_t<decltype(Hook_123456)> func;

bool Hook_123456(void* a_instance)
{
    // do something
    return func(a_instance);
}

auto addr = dku::Hook::Module::get().base() + 0x345678;
// save original function
func = dku::Hook::write_call<5>(addr, Hook_123456);
```

:::
