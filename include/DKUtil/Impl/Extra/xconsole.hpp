#pragma once

#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"


#define CONSOLE(...)                                                          \
	{                                                                         \
		if (auto* console = RE::ConsoleLog::GetSingleton()) {                 \
			auto fmt = fmt::format(__VA_ARGS__);                              \
			INFO("[console] {}", fmt);                                        \
			console->Print(fmt::format("{}->{}", PROJECT_NAME, fmt).c_str()); \
		}                                                                     \
	}
