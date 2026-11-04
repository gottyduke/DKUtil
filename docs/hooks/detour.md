# Function Detour

Detour a function at its entry to a hook function, while keeping the original function callable.

This hooks every caller at once, so it suits targets with too many callsites to relocate one by one with [`write_call`](relocation).

## Syntax

```cpp
DetourHookHandle Hook::AddDetourHook<N>(address, function);
```

## Parameter

+ `N` : length of the stolen prologue, at least 5 bytes, and it must end on an instruction boundary. e.g. `mov [rsp+8], rbx` is `48 89 5C 24 08`, which is 5 bytes. `push rbx` + `sub rsp, 20h` is `40 53` + `48 83 EC 20`, which is 6 bytes.
+ `address` : address of the first instruction of the target function.
+ `function` : hook function, it should have the same signature as the target function.

::: warning Stolen Prologue
The first `N` bytes are copied verbatim into the trampoline, so they must not contain any rip-relative operand (`mov rax, [rip+disp]`, `lea rcx, [rip+disp]`, etc.) or relative branch (`call`, `jmp`, `jcc`). DKUtil does not disassemble the prologue: pick `N` from the disassembly, the debug log prints the stolen bytes.

As a sanity check, a head that already starts with a branch (e.g. hooked by another plugin, or an incremental linking thunk), a `ret` or an `int3` is refused.
:::

## HookHandle

A `DetourHookHandle` object will be returned:

```cpp
class DetourHookHandle
{
    const std::size_t   StolenSize;
    const Imm64         OriginalFunc;
    Imm64               Destination;
    std::vector<OpCode> OldBytes{};
    std::vector<OpCode> Detour{};
};
```

`DetourHookHandle` can be implicitly converted to address of original function. Like other hook APIs, the hook is disabled until `Enable()` is called, `Disable()` restores the stolen prologue. The original function stays callable either way.

## Workflow

```asm
; target function, N bytes
jmp relay                       ; E9 rel32, padded with NOP

; trampoline, N + 19 bytes
dq  function
relay:                          ; TramEntry
jmp qword ptr [rip - 0xE]       ; -> function
original:                       ; OriginalFunc
<N stolen bytes>
jmp target + N                  ; E9 rel32
```

Each detour consumes `N + 19` bytes of [trampoline](trampoline).

## Wrapper

For the ease of use, you can use `Hook::write_detour<N>` for convenience. This wrapper will enable itself and return the original function address, same as `write_call`.

## Example

Given target function:

```asm
0x140345670:   mov [rsp+8], rbx     ; 48 89 5C 24 08
0x140345675:   push rdi             ; 57
0x140345676:   sub rsp, 20h         ; 48 83 EC 20
```

We want every call to `Game.exe+0x345670` to go through `Hook_345670`:

::: code-group

```cpp [Class Style]
class Hook
{
    // hook function
    static void* Hook_345670(void* a_this, void* a_arg)
    {
        // do something
        return func(a_this, a_arg);
    }

    // original function
    static inline std::add_pointer_t<decltype(Hook_345670)> func;

public:
    static void Install()
    {
        auto addr = dku::Hook::Module::get().base() + 0x345670;

        // steal mov [rsp+8], rbx, save original function
        func = dku::Hook::write_detour<5>(addr, Hook_345670);
    }
};
```

```cpp [Hook Handle]
// forward
void* Hook_345670(void* a_this, void* a_arg);
// original function
std::add_pointer_t<decltype(Hook_345670)> func;

auto addr = dku::Hook::Module::get().base() + 0x345670;

// steal mov [rsp+8], rbx + push rdi
auto handle = dku::Hook::AddDetourHook<6>(addr, AsAddress(&Hook_345670));
// save original function
func = *handle;

handle->Enable();
// toggle at will
handle->Disable();
```

:::

::: tip Patching
The function head is patched with a plain memory write, install the detour when no other thread can be executing the target function, e.g. during plugin load.
:::
