#include "DKUtil/Config.hpp"


namespace Test::Config
{
	void Load()
	{
		auto compileTimeIniProxy = GET_PROXY("DKUtilDebugger.ini"sv);
		compileTimeIniProxy.Load();

		//auto RunTimeIniProxy = GET_RUNTIME_PROXY("AnotherBiteTheConfig.ini");
		//RunTimeIniProxy.Load();
	}
}
