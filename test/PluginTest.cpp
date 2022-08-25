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
	void MsgCallback(F4SE::MessagingInterface::Message* a_msg) noexcept
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


DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_F4SE)
{
	INFO("{} v{} loaded", Version::PROJECT, Version::NAME);

	F4SE::Init(a_F4SE);


	const auto* messaging = F4SE::GetMessagingInterface();
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
