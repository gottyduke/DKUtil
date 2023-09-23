#pragma once

#if defined(SKSEAPI)
#	include "SKSE/API.h"
#	define IS_AE REL::Module::IsAE()
#	define IS_SE REL::Module::IsSE()
#	define IS_VR REL::Module::IsVR()

#	define TRAMPOLINE SKSE::GetTrampoline()
#	define TRAM_ALLOC(SIZE) AsAddress((TRAMPOLINE).allocate((SIZE)))

inline std::uintptr_t IDToAbs([[maybe_unused]] std::uint64_t a_ae, [[maybe_unused]] std::uint64_t a_se, [[maybe_unused]] std::uint64_t a_vr = 0) noexcept
{
	__DEBUG("DKU_H: Attempt to load {} address by id {}", IS_AE ? "AE" : IS_VR ? "VR" :
																				 "SE",
		IS_AE ? a_ae : a_vr ? a_vr :
							  a_se);
	std::uintptr_t resolved = a_vr ? REL::RelocationID(a_se, a_ae, a_vr).address() : REL::RelocationID(a_se, a_ae).address();
	__DEBUG("DKU_H: Resolved: {:X} | Base: {:X} | RVA: {:X}", REL::RelocationID(a_se, a_ae).address(), REL::Module::get().base(), resolved - REL::Module::get().base());

	return resolved;
}

inline offset_pair RuntimeOffset(
	[[maybe_unused]] const std::ptrdiff_t a_aeLow, [[maybe_unused]] const std::ptrdiff_t a_aeHigh,
	[[maybe_unused]] const std::ptrdiff_t a_seLow, [[maybe_unused]] const std::ptrdiff_t a_seHigh,
	[[maybe_unused]] const std::ptrdiff_t a_vrLow = -1, [[maybe_unused]] const std::ptrdiff_t a_vrHigh = -1) noexcept
{
	switch (REL::Module::GetRuntime()) {
	case REL::Module::Runtime::AE:
		{
			return std::make_pair(a_aeLow, a_aeHigh);
		}
	case REL::Module::Runtime::SE:
		{
			return std::make_pair(a_seLow, a_seHigh);
		}
	case REL::Module::Runtime::VR:
		{
			return a_vrLow != -1 ? std::make_pair(a_vrLow, a_vrHigh) : std::make_pair(a_seLow, a_seHigh);
		}
	default:
		{
			ERROR("DKU_H: Runtime offset failed to relocate for unknown runtime!");
		}
	}
}

inline auto RuntimePatch(
	[[maybe_unused]] const Xbyak::CodeGenerator* a_ae,
	[[maybe_unused]] const Xbyak::CodeGenerator* a_se,
	[[maybe_unused]] const Xbyak::CodeGenerator* a_vr = nullptr) noexcept
{
	switch (REL::Module::GetRuntime()) {
	case REL::Module::Runtime::AE:
		{
			return a_ae;
		}
	case REL::Module::Runtime::SE:
		{
			return a_se;
		}
	case REL::Module::Runtime::VR:
		{
			return (a_vr && a_vr->getCode() && a_vr->getSize()) ? a_vr : a_se;
		}
	default:
		{
			ERROR("DKU_H: Runtime patch failed to relocate for unknown runtime!");
		}
	}
}

inline auto RuntimePatch(
	[[maybe_unused]] const Patch* a_ae,
	[[maybe_unused]] const Patch* a_se,
	[[maybe_unused]] const Patch* a_vr = nullptr) noexcept
{
	switch (REL::Module::GetRuntime()) {
	case REL::Module::Runtime::AE:
		{
			return a_ae;
		}
	case REL::Module::Runtime::SE:
		{
			return a_se;
		}
	case REL::Module::Runtime::VR:
		{
			return (a_vr && a_vr->Data && a_vr->Size) ? a_vr : a_se;
		}
	default:
		{
			ERROR("DKU_H: Runtime patch failed to relocate for unknown runtime!");
			return (const Patch*)nullptr;
		}
	}
}

inline const unpacked_data RuntimePatch(
	[[maybe_unused]] unpacked_data a_ae,
	[[maybe_unused]] unpacked_data a_se,
	[[maybe_unused]] unpacked_data a_vr = { nullptr, 0 }) noexcept
{
	switch (REL::Module::GetRuntime()) {
	case REL::Module::Runtime::AE:
		{
			return a_ae;
		}
	case REL::Module::Runtime::SE:
		{
			return a_se;
		}
	case REL::Module::Runtime::VR:
		{
			return (a_vr.first && a_vr.second) ? a_vr : a_se;
		}
	default:
		{
			ERROR("DKU_H: Runtime patch failed to relocate for unknown runtime!");
		}
	}
}
#elif defined(F4SEAPI)
#	include "F4SE/API.h"
#	define TRAMPOLINE F4SE::GetTrampoline()
#	define TRAM_ALLOC(SIZE) AsAddress((TRAMPOLINE).allocate((SIZE)))
#elif defined(SFSEAPI) && !defined(PLUGIN_MODE)
#	include "SFSE/API.h"
#	define TRAMPOLINE SFSE::GetTrampoline()
#	define TRAM_ALLOC(SIZE) AsAddress((TRAMPOLINE).allocate((SIZE)))
#elif defined(PLUGIN_MODE)
namespace Trampoline
{
	extern inline void* Allocate(std::size_t a_size);
}
#	define TRAM_ALLOC(SIZE) AsAddress(Trampoline::Allocate(SIZE))
#endif
