[SF_NativeAutoHDR](https://github.com/ersh1/SF_NativeAutoHDR)

::: code-group

```cpp [Settings.h]
#pragma once
#include "DKUtil/Config.hpp"

namespace Settings
{
    using namespace DKUtil::Alias;

    class Main : public DKUtil::model::Singleton<Main>
    {
    public:
		Integer ImageSpaceBufferFormat{ "ImageSpaceBufferFormat", "Main" };
		Boolean UpgradeUIRenderTarget{ "UpgradeUIRenderTarget", "Main" };
		Integer UpgradeRenderTargets{ "UpgradeRenderTargets", "Main" };

		Integer FrameBufferFormat{ "FrameBufferFormat", "HDR" };

		String RenderTargetsToUpgrade{ "RenderTargetsToUpgrade", "RenderTargets" };

        void Load() noexcept;

    private:
		TomlConfig config = COMPILE_PROXY("NativeAutoHDR.toml"sv);
    };
}
```

```cpp [Settings.cpp]
#include "Settings.h"

namespace Settings
{
	void Main::Load() noexcept
	{
		static std::once_flag ConfigInit;
		std::call_once(ConfigInit, [&]() {
			config.Bind(ImageSpaceBufferFormat, 0);
			config.Bind(UpgradeUIRenderTarget, true);
			config.Bind(UpgradeRenderTargets, 2);
			config.Bind(FrameBufferFormat, 0);
			config.Bind(RenderTargetsToUpgrade, "SF_ColorBuffer", "HDRImagespaceBuffer", "ImageSpaceHalfResBuffer", "ImageProcessColorTarget", "ImageSpaceBufferB10G11R11", "ImageSpaceBufferE5B9G9R9", "TAA_idTech7HistoryColorTarget", "EnvBRDF", "ImageSpaceBufferR10G10B10A2");
		});

		config.Load();

		INFO("Config loaded"sv)
	}
}
```

```cpp [Usage]
const auto settings = Settings::Main::GetSingleton();

if (*settings->FrameBufferFormat == 2) { // [!code focus]
	SetBufferFormat(RE::Buffers::FrameBuffer, RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT);
} else {
	SetBufferFormat(RE::Buffers::FrameBuffer, RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R10G10B10A2_UNORM);
}

switch (*settings->ImageSpaceBufferFormat) { // [!code focus]
case 1:
	SetBufferFormat(RE::Buffers::ImageSpaceBuffer, RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R10G10B10A2_UNORM);
	break;
case 2:
	SetBufferFormat(RE::Buffers::ImageSpaceBuffer, RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT);
	break;
}

if (*settings->UpgradeUIRenderTarget) { // [!code focus]
	SetBufferFormat(RE::Buffers::ScaleformCompositeBuffer, RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT);
}
```

:::
