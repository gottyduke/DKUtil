#define CONFIG_ENTRY "D:\\WorkSpace\\SKSEPlugins\\Build\\bin\\Debug"
#include "DKUtil/Config.hpp"


namespace Test::Config
{
	using namespace DKUtil::Alias;


	static Integer iA{ "iAwesome" };
	static String sA{ "sAwesome" };
	static Boolean bA{ "bAwesome" };
	static Double dA{ "dAwesome" };


	void Run()
	{
		auto MainIni = COMPILE_PROXY("DKUtilDebugger.ini"sv);
		auto MainJson = COMPILE_PROXY("DKUtilDebugger.json"sv);
		auto MainToml = COMPILE_PROXY("DKUtilDebugger.toml"sv);

		MainToml.Bind(iA, 10);
		MainToml.Bind(sA, "First", "Second", "Third");
		MainToml.Bind(bA, true);
		MainToml.Bind(dA, 114.514);

		MainToml.Load();
		MainJson.Load();
		MainToml.Load();

		INFO("{} {} {} {}", *iA, *sA, *bA, *dA);
		for (auto f : dA.get_collection()) {
			INFO("{}", f);
		}

		std::string someRandomeName = DKUtil::Config::GetPath("AnotherBiteTheConfig.ini");
		static auto runtime = RUNTIME_PROXY(someRandomeName);

		runtime.Load();
		INFO("ini#{} json#{} toml#{} runtime#{}", MainIni.get_id(), MainJson.get_id(), MainToml.get_id(), runtime.get_id());

		auto f = dku::Config::GetAllFiles({}, ".ini"sv, {}, {}, true);
		for (auto& fi : f) {
			INFO("File -> {}", fi);
		}
	}
}  // namespace Test::Config
