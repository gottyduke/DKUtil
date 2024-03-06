# Trampoline

A trampoline is often used internally, it leverages many limitations in memory related operations, common scenarios are:

+ Detouring a function call, because of rip addressing mode is within -/+2GiB range, DKUtil will utilize a trampoline to extend the call indirection.
+ Writing assembly patches, trampoline serves as a block of memory to put assembly patches in.
+ Allocating custom data.

## Workflow

`Target Code Region` <-**within -/+2GiB**-> `Trampoline` <-> `Our Code Region`

## Allocating Trampoline

Before committing **any** action that requires a trampoline (e.g. detour hooks, cave hooks, asm patches with exceeding sizes), `DKUtil::Hook` requires a manual `DKUtil::Hook::Trampoline::AllocTrampoline(size)` call to initiate a page allocation for trampoline to use. This should be called **only once**, with sufficient size.

```cpp
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());

    DKUtil::Hook::Trampoline::AllocTrampoline(1 << 10); // [!code focus]

    Configs::Load();
    Hooks::Install();
}
```

::: warning Planned Change
This API has been planned for breaking changes that will auto allocate trampoline and extend its size accordingly.
:::

## Determine Proper Size

Allocating large enough memory for trampoline is good to begin with, the actual trampoline usage are always logged for debug builds, from there an appropriate sufficient size can be determined for release builds. 

## Allocate Memory

To allocate memory **from** trampoline:

```cpp
auto& trampoline = dku::Hook::Trampoline::GetTrampoline();

struct CustomData;
CustomData* data = trampoline.allocate<CustomData>();
void* sized_data = trampoline.allocate(0x100);
```

## SKSE / SFSE / F4SE

For SKSE, SFSE, and F4SE plugin projects, `DKUtil::Hook` will use commonlib's trampoline interface instead of its internal trampoline. However, `CommonLib::AllocTrampoline` still needs to be called.
