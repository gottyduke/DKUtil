#pragma once


#define DKU_E_VERSION_MAJOR     1
#define DKU_E_VERSION_MINOR     0
#define DKU_E_VERSION_REVISION  0


#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>


#include "Logger.hpp"
#include "Utility.hpp"

#ifdef DKU_E_DEBUG
#define DKU_DEBUG(...)	DEBUG(__VA_ARGS__)
#else
#define DKU_DEBUG(...)	void(0)
#endif


#define CONSOLE(...)                                          \
	{                                                         \
		if (auto* console = RE::ConsoleLog::GetSingleton()) { \
			auto fmt = fmt::format(__VA_ARGS__);              \
			fmt = PROJECT_NAME + " -> "s + fmt;               \
			console->Print(fmt.c_str());                      \
		}                                                     \
	}


namespace DKUtil
{
} // namespace DKUtil


#ifdef DKU_E_DEBUG
#undef DKU_DEBUG
#endif
