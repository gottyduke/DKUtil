#include "ConfigTest.h"
#include "HookTest.h"
#include "LoggerTest.h"
#include "UtilityTest.h"
#include "SSEExtraTest.h"

#define TEST_CONFIG
#define TEST_HOOK
#define TEST_LOGGER
#define TEST_UTILITY
#define TEST_CUSTOM
#define TEST_AND_EXIT

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
/**/


int main()
{
#ifdef TEST_CONFIG
	__do_test_run(Config);
#endif

#ifdef TEST_HOOK
	__do_test_run(Hook);
#endif

#ifdef TEST_LOGGER
	__do_test_run(Logger);
#endif

#ifdef TEST_UTILITY
	__do_test_run(Utility);
#endif

#ifdef TEST_CUSTOM
	__do_test_run(Extra);
#endif

#ifdef TEST_AND_EXIT
	std::exit('EXIT');
#endif
}


DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	DKUtil::Logger::Init(Plugin::NAME, REL::Module::get().version().string());

	SKSE::Init(a_skse);
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	SKSE::GetMessagingInterface()->RegisterListener(MsgCallback);

	main();

	return true;
}
