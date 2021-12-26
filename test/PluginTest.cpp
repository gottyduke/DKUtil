#include "GUITest.h"
#include "ConfigTest.h"
#include "HookTest.h"
#include "LoggerTest.h"
#include "UtilityTest.h"

#include <atomic>
#include <future>
#include <chrono>
#include <thread>


//#define TEST_CONFIG
#define TEST_GUI
//#define TEST_HOOK
//#define TEST_LOGGER
//#define TEST_UTILITY


#if ANNIVERSARY_EDITION
 
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
{
	SKSE::PluginVersionData data{};

	data.PluginVersion(Version::MAJOR);
	data.PluginName(Version::NAME);
	data.AuthorName("Dropkicker"sv);

	data.CompatibleVersions({ SKSE::RUNTIME_LATEST });
	data.UsesAddressLibrary(true);

	return data;
}();

#else

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	DKUtil::Logger::Init(Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		ERROR("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		ERROR("Unable to load this plugin, incompatible runtime version!\nExpected: Newer than 1-5-39-0 (A.K.A Special Edition)\nDetected: {}", ver.string());
		return false;
	}
	
	return true;
}

#endif


namespace
{
	void MsgCallback(SKSE::MessagingInterface::Message* a_msg) noexcept
	{
#ifdef TEST_CONFIG

#endif

#ifdef TEST_GUI
		if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame) {
			Test::GUI::Start();
		}
#endif

#ifdef TEST_HOOK

#endif

#ifdef TEST_LOGGER

#endif
	
#ifdef TEST_UTILITY
#endif
	}
}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#if ANNIVERSARY_EDITION

	DKUtil::Logger::Init(Version::PROJECT, Version::NAME);

	if (REL::Module::get().version() < SKSE::RUNTIME_1_6_317) {
		ERROR("Unable to load this plugin, incompatible runtime version!\nExpected: Newer than 1-6-317-0 (A.K.A Anniversary Edition)\nDetected: {}", REL::Module::get().version().string());
		return false;
	}

#endif

	INFO("{} v{} loaded", Version::PROJECT, Version::NAME);

	SKSE::Init(a_skse);


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
