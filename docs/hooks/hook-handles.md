# Hook Handles

`HookHandle` class is the base object of a successful `DKUtil::Hook` operation, and it's used to access the hook specific data and enable/disable the hook.

```cpp
class HookHandle
{
    const std::uintptr_t Address;
    const std::uintptr_t TramEntry;
    std::uintptr_t       TramPtr{ 0x0 };
};
```

## Control

`HookHandle` can be used to enable/disable a hook.

```cpp
HookHandle handle = SomeDKUtilHookAPI();

// hook control
handle->Enable();
handle->Disable();
```

## Derived Cast

To cast into derived types of specific hook API:

```cpp
HookHandle handle = SomeDKUtilHookAPI();
ASMPatchHandle asmHandle = handle->As<ASMPatchHandle>();
```

This is useful when you only declare a base `HookHandle` object before commiting any hook operation, then downcast it.

For related information, see each API's own section.

## Internal Trampoline

You can write data directly to `HookHandle` internal `TramPtr`, this normally points to next available memory address in trampoline. However, unless you know what you are doing, you shouldn't be doing it.

```cpp
HookHandle handle = SomeDKUtilHookAPI();
handle->Write(0x100); // write Imm32 0x100 in trampoline
```
