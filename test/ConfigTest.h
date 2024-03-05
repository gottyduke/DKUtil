#if defined(NDEBUG)
#	define BUILD_TYPE "Release"
#else
#	define BUILD_TYPE "Debug"
#endif

#define CONFIG_ENTRY "..\\..\\bin\\" BUILD_TYPE

#include "DKUtil/Config.hpp"

namespace Test::Config
{
	using namespace DKUtil::Alias;

	void TestConfig()
	{
		static Integer iA{ "iAwesome", "Awesome" };
		static String  sA{ "sAwesome", "Awesome" };
		static Boolean bA{ "bAwesome", "Awesome" };
		static Double  dA{ "dAwesome", "Awesome" };

		auto MainIni = COMPILE_PROXY("DKUtilDebugger.ini"sv);
		auto MainJson = COMPILE_PROXY("DKUtilDebugger.json"sv);
		auto MainToml = COMPILE_PROXY("DKUtilDebugger.toml"sv);

		MainToml.Bind(iA, 10);
		MainToml.Bind(sA, "First", "Second", "Third");
		MainToml.Bind(bA, true);
		MainToml.Bind(dA, 114.514);

		MainIni.Load();
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

		auto f = dku::Config::GetAllFiles<true>({}, ".ini"sv);
		for (auto& fi : f) {
			INFO("File -> {}", fi);
		}
	}

	struct CustomData
	{
		dku::numbers::hex form;
		std::string   name;
		std::string   payload;
		bool          excluded;
	};

	void TestSchema()
	{
		auto sc = SCHEMA_PROXY("Schema_SC.txt");
		sc.Load();
		auto& p = sc.get_parser();

		auto d = p.ParseNextLine<CustomData>("|").value();
		INFO("{} {} {} {}", d.form, d.name, d.payload, d.excluded);

		auto d2 = dku::Config::ParseSchemaString<CustomData>("0x1234|BUSTED|random payload |true", "|");
		INFO("{} {} {} {}", d2.form, d2.name, d2.payload, d2.excluded);
	}

	void Run()
	{
		//TestConfig();
		TestSchema();
	}
}  // namespace Test::Config
