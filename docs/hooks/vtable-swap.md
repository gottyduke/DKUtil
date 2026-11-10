# Virtual Method Table Swap

Swaps a virtual method table function with hook function.

## Syntax

```cpp
VMTHookHandle AddVMTHook(
    void* vtbl,
    std::uint16_t index,
    FuncInfo funcInfo,
    Patch* patch = nullptr
);
```

## Parameter

+ `vtbl` : pointer to class object, not the table itself.
+ `index` : **index** of the virtual function in the virtual method table.
+ `funcInfo` : FUNC_INFO macro wrapper of hook function.
+ `patch` : **optional**, stack neutral prolog patch before detouring to hook function.

## HookHandle

A `VMTHookHandle` object will be returned:

```cpp
class VMTHookHandle
{
    template <typename F>
    F GetOldFunction();

    std::uintptr_t OldAddress;
};
```

`OldAddress` is refreshed on `Enable`. `Disable` only restores it while the entry still points to this hook.

## Example

Given base class `Dummy` with two functions:

```cpp
class Dummy
{
public:
    void MsgA() { INFO("Called MsgA"sv); } // 0
    void MsgB() { INFO("Called MsgB"sv); } // 1
};

// target dummy vtbl, it's implicitly at <class pointer + 0x0>
Dummy* dummy = new Dummy();
```

First translate class methods to __cdecl convention:

```cpp
using namespace DKUtil::Alias;

// first parameter is this pointer
using MsgFunc_t = std::add_pointer_t<void(Dummy*)>;
MsgFunc_t oldMsgA;
MsgFunc_t oldMsgB;
```

To swap the functions with our own:

```cpp
// hook function
void MsgC(Dummy* a_this) { 
    INFO("Called MsgC"sv);
    // call original function
    oldMsgA(a_this);
    oldMsgB(a_this);
}

auto Hook_MsgA = dku::Hook::AddVMTHook(dummy, 0, FUNC_INFO(MsgC));
auto Hook_MsgB = dku::Hook::AddVMTHook(dummy, 1, FUNC_INFO(MsgC));

// save original function
oldMsgA = Hook_MsgA->GetOldFunction<MsgFunc>();
oldMsgB = Hook_MsgB->GetOldFunction<MsgFunc>();

Hook_MsgA->Enable();
Hook_MsgB->Enable();
// now MsgA and MsgB will both be detoured to MsgC instead
```
