#pragma once


#include "DKUtil/Impl/pch.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"

#include <xbyak/xbyak.h>


#define AsAddress(PTR) std::bit_cast<std::uintptr_t>(PTR)
#define AsPointer(ADDR) std::bit_cast<void*>(ADDR)


#define NO_PATCH   \
	{              \
		nullptr, 0 \
	}

#define ASM_MINIMUM_SKIP 2
#define CAVE_MINIMUM_BYTES 0x5
#ifndef CAVE_BUF_SIZE 1 << 7
#	define CAVE_BUF_SIZE 1 << 7
#endif

#define ASM_STACK_ALLOC_SIZE 0x20

// concept
template <typename data_t>
concept dku_h_pod_t =
	std::is_integral_v<data_t> ||
	(std::is_standard_layout_v<data_t> && std::is_trivial_v<data_t>);

template <typename mem_t>
concept dku_h_addr_t = std::convertible_to<void*, mem_t> || std::convertible_to<std::uintptr_t, mem_t>;


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
			(std::is_standard_layout_v<data_t> && std::is_trivial_v<data_t>);


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
#elif defined(F4SEAPI)
#elif defined(PLUGIN_MODE)
#	define TRAM_ALLOC(SIZE)
#endif


		inline std::string_view GetProcessName(HMODULE a_handle = 0) noexcept
		{
			static std::string fileName(MAX_PATH + 1, ' ');
			auto res = ::GetModuleBaseNameA(GetCurrentProcess(), a_handle, fileName.data(), MAX_PATH + 1);
			if (res == 0) {
				fileName = "[ProcessHost]";
				res = 13;
			}

			return { fileName.c_str(), res };
		}


		inline std::string_view GetProcessPath(HMODULE a_handle = 0) noexcept
		{
			static std::string fileName(MAX_PATH + 1, ' ');
			auto res = ::GetModuleFileNameA(a_handle, fileName.data(), MAX_PATH + 1);
			if (res == 0) {
				fileName = "[ProcessHost]";
				res = 13;
			}

			return { fileName.c_str(), res };
		}


		inline void WriteData(const dku_h_addr_t auto& a_dst, const void* a_data, const std::size_t a_size, bool a_requestAlloc = false) noexcept
		{
			if (a_requestAlloc) {
				void(TRAM_ALLOC(a_size));
			}

			DWORD oldProtect;

			auto success = ::VirtualProtect(AsPointer(a_dst), a_size, PAGE_EXECUTE_READWRITE, std::addressof(oldProtect));
			if (success != FALSE) {
				std::memcpy(AsPointer(a_dst), a_data, a_size);
				success = ::VirtualProtect(AsPointer(a_dst), a_size, oldProtect, std::addressof(oldProtect));
			}

			assert(success != FALSE);
		}

		// imm
		inline void WriteImm(const dku_h_addr_t auto& a_dst, const dku_h_pod_t auto& a_data, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, std::addressof(a_data), sizeof(a_data), a_requestAlloc);
		}

		// pair patch
		inline void WritePatch(const dku_h_addr_t auto& a_dst, const unpacked_data a_patch, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, a_patch.first, a_patch.second, a_requestAlloc);
		}

		// xbyak patch
		inline void WritePatch(const dku_h_addr_t auto& a_dst, const Xbyak::CodeGenerator* a_patch, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, a_patch->getCode(), a_patch->getSize(), a_requestAlloc);
		}

		// struct patch
		inline void WritePatch(const dku_h_addr_t auto& a_dst, const Hook::Patch* a_patch, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, a_patch->Data, a_patch->Size, a_requestAlloc);
		}

		// util func
		inline constexpr std::uintptr_t TblToAbs(const dku_h_addr_t auto& a_base, const std::uint16_t a_index, const std::size_t a_size = sizeof(Imm64)) noexcept
		{
			return AsAddress(a_base + a_index * a_size);
		}

		template <class To, class From>
		[[nodiscard]] To unrestricted_cast(From a_from) noexcept
		{
			if constexpr (std::is_same_v<
							  std::remove_cv_t<From>,
							  std::remove_cv_t<To>>) {
				return To{ a_from };

				// From != To
			} else if constexpr (std::is_reference_v<From>) {
				return unrestricted_cast<To>(std::addressof(a_from));

				// From: NOT reference
			} else if constexpr (std::is_reference_v<To>) {
				return *unrestricted_cast<
					std::add_pointer_t<
						std::remove_reference_t<To>>>(a_from);

				// To: NOT reference
			} else if constexpr (std::is_pointer_v<From> &&
								 std::is_pointer_v<To>) {
				return static_cast<To>(
					const_cast<void*>(
						static_cast<const volatile void*>(a_from)));
			} else if constexpr ((std::is_pointer_v<From> && std::is_integral_v<To>) ||
								 (std::is_integral_v<From> && std::is_pointer_v<To>)) {
				return std::bit_cast<To>(a_from);
			} else {
				union
				{
					std::remove_cv_t<std::remove_reference_t<From>> from;
					std::remove_cv_t<std::remove_reference_t<To>> to;
				};

				from = std::forward<From>(a_from);
				return to;
			}
		}

		template <typename T = void, typename U>
		[[nodiscard]] inline constexpr auto adjust_pointer(U* a_ptr, std::ptrdiff_t a_adjust) noexcept
		{
			auto addr = a_ptr ? std::bit_cast<std::uintptr_t>(a_ptr) + a_adjust : 0;
			if constexpr (std::is_const_v<U> && std::is_volatile_v<U>) {
				return std::bit_cast<std::add_cv_t<T>*>(addr);
			} else if constexpr (std::is_const_v<U>) {
				return std::bit_cast<std::add_const_t<T>*>(addr);
			} else if constexpr (std::is_volatile_v<U>) {
				return std::bit_cast<std::add_volatile_t<T>*>(addr);
			} else {
				return std::bit_cast<T*>(addr);
			}
		}

		template <typename T = std::uintptr_t*>
		[[nodiscard]] inline constexpr auto offset_pointer(const dku_h_addr_t auto& a_ptr, std::ptrdiff_t a_offset) noexcept
		{
			return *adjust_pointer<T>(a_ptr, a_offset);
		}

		template <typename T>
		inline constexpr void memzero(volatile T* a_ptr, std::size_t a_size = sizeof(T)) noexcept
		{
			const auto begin = std::bit_cast<volatile char*>(a_ptr);
			constexpr char val{ 0 };
			std::fill_n(begin, a_size, val);
		}

		inline Disp32 ReDisp(const std::uintptr_t a_src, const Disp32 a_srcOffset, const std::uintptr_t a_dst, const Disp32 a_dstOffset)
		{
			Disp32* disp = std::bit_cast<Disp32*>(a_src + a_srcOffset);
			Disp32* newDisp = std::bit_cast<Disp32*>(a_dst + a_dstOffset);
			*newDisp = a_src + *disp - a_dst;

			return *newDisp;
		}


		class Module
		{
		public:
			enum class Section : std::size_t
			{
				textx,
				idata,
				rdata,
				data,
				pdata,
				tls,
				textw,
				gfids,
				total
			};
			using SectionDescriptor = std::tuple<Section, std::uintptr_t, std::size_t>;

			constexpr Module() = delete;
			explicit Module(std::uintptr_t a_base)
			{
				if (!a_base) {
					ERROR("DKU_H: Failed to initializing module info with null module base");
				}

				_base = AsAddress(a_base);
				_dosHeader = std::bit_cast<::IMAGE_DOS_HEADER*>(a_base);
				_ntHeader = adjust_pointer<::IMAGE_NT_HEADERS64>(_dosHeader, _dosHeader->e_lfanew);
				_sectionHeader = IMAGE_FIRST_SECTION(_ntHeader);

				const auto total = std::min<std::size_t>(_ntHeader->FileHeader.NumberOfSections, std::to_underlying(Section::total));
				for (auto idx = 0; idx < total; ++idx) {
					const auto section = _sectionHeader[idx];
					auto& sectionNameTbl = dku::static_enum<Section>();
					for (Section name : sectionNameTbl.value_range(Section::textx, Section::gfids)) {
						const auto len = (std::min)(dku::print_enum(name).size(), std::extent_v<decltype(section.Name)>);
						if (std::memcmp(dku::print_enum(name).data(), section.Name + 1, len - 1) == 0) {
							_sections[idx] = std::make_tuple(name, _base + section.VirtualAddress, section.Misc.VirtualSize);
						}
					}
				}
			}
			explicit Module(std::string_view a_filePath)
			{
				const auto base = AsAddress(::GetModuleHandleA(a_filePath.data())) & ~3;
				if (!base) {
					ERROR("DKU_H: Failed to initializing module info with file {}", a_filePath);
				}

				*this = Module(base);
			}

			[[nodiscard]] constexpr auto base() const noexcept { return _base; }
			[[nodiscard]] constexpr auto* dosHeader() const noexcept { return _dosHeader; }
			[[nodiscard]] constexpr auto* ntHeader() const noexcept { return _ntHeader; }
			[[nodiscard]] constexpr auto* sectionHeader() const noexcept { return _sectionHeader; }
			[[nodiscard]] constexpr auto section(Section a_section) noexcept
			{
				auto& [sec, addr, size] = _sections[std::to_underlying(a_section)];
				return std::make_pair(addr, size);
			}

			[[nodiscard]] static Module& get(const dku_h_addr_t auto a_address) noexcept
			{
				static std::unordered_map<std::uintptr_t, Module> managed;

				const auto base = AsAddress(a_address) & ~3;
				if (!managed.contains(base)) {
					managed.try_emplace(base, base);
				}

				return managed.at(base);
			}

			[[nodiscard]] static Module& get(std::string_view a_filePath = {}) noexcept
			{
				const auto base = AsAddress(::GetModuleHandleA(a_filePath.empty() ? GetProcessPath().data() : a_filePath.data()));
				return get(base);
			}

		private:
			std::uintptr_t _base;
			::IMAGE_DOS_HEADER* _dosHeader;
			::IMAGE_NT_HEADERS64* _ntHeader;
			::IMAGE_SECTION_HEADER* _sectionHeader;
			std::array<SectionDescriptor, std::to_underlying(Section::total)> _sections;
		};

		[[nodiscard]] inline void* GetImportAddress(std::string_view a_moduleName, std::string_view a_libraryName, std::string_view a_importName) noexcept
		{
			if (a_libraryName.empty() || a_importName.empty()) {
				ERROR("DKU_H: IAT hook must have valid library name & method name\nConsider using GetProcessName([Opt]HMODULE)");
			}

			auto& module = Module::get(a_moduleName);
			const auto* dosHeader = module.dosHeader();
			const auto* importTbl = adjust_pointer<const ::IMAGE_IMPORT_DESCRIPTOR>(dosHeader, module.ntHeader()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

			for (void(0); importTbl->Characteristics; ++importTbl) {
				const char* libraryName = adjust_pointer<const char>(dosHeader, importTbl->Name);
				if (!string::iequals(a_libraryName, libraryName)) {
					continue;
				}

				if (!importTbl->FirstThunk || !importTbl->OriginalFirstThunk) {
					break;
				}

				const auto* iat = adjust_pointer<const ::IMAGE_THUNK_DATA>(dosHeader, importTbl->FirstThunk);
				const auto* thunk = adjust_pointer<const ::IMAGE_THUNK_DATA>(dosHeader, importTbl->OriginalFirstThunk);

				for (void(0); iat->u1.Function; ++thunk, ++iat) {
					if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
						continue;
					}

					const auto* info = adjust_pointer<const ::IMAGE_IMPORT_BY_NAME>(dosHeader, thunk->u1.AddressOfData);

					if (!string::iequals(a_importName, std::bit_cast<const char*>(std::addressof(info->Name[0])))) {
						continue;
					}

					return AsPointer(iat);
				}
			}

			return nullptr;
		}

		[[nodiscard]] inline std::uintptr_t GetFuncPrologAddr(std::uintptr_t a_addr)
		{
			static std::unordered_map<std::uintptr_t, std::uintptr_t> func;
			constexpr auto maxWalkableOpSeq = static_cast<size_t>(1) << 12;

			if (!func.contains(a_addr)) {
				auto* prev = std::bit_cast<OpCode*>(a_addr);
				std::size_t walked = 0;
				while (walked++ < maxWalkableOpSeq) {
					if (*--prev == 0xCC && *--prev == 0xCC) {
						func[a_addr] = AsAddress(prev + 0x2);
						break;
					}
				}
			}

			return func[a_addr];
		}
	}  // namespace Hook
}  // namespace DKUtil
