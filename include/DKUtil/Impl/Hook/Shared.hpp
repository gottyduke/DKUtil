#pragma once


#include "DKUtil/Impl/PCH.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"

#include <xbyak/xbyak.h>


#define AsAddress(PTR) std::bit_cast<std::uintptr_t>(PTR)
#define AsPointer(ADDR) std::bit_cast<void*>(ADDR)


#define REQUEST_ALLOC true
#define NO_ALLOC false
#define FORWARD_PTR true
#define NO_FORWARD false

#define NO_PATCH   \
	{              \
		nullptr, 0 \
	}

#define ASM_MINIMUM_SKIP 2
#define CAVE_MINIMUM_BYTES 0x5
#ifndef CAVE_BUF_SIZE 1 << 7
#	define CAVE_BUF_SIZE 1 << 7
#endif


#define FUNC_INFO(FUNC) \
	DKUtil::Hook::FuncInfo { reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::function::GetFuncArgsCount(FUNC), #FUNC }
#define MEM_FUNC_INFO(FUNC) \
	DKUtil::Hook::FuncInfo { reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::function::GetMemFuncArgsCount(FUNC), #FUNC }
#define RT_INFO(FUNC, NAME) \
	DKUtil::Hook::FuncInfo { FUNC, 0, NAME }

namespace DKUtil
{
	namespace Alias
	{
		using OpCode = std::uint8_t;
		using Disp8 = std::int8_t;
		using Disp16 = std::int16_t;
		using Disp32 = std::int32_t;
		using Imm8 = std::uint8_t;
		using Imm16 = std::uint16_t;
		using Imm32 = std::uint32_t;
		using Imm64 = std::uint64_t;
	}  // namesapce Alias


	namespace Hook
	{
		using REX = std::uint8_t;
		using ModRM = std::uint8_t;
		using SIndex = std::uint8_t;

		using unpacked_data = std::pair<const void*, std::size_t>;
		using offset_pair = std::pair<std::ptrdiff_t, std::ptrdiff_t>;


		template <typename data_t>
		concept dku_h_pod_t =
			std::is_integral_v<data_t> ||
			(std::is_standard_layout_v<data_t>&& std::is_trivial_v<data_t>);


		enum class HookFlag : std::uint32_t
		{
			kNoFlag = 0,

			kSkipNOP = 1u << 0,              // skip NOPs
			kRestoreBeforeProlog = 1u << 1,  // apply stolens before prolog
			kRestoreAfterProlog = 1u << 2,   // apply stolens after prolog
			kRestoreBeforeEpilog = 1u << 3,  // apply stolens before epilog
			kRestoreAfterEpilog = 1u << 4,   // apply stolens after epilog
		};


		struct Patch
		{
			const void* Data;
			const std::size_t Size;
		};


		struct FuncInfo
		{
			std::uintptr_t Address;
			std::size_t ArgsCount;
			std::string_view Name;
		};

		using namespace Alias;

#ifdef SKSEAPI

// CommonLib - DKUtil should only be integrated into project using the designated SKSEPlugins/F4SEPlugins workspace
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


		inline std::uintptr_t IDToAbs([[maybe_unused]] std::uint64_t a_ae, [[maybe_unused]] std::uint64_t a_se, [[maybe_unused]] std::uint64_t a_vr = 0) noexcept
		{
			DEBUG("DKU_H: Attempt to load {} address by id {}", IS_AE ? "AE" : IS_VR ? "VR" :
																					   "SE",
				IS_AE ? a_ae : a_vr ? a_vr :
									  a_se);
			std::uintptr_t resolved = a_vr ? REL::RelocationID(a_se, a_ae, a_vr).address() : REL::RelocationID(a_se, a_ae).address();
			DEBUG("DKU_H: Resolved: {:X} | Base: {:X} | RVA: {:X}", REL::RelocationID(a_se, a_ae).address(), REL::Module::get().base(), resolved - REL::Module::get().base());

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
#endif


		inline std::string_view GetProcessName(HMODULE a_handle = 0) noexcept
		{
			static std::string fileName(MAX_PATH + 1, ' ');
			auto res = GetModuleBaseNameA(GetCurrentProcess(), a_handle, fileName.data(), MAX_PATH + 1);
			if (res == 0) {
				fileName = "[ProcessHost]";
				res = 13;
			}

			return { fileName.c_str(), res };
		}


		inline std::pair<std::uintptr_t, std::uintptr_t> GetModuleSectionRange(std::string_view a_section, const char* a_moduleName = nullptr) noexcept
		{
			auto base = AsAddress(GetModuleHandleA(a_moduleName));
			auto* dosHeader = std::bit_cast<const IMAGE_DOS_HEADER*>(base);
			auto* ntHeader = std::bit_cast<const IMAGE_NT_HEADERS64*>(dosHeader + dosHeader->e_lfanew);
			const auto* sections = IMAGE_FIRST_SECTION(ntHeader);

			for (std::size_t i = 0; i < 8; ++i) {
				const auto& section = sections[i];
				constexpr auto size = std::extent_v<decltype(section.Name)>;
				const auto len = std::min(a_section.size(), size);
				INFO("{} {}", i, len);
				if (std::memcmp(a_section.data(), section.Name, len) == 0 &&
					(section.Characteristics & a_section.size()) == a_section.size()) {
					return std::make_pair(base + section.VirtualAddress, base + section.Misc.VirtualSize - 1);
				}
			}

			return std::make_pair(0, 0);
		}


		inline void WriteData(std::uintptr_t& a_dst, const void* a_data, const std::size_t a_size, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
		{
			if (a_requestAlloc) {
				void(TRAM_ALLOC(a_size));
			}

			DWORD oldProtect;

			auto success = VirtualProtect(AsPointer(a_dst), a_size, PAGE_EXECUTE_READWRITE, std::addressof(oldProtect));
			if (success != FALSE) {
				std::memcpy(AsPointer(a_dst), a_data, a_size);
				success = VirtualProtect(AsPointer(a_dst), a_size, oldProtect, std::addressof(oldProtect));
			}

			assert(success != FALSE);

			if (a_forwardPtr) {
				a_dst += a_size;
			}
		}

		inline void WriteData(const std::uintptr_t& a_dst, const void* a_data, const std::size_t a_size, bool a_requestAlloc = NO_ALLOC) noexcept
		{
			return WriteData(const_cast<std::uintptr_t&>(a_dst), a_data, a_size, NO_FORWARD, a_requestAlloc);
		}

		inline void WriteImm(std::uintptr_t& a_dst, const dku_h_pod_t auto& a_data, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
		{
			return WriteData(a_dst, std::addressof(a_data), sizeof(a_data), a_forwardPtr, a_requestAlloc);
		}

		inline void WriteImm(const std::uintptr_t& a_dst, const dku_h_pod_t auto& a_data, bool a_requestAlloc = NO_ALLOC) noexcept
		{
			return WriteData(const_cast<std::uintptr_t&>(a_dst), std::addressof(a_data), sizeof(a_data), NO_FORWARD, a_requestAlloc);
		}

		inline void WritePatch(std::uintptr_t& a_dst, const unpacked_data a_patch, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
		{
			return WriteData(a_dst, a_patch.first, a_patch.second, a_forwardPtr, a_requestAlloc);
		}

		inline void WritePatch(const std::uintptr_t& a_dst, const unpacked_data a_patch, bool a_requestAlloc = NO_ALLOC) noexcept
		{
			return WriteData(const_cast<std::uintptr_t&>(a_dst), a_patch.first, a_patch.second, NO_FORWARD, a_requestAlloc);
		}

		inline void WritePatch(std::uintptr_t& a_dst, const Xbyak::CodeGenerator* a_patch, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
		{
			return WriteData(a_dst, a_patch->getCode(), a_patch->getSize(), a_forwardPtr, a_requestAlloc);
		}

		inline void WritePatch(const std::uintptr_t& a_dst, const Xbyak::CodeGenerator* a_patch, bool a_requestAlloc = NO_ALLOC) noexcept
		{
			return WriteData(const_cast<std::uintptr_t&>(a_dst), a_patch->getCode(), a_patch->getSize(), NO_FORWARD, a_requestAlloc);
		}

		inline void WritePatch(std::uintptr_t& a_dst, const Hook::Patch* a_patch, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
		{
			return WriteData(a_dst, a_patch->Data, a_patch->Size, a_forwardPtr, a_requestAlloc);
		}

		inline void WritePatch(const std::uintptr_t& a_dst, const Hook::Patch* a_patch, bool a_requestAlloc = NO_ALLOC) noexcept
		{
			return WriteData(const_cast<std::uintptr_t&>(a_dst), a_patch->Data, a_patch->Size, NO_FORWARD, a_requestAlloc);
		}

		inline constexpr std::uintptr_t TblToAbs(const std::uintptr_t a_base, const std::uint16_t a_index, const std::size_t a_size = sizeof(Imm64)) noexcept
		{
			return AsAddress(a_base + a_index * a_size);
		}

		template <typename T, typename P>
		constexpr T& AsPun(P* a_pointer) noexcept
		{
			if constexpr (std::is_const_v<P> && std::is_volatile_v<P>) {
				return *std::bit_cast<std::add_cv_t<T>*>(a_pointer);
			} else if constexpr (std::is_const_v<P>) {
				return *std::bit_cast<std::add_const_t<T>*>(a_pointer);
			} else if constexpr (std::is_volatile_v<P>) {
				return *std::bit_cast<std::add_volatile_t<T>*>(a_pointer);
			} else {
				return *std::bit_cast<T*>(a_pointer);
			}
		}

		template <typename T>
		constexpr T& AsPun(const std::uintptr_t a_address) noexcept
		{
			return AsPun<T>(AsPointer(a_address));
		}
	}  // namespace Hook
}  // namespace DKUtil
