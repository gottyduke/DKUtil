# Callsite Logging

Sometimes during plugin development, you may want to track down callers to specific function during runtime, and especially when it's a virtual function/member function, that makes it even harder to find.

Here's a common example of intercepting a virtual function at runtime and log its callers(more specifically, its return address):

## Target Function

```cpp
void TESObjectREFR::SetStartingPosition(const NiPoint3& a_pos); // 0x54 in vtbl
// this function is a virtual member function of class TESObjectREFR, 
// first we need to translate it to __cdecl function:
void SetStartingPosition(TESObjectREFR* a_this, const NiPoint3& a_pos);
// per x64 calling convention, *this pointer is implicitly passed as first argument
```

## Target Assembly

```asm
40:56   | push rsi
57      | push rdi
41:56   | push r14
```

We only need 5 bytes minimum to setup a cave hook, so these three lines are sufficient.

## Example

```cpp
using namespace DKUtil::Alias;

// assume we acquired an instance of the class TESObjectREFR
TESObjectREFR* refr_instance = 0x123456;

class SetStartingPositionEx : 
    public Xbyak::CodeGenerator
{
    // because vptr is at 0x0 of class instance, so a pointer to class is a pointer to vptr
    inline static std::uintptr_t* vtbl{ *std::bit_cast<std::uintptr_t**>(refr_instance) };
    // the index of the target virtual function we want to log in vtbl
    inline static std::size_t index{ 0x54 };

    // actual logging function, we have added third argument, it's the return address/callsite
    static void Intercept_SSP(
        TESObjectREFR* a_this, 
        const RE::NiPoint3& a_pos, 
        std::uintptr_t a_caller = 0) // [!code ++]
    {
        INFO("ret 0x{:X}", dku::Hook::GetRawAddress(a_caller));
    }

public:
    SetStartingPositionEx()
    {
        // move the return address on stack[rsp] to third argument, as per x64 calling convention
        mov(r8, qword[rsp]);
    }

    static void Install() 
    {
        SetStartingPositionEx sse{};
        sse.ready();

        // generates prolog & epilog patches that preserve register values, 
        // so our logging function won't break anything
        auto [ppatch, epatch] = dku::Hook::JIT::MakeNonVolatilePatch(
            {
                dku::Hook::Register::ALL
            });

        // actual prolog patch
        Patch prolog;
        // first we move the third argument
        prolog.Append(sse);
        // then apply the preserve patch
        prolog.Append(ppatch);
        // nothing extra to add for epilog patch, because we didn't break stack balance

        // target function is at index 0x54 in vtbl
        auto addr = vtbl[index];
        auto hook = dku::Hook::AddCaveHook(
            addr,
            { 0, 5 },
            FUNC_INFO(Intercept_SSP), 
            &prolog, 
            &epatch, 
            // important that we restore original function prolog(the 5 bytes we took) after returning from logger
            HookFlag::kRestoreAfterEpilog);
        hook->Enable();
    }
};
```

## Custom Prolog/Epilog

When composing arguments for custom cave functions, do follow [x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170).
