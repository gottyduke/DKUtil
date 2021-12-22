#include "DKUtil/Config.hpp"


namespace Test::Config
{
	using namespace DKUtil::Alias;


	static Integer iA{ "iAwesome", "Awesome" };
	static String sA{ "sAwesome", "Awesome" };
	static Boolean bA{ "bAwesome", "Awesome" };
	static Double dA{ "dAwesome" };


	void Load()
	{
		auto Main = COMPILE_PROXY("DKUtilDebugger.toml"sv);

		Main.Bind(iA, 10);
		Main.Bind(sA, "First", "Second", "Third");
		Main.Bind(bA, true);
		Main.Bind(dA, 114.514);

		Main.Load();

		DEBUG("{} {} {} {}", *iA, *sA, *bA, *dA);
		DEBUG("{} {} {}", sA[0], sA[1], sA[2]);

		std::string someRandomeName = DKUtil::Config::GetPath("AnotherBiteTheConfig.ini");		
		static auto runtime = RUNTIME_PROXY(someRandomeName);

		//runtimeProxy.Load();
	}
}
