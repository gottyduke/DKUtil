[YouCanSleepRemake](https://github.com/gottyduke/YouCanSleepRemake)

::: code-group

```cpp [Config.h]
#pragma once
#include "DKUtil/Config.hpp"

namespace Config
{
	using namespace DKUtil::Alias;

	extern Boolean EnableSleepWait[8];

	void Load();
}
```

```cpp [Config.cpp]
#include "Config.h"

namespace Config
{
	Boolean EnableSleepWait[8] = {
		{ "InAir" },
		{ "Trespassing" },
		{ "AskedToLeave" },
		{ "GuardsPursuing" },
		{ "EnemiesNearby" },
		{ "TakingHealthDamage" },
		{ "Owned" },
		{ "InUse" },
	};

	void Load()
	{
		auto main = COMPILE_PROXY("YouCanSleepRemake.toml"sv);

		for (auto index = 0; index < std::extent_v<decltype(EnableSleepWait)>; ++index) {
			main.Bind(EnableSleepWait[index], true);
		}

		main.Load();

		INFO("Config Loaded"sv);
	}
}
```

```cpp [Usage]
std::ptrdiff_t OffsetTbl[8] = {
    0x2E,                        // You cannot sleep in the air.
    0x89,                        // You cannot sleep while trespassing.
    0xB1,                        // You cannot sleep while being asked to leave.
    0xF6,                        // You cannot sleep while guards are pursuing you.
    0x11F,                       // You cannot sleep when enemies are nearby.
    0x146,                       // You cannot sleep while taking health damage.
    0x1BB,                       // This object is already in use by someone else.
    REL::Relocate(0x3BC, 0x3C0)  // You cannot sleep in an owned bed.
};

const auto funcAddr = DKUtil::Hook::IDToAbs(AE_FuncID, SE_FuncID);

for (auto index = 0; index < std::extent_v<decltype(Config::EnableSleepWait)>; ++index) {
    if (*Config::EnableSleepWait[index]) { // [!code focus]
        DKUtil::Hook::WriteImm(funcAddr + OffsetTbl[index], JmpShort); // [!code focus]
        INFO("Enabled SleepWait {} at 0x{:X}", Config::EnableSleepWait[index].get_key(), OffsetTbl[index]); // [!code focus]
    }
}

// loop check for in air position
if (*Config::EnableSleepWait[0]) { // [!code focus]
    DKUtil::Hook::WriteData(funcAddr + InAirLoopOffset, &InAirHookNop, sizeof(InAirHookNop));
}

INFO("Hooks installed"sv);
```

:::
