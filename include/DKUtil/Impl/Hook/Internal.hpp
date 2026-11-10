#pragma once

#include "assembly.hpp"
#include "jit.hpp"
#include "trampoline.hpp"

namespace DKUtil::Hook
{
	using namespace Assembly;

	class HookHandle
	{
	public:
		virtual ~HookHandle() = default;

		virtual void Enable() noexcept = 0;
		virtual void Disable() noexcept = 0;

		template <std::derived_from<HookHandle> derived_t>
		constexpr derived_t* As() noexcept
		{
			return dynamic_cast<derived_t*>(this);
		}

		// write directly to internal trampoline ptr
		template <typename T>
			requires(!std::is_pointer_v<T>)
		void Write(T a_in) noexcept
		{
			Write(std::addressof(a_in), sizeof(a_in));
		}

		void Write(const void* a_src, std::size_t a_size) noexcept
		{
			const auto next = TRAM_ALLOC(0);

			if (!TramPtr) {
				TramPtr = next;
			}

			dku_assert(TramPtr == next,
				"DKU_H: HookHandle can only write at the end of the used trampoline memory\n"
				"handle : {:X}\nnext   : {:X}",
				TramPtr, next);

			WriteData(TramPtr, a_src, a_size, true);
			TramPtr += a_size;
		}

		const std::uintptr_t Address;
		const std::uintptr_t TramEntry;
		std::uintptr_t       TramPtr{ 0x0 };

	protected:
		HookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry) :
			Address(a_address), TramEntry(a_tramEntry), TramPtr(a_tramEntry)
		{}
	};
}  // namespace DKUtil::Hook

#define DKU_H_INTERNAL_IMPORTS DKU_H_VERSION_MAJOR

#include "Internal/ASMPatch.hpp"
#include "Internal/CaveHook.hpp"
#include "Internal/DetourHook.hpp"
#include "Internal/IATHook.hpp"
#include "Internal/RelHook.hpp"
#include "Internal/VMTHook.hpp"

#undef DKU_H_INTERNAL_IMPORTS
