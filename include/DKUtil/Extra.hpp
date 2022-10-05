#pragma once


#define DKU_E_VERSION_MAJOR 1
#define DKU_E_VERSION_MINOR 0
#define DKU_E_VERSION_REVISION 0


#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>


#include "Impl/PCH.hpp"
#include "Logger.hpp"
#include "Utility.hpp"


#define CONSOLE(...)                                          \
	{                                                         \
		if (auto* console = RE::ConsoleLog::GetSingleton()) { \
			auto fmt = fmt::format(__VA_ARGS__);              \
			fmt = PROJECT_NAME + " -> "s + fmt;               \
			console->Print(fmt.c_str());                      \
		}                                                     \
	}
