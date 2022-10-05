#pragma once


#include "Shared.hpp"


// CommonLib - DKUtil should only be integrated into project using the designated SKSEPlugins/F4SEPlugins workspace
#ifndef TRAMPOLINE

#	if defined(F4SEAPI)
#		include "F4SE/API.h"
#	elif defined(SKSEAPI)
#		include "SKSE/API.h"

#		define IS_AE REL::Module::IsAE()
#		define IS_SE REL::Module::IsSE()
#		define IS_VR REL::Module::IsVR()

#	else
#		error "Neither CommonLib nor custom TRAMPOLINE defined"
#	endif

#	define TRAMPOLINE SKSE::GetTrampoline()
#	define TRAM_ALLOC(SIZE) AsAddress((TRAMPOLINE).allocate((SIZE)))
#	define PAGE_ALLOC(SIZE) SKSE::AllocTrampoline((SIZE))

#endif


namespace DKUtil::Hook::Trampoline
{
	// https://stackoverflow.com/a/54732489/17295222
	inline void* Allocate2GBRange(const std::uintptr_t a_address, const std::size_t a_size)
	{
		static std::uint32_t dwAllocationGranularity;

		if (!dwAllocationGranularity) {
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			dwAllocationGranularity = si.dwAllocationGranularity;
		}

		std::uintptr_t min, max, addr, add = dwAllocationGranularity - 1, mask = ~add;

		min = a_address >= 0x80000000 ? (a_address - 0x80000000 + add) & mask : 0;
		max = a_address < (UINTPTR_MAX - 0x80000000) ? (a_address + 0x80000000) & mask : UINTPTR_MAX;

		MEMORY_BASIC_INFORMATION mbi;
		do {
			if (!VirtualQuery(AsPointer(min), &mbi, sizeof(mbi))) {
				return nullptr;
			}

			min = AsAddress(mbi.BaseAddress) + mbi.RegionSize;

			if (mbi.State == MEM_FREE) {
				addr = (AsAddress(mbi.BaseAddress) + add) & mask;

				if (addr < min && a_size <= (min - addr)) {
					if (addr = AsAddress(VirtualAlloc(AsPointer(addr), a_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
						return AsPointer(addr);
					}
				}
			}

		} while (min < max);

		return nullptr;
	}
}  // namespace DKUtil::Hook
