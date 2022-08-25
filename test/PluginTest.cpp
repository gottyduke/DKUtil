#include "ConfigTest.h"
#include "HookTest.h"
#include "LoggerTest.h"
#include "UtilityTest.h"

#include <atomic>
#include <future>
#include <chrono>
#include <thread>


//#define TEST_CONFIG
//#define TEST_HOOK
//#define TEST_LOGGER
//#define TEST_UTILITY

namespace
{
	void MsgCallback(SKSE::MessagingInterface::Message* a_msg) noexcept
	{
#ifdef TEST_CONFIG

#endif

#ifdef TEST_HOOK

#endif

#ifdef TEST_LOGGER

#endif
	
#ifdef TEST_UTILITY

#endif
	}
}


DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept
{
	SKSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesAddressLibrary(true);

	return data;
}();


DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
    pluginInfo->name = SKSEPlugin_Version.pluginName;
    pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
    pluginInfo->version = SKSEPlugin_Version.pluginVersion;

    return true;
}


DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());

	SKSE::Init(a_skse);
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	const auto* messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MsgCallback);


#ifdef TEST_CONFIG

	Test::Config::Load();

#endif

#ifdef TEST_GUI

	Test::GUI::Install();

#endif

#ifdef TEST_HOOK

#endif

#ifdef TEST_LOGGER

#endif

#ifdef TEST_UTILITY

	Test::Utility::StartTest();

#endif


	return true;
}
