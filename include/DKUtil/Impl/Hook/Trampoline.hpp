#pragma once


#include "shared.hpp"


namespace DKUtil::Hook::Trampoline
{
	// partially taken from CommonLibSSE
	class Trampoline : public model::Singleton<Trampoline>
	{
	public:
		// https://stackoverflow.com/a/54732489/17295222
		static void* PageAlloc(const std::size_t a_size, std::uintptr_t a_from = 0) noexcept
		{
			static ::DWORD dwAllocationGranularity;

			if (!dwAllocationGranularity) {
				::SYSTEM_INFO si;
				::GetSystemInfo(&si);
				dwAllocationGranularity = si.dwAllocationGranularity;
			}

			std::uintptr_t min, max, addr, add = static_cast<uintptr_t>(dwAllocationGranularity) - 1, mask = ~add;

			if (!a_from) {
				const auto textx = Module::get().section(Module::Section::textx);
				a_from = textx.first + textx.second / 2;
			}

			min = a_from >= 0x80000000 ? (a_from - 0x80000000 + add) & mask : 0;
			max = a_from < (std::numeric_limits<uintptr_t>::max() - 0x80000000) ? (a_from + 0x80000000) & mask : std::numeric_limits<uintptr_t>::max();

			::MEMORY_BASIC_INFORMATION mbi;
			do {
				if (!::VirtualQuery(AsPointer(min), &mbi, sizeof(mbi))) {
					return nullptr;
				}

				min = AsAddress(mbi.BaseAddress) + mbi.RegionSize;

				if (mbi.State == MEM_FREE) {
					addr = (AsAddress(mbi.BaseAddress) + add) & mask;

					if (addr < min && a_size <= (min - addr)) {
						if (addr = AsAddress(::VirtualAlloc(AsPointer(addr), a_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
							return AsPointer(addr);
						}
					}
				}

			} while (min < max);

			return nullptr;
		}

		[[nodiscard]] void* allocate(std::size_t a_size)
		{
			auto result = do_allocate(a_size);
			log_stats();
			return result;
		}

		template <class T>
		[[nodiscard]] T* allocate()
		{
			return static_cast<T*>(allocate(sizeof(T)));
		}

		constexpr void release() noexcept
		{
			_data = nullptr;
			_capacity = 0;
			_used = 0;
		}

		[[nodiscard]] constexpr bool empty() const noexcept { return _capacity == 0; }
		[[nodiscard]] constexpr std::size_t capacity() const noexcept { return _capacity; }
		[[nodiscard]] constexpr std::size_t consumed() const noexcept { return _used; }
		[[nodiscard]] constexpr std::size_t free_size() const noexcept { return _capacity - _used; }

	private:
		[[nodiscard]] void* do_allocate(std::size_t a_size)
		{
			if (a_size > free_size()) {
				FATAL("Failed to handle allocation request");
			}

			auto mem = _data + _used;
			_used += a_size;

			return mem;
		}

		void log_stats() const noexcept
		{
			auto pct = (static_cast<double>(_used) / static_cast<double>(_capacity)) * 100.0;
			DEBUG("Trampoline => {}B / {}B ({:05.2f}%)", _used, _capacity, pct);
		}

		std::byte* _data{ nullptr };
		std::size_t _capacity{ 0 };
		std::size_t _used{ 0 };
	};

	inline Trampoline& GetTrampoline() noexcept
	{
		static Trampoline trampoline;
		return trampoline;
	}

	inline Trampoline& AllocTrampoline(std::size_t a_size)
	{
		auto& trampoline = GetTrampoline();
		trampoline.release();
		trampoline.PageAlloc(a_size);
		return trampoline;
	}
}  // namespace DKUtil::Hook
