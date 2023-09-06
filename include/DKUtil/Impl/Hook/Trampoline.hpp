#pragma once


#include "shared.hpp"


namespace DKUtil::Hook::Trampoline
{
	// partially taken from CommonLibSSE
	class Trampoline : public model::Singleton<Trampoline>
	{
	public:
		// https://stackoverflow.com/a/54732489/17295222
		std::byte* PageAlloc(const std::size_t a_size, std::uintptr_t a_from = 0) noexcept
		{
			release();

			constexpr std::size_t gigabyte = static_cast<std::size_t>(1) << 30;
			constexpr std::size_t minRange = gigabyte * 2;
			constexpr std::uintptr_t maxAddr = std::numeric_limits<std::uintptr_t>::max();

			::DWORD granularity;
			::SYSTEM_INFO si;
			::GetSystemInfo(&si);
			granularity = si.dwAllocationGranularity;

			if (!a_from) {
				const auto textx = Module::get().section(Module::Section::textx);
				a_from = textx.first + textx.second;
			}

			std::uintptr_t min = a_from >= minRange ? numbers::roundup(a_from - minRange, granularity) : 0;
			const std::uintptr_t max = a_from < (maxAddr - minRange) ? numbers::rounddown(a_from + minRange, granularity) : maxAddr;
			std::uintptr_t addr = 0;

			::MEMORY_BASIC_INFORMATION mbi;
			do {
				if (!::VirtualQuery(AsPointer(min), &mbi, sizeof(mbi))) {
					break;
				}

				min = AsAddress(mbi.BaseAddress) + mbi.RegionSize;

				if (mbi.State == MEM_FREE) {
					addr = numbers::roundup(AsAddress(mbi.BaseAddress), granularity);

					if (addr < min && a_size <= (min - addr)) {
						if (auto* data = ::VirtualAlloc(AsPointer(addr), a_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) {
							_capacity = a_size;
							_data = static_cast<std::byte*>(data);
							break;
						}
					}
				}

			} while (min < max);

			if (!_data || !_capacity) {
				release();
				FATAL("DKU_H: PageAlloc failed with code: 0x{:08X}"sv, ::GetLastError());
			}

			return _data;
		}

		void set_trampoline(void* a_mem, std::size_t a_size)
		{
			release();

			_data = static_cast<std::byte*>(a_mem);
			_capacity = a_size;
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
		return *Trampoline::GetSingleton();
	}

	inline Trampoline& AllocTrampoline(std::size_t a_size)
	{
		auto& trampoline = GetTrampoline();
		if (!trampoline.capacity()) {
			trampoline.release();
			trampoline.PageAlloc(a_size);
		}
		return trampoline;
	}

	inline void* Allocate(std::size_t a_size)
	{
		auto& trampoline = GetTrampoline();
		return trampoline.allocate(a_size);
	}
}  // namespace DKUtil::Hook
