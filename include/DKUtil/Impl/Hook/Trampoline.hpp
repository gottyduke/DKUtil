#pragma once


#include "Shared.hpp"


namespace DKUtil::Hook::Trampoline
{
	// partially taken from CommonLibSSE
	class Trampoline : model::Singleton<Trampoline>
	{
	public:
		// https://stackoverflow.com/a/54732489/17295222
		static void* PageAlloc(const std::size_t a_size, const std::uintptr_t a_from = 0) noexcept
		{
			static std::uint32_t dwAllocationGranularity;

			if (!dwAllocationGranularity) {
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				dwAllocationGranularity = si.dwAllocationGranularity;
			}

			std::uintptr_t min, max, addr, add = static_cast<uintptr_t>(dwAllocationGranularity) - 1, mask = ~add;

			min = a_from >= 0x80000000 ? (a_from - 0x80000000 + add) & mask : 0;
			max = a_from < (std::numeric_limits<uintptr_t>::max() - 0x80000000) ? (a_from + 0x80000000) & mask : std::numeric_limits<uintptr_t>::max();

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


		[[nodiscard]] constexpr bool empty() const noexcept { return _capacity == 0; }
		[[nodiscard]] constexpr std::size_t capacity() const noexcept { return _capacity; }
		[[nodiscard]] constexpr std::size_t consumed() const noexcept { return _used; }
		[[nodiscard]] constexpr std::size_t reserved() const noexcept { return _capacity - _used; }

	private:
		std::byte* _data{ nullptr };
		std::size_t _capacity{ 0 };
		std::size_t _used{ 0 };
	};
}  // namespace DKUtil::Hook
