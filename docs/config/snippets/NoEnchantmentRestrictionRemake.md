[NoEnchantmentRestrictionRemake](https://github.com/gottyduke/NoEnchantmentRestrictionRemake)

::: code-group

```cpp [Config.h]
#pragma once
#include "DKUtil/Config.hpp"

namespace Config
{
	using namespace DKUtil::Alias;

	extern Boolean EnableDE;
	extern Boolean EnableUE;
	extern Integer UEMax;
	extern String Exclusion;

	void Load();
}
```

```cpp [Config.cpp]
#include "Config.h"

namespace Config
{
	Boolean EnableDE{ "EnableDisenchantEverything" };
	Boolean EnableUE{ "EnableUnlimitedEnchantment" };
	Integer UEMax{ "EnchantmentMaxSlots" };
	String Exclusion{ "ExcludedEnchantments" };

	void Load()
	{
		auto mainConfig = COMPILE_PROXY("NoEnchantmentRestrictionRemake.toml"sv);

		mainConfig.Bind(EnableDE, true);
		mainConfig.Bind(EnableUE, true);
		mainConfig.Bind(UEMax, 3);
		mainConfig.Bind(Exclusion, "0|Example.esp");

		mainConfig.Load();

		INFO("Config Loaded"sv);
	}
}
```

```cpp [Usage]
DKUtil::Hook::WriteImm(_Hook_UES->TramPtr, static_cast<Imm32>(*Config::UEMax)); // [!code focus]

void Validator::ResolveExclusion()
{
    auto* dataHandler = RE::TESDataHandler::GetSingleton();

    for (auto& ex : Config::Exclusion.get_collection()) { // [!code focus]
        auto list = dku::string::split(ex, "|"sv);  // [!code focus]
        if (list.size() < 2) {
            continue;
        }
    }
}
```

:::
