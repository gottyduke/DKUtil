#pragma once
#pragma comment(lib, "Version.lib")

#include "DKUtil/Impl/pch.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"

#include <xbyak/xbyak.h>
#define AsAddress(PTR) std::bit_cast<std::uintptr_t>(PTR)
#define AsPointer(ADDR) std::bit_cast<void*>(ADDR)
#define AsRawAddr(ADDR) dku::Hook::GetRawAddress(AsAddress(ADDR))
#define AsMemCpy(DST, SRC) *std::bit_cast<std::remove_cvref_t<decltype(SRC)>*>(DST) = SRC

#define ASM_MINIMUM_SKIP 2
#define CAVE_MINIMUM_BYTES 0x5
#ifndef CAVE_BUF_SIZE
#	define CAVE_BUF_SIZE 1 << 7
#endif

#define ASM_STACK_ALLOC_SIZE 0x20

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
		using namespace Alias;

		using unpacked_data = std::pair<const void*, std::size_t>;
		using offset_pair = std::pair<std::ptrdiff_t, std::ptrdiff_t>;

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
			constexpr Patch(const void* a_data = nullptr, std::size_t a_size = 0, bool a_managed = false) noexcept :
				Data(a_data), Size(a_size), Managed(a_managed)
			{}

			constexpr ~Patch() noexcept
			{
				Free();
			}

			// owning copy
			constexpr Patch(Patch& a_rhs) noexcept :
				Data(a_rhs.Data), Size(a_rhs.Size), Managed(a_rhs.Managed)
			{
				if (Managed) {
					a_rhs.Managed = false;
				}
			}

			// owning move
			constexpr Patch(Patch&& a_rhs) noexcept :
				Data(a_rhs.Data), Size(a_rhs.Size), Managed(a_rhs.Managed)
			{
				a_rhs.Managed = false;
			}

			constexpr operator std::span<OpCode>() const noexcept
			{
				return std::span<OpCode>{ std::bit_cast<OpCode*>(Data), Size };
			}

			constexpr operator unpacked_data() const noexcept
			{
				return std::make_pair(Data, Size);
			}

			// append a patch in new memory, current data is reallocated
			Patch& Append(Patch a_rhs)
			{
				std::size_t total = Size + a_rhs.Size;
				if (!total) {
					return *this;
				}

				auto* buf = new OpCode[total]();

				if (Data && Size) {
					std::memcpy(buf, Data, Size);
				}

				if (a_rhs.Data && a_rhs.Size) {
					std::memcpy(buf + Size, a_rhs.Data, a_rhs.Size);
				}

				Free();

				Data = buf;
				Size = total;
				Managed = true;

				return *this;
			}

			Patch& Append(Xbyak::CodeGenerator& a_rhs) noexcept
			{
				return Append({ a_rhs.getCode(), a_rhs.getSize(), false });
			}

			Patch& Append(unpacked_data a_rhs) noexcept
			{
				return Append({ a_rhs.first, a_rhs.second, false });
			}

			constexpr void Free() noexcept
			{
				if (Managed && Data) {
					delete[] std::bit_cast<OpCode*>(Data);
					Data = nullptr;
					Size = 0;
					Managed = false;
				}
			}

			const void* Data{ nullptr };
			std::size_t Size{ 0 };
			bool        Managed{ false };
		};

		template <typename T = void, typename U>
		[[nodiscard]] inline constexpr auto adjust_pointer(U* a_ptr, std::ptrdiff_t a_adjust) noexcept
		{
			auto addr = a_ptr ? AsAddress(a_ptr) + a_adjust : 0;
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
		[[nodiscard]] inline constexpr auto offset_pointer(const model::concepts::dku_memory auto a_ptr, std::ptrdiff_t a_offset) noexcept
		{
			return *adjust_pointer<T>(a_ptr, a_offset);
		}

		template <typename T>
		inline constexpr void memzero(volatile T* a_ptr, std::size_t a_size = sizeof(T)) noexcept
		{
			const auto*    begin = std::bit_cast<volatile char*>(a_ptr);
			constexpr char val{ 0 };
			std::fill_n(begin, a_size, val);
		}

		/**
		 * \brief Get the calculated address of the rip displacement in an assembly instruction
		 * \brief Example : `FF 25 21 43 65 87` -> jmp qword ptr [rip + 0x87654321]
		 * \brief Calculates and returns the actual address it branches to
		 * \param T : Data type to read in at the calculated address
		 * \param a_src : Address of the assembly instruction. This must be the beginning of the full instruction
		 * \param a_opOffset : Optionally specify the offset where the displacement is, e.g. 1 for `E8 12 34 56 78`, 0 for auto
		 * @return Calculated address as T, std::uintptr_t by default
		 */
		template <typename T = std::uintptr_t>
		inline T GetDisp(const model::concepts::dku_memory auto a_src, const std::uint8_t a_opOffset = 0) noexcept
		{
			// assumes assembly is safe to read
			auto* opSeq = std::bit_cast<OpCode*>(a_src);
			Imm64 dst = 0;

			if (a_opOffset) {
				auto disp = *adjust_pointer<Disp32>(opSeq, a_opOffset - sizeof(Disp32)) + a_opOffset;
				dst = AsAddress(a_src) + disp;
			} else {
				// determine op
				switch (opSeq[0]) {
				// cd
				case 0xE8:  // call disp32
				case 0xE9:  // jmp disp32
					{
						auto disp = *adjust_pointer<Disp32>(opSeq, 5 - sizeof(Disp32)) + 5;
						dst = AsAddress(a_src) + disp;
					}
					break;
				// cb
				case 0xEB:  // jmp disp8
					{
						auto disp = *adjust_pointer<Disp8>(opSeq, 2 - sizeof(Disp8)) + 2;
						dst = AsAddress(a_src) + disp;
					}
					break;
				// /2 | /4
				case 0xFF:  // call/jmp modRM disp32
					{
						// FF /3 | /5 is not supported
						dku_assert(opSeq[1] != 0x1D && opSeq[1] != 0x2D,
							"DKU_H: GetDisp does not support reading FAR proc memory in different segment");

						auto disp = *adjust_pointer<Disp32>(opSeq, 6 - sizeof(Disp32)) + 6;
						dst = AsAddress(a_src) + disp;
						dst = *std::bit_cast<Imm64*>(dst);
					}
					break;
				// mov/lea /r
				case 0x88:
				case 0x89:
				case 0x8A:
				case 0x8B:
				case 0x8C:
				case 0x8D:
					{
						auto disp = *adjust_pointer<Disp32>(opSeq, 6 - sizeof(Disp32)) + 6;
						dst = AsAddress(a_src) + disp;
					}
					break;
				// REX.W
				case 0x48:
				case 0x66:
					{
						// advance to the next op
						dst = GetDisp(std::addressof(opSeq[1]));
					}
					break;
				default:
					break;
				}
			}

			dku_assert(dst, "DKU_H: GetDisp reads invalid relocation instruction\nsource : {:X}\nread-in : 0x{:2X}", AsAddress(a_src), opSeq[0]);

			return std::bit_cast<T>(dst);
		}

		inline Disp32 ReDisp(const model::concepts::dku_memory auto a_src, const model::concepts::dku_memory auto a_dst, const Disp32 a_dstOffset)
		{
			auto* newDisp = adjust_pointer<Disp32>(std::bit_cast<OpCode*>(a_dst), a_dstOffset);
			*newDisp = GetDisp(a_src) - AsAddress(a_dst);

			return *newDisp;
		}

		constexpr void assert_trampoline_range(std::ptrdiff_t a_disp)
		{
			constexpr auto min = std::numeric_limits<std::int32_t>::min();
			constexpr auto max = std::numeric_limits<std::int32_t>::max();

			dku_assert((min <= a_disp && a_disp <= max), "DKU_H: displacement is out of range for trampoline relocation!");
		}

		[[nodiscard]] inline std::vector<std::uint32_t> GetFileVersion(std::string_view a_filename)
		{
			std::vector<std::uint32_t> version(4);
			std::uint32_t              dummy{ 0 };
			std::vector<char>          buf(::GetFileVersionInfoSizeA(a_filename.data(), std::bit_cast<LPDWORD>(std::addressof(dummy))));
			if (buf.empty()) {
				return version;
			}

			if (!::GetFileVersionInfoA(a_filename.data(), 0, static_cast<std::uint32_t>(buf.size()), buf.data())) {
				return version;
			}

			void*         verBuf{ nullptr };
			std::uint32_t verLen{ 0 };
			if (!::VerQueryValueA(buf.data(), "\\StringFileInfo\\040904B0\\ProductVersion", std::addressof(verBuf), std::addressof(verLen))) {
				return version;
			}

			std::istringstream ss(std::string(static_cast<const char*>(verBuf), verLen));
			std::string        token;
			for (std::size_t i = 0; i < 4 && std::getline(ss, token, '.'); ++i) {
				version[i] = static_cast<std::uint32_t>(std::stoi(token));
			}

			return version;
		}

		inline std::string GetModuleName(HMODULE a_handle = 0) noexcept
		{
			std::string fileName(MAX_PATH + 1, '\0');
			fileName.resize(::GetModuleBaseNameA(GetCurrentProcess(), a_handle, fileName.data(), MAX_PATH));

			return fileName;
		}

		inline std::string GetModulePath(HMODULE a_handle = 0) noexcept
		{
			std::string filePath(MAX_PATH + 1, '\0');
			filePath.resize(::GetModuleFileNameA(a_handle, filePath.data(), MAX_PATH));

			return filePath;
		}

		inline std::string GetProcessName(DWORD a_process) noexcept
		{
			HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, a_process);
			if (!hProcess) {
				return {};
			}

			HMODULE hMod;
			DWORD   cbNeeded;
			if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded) == 0) {
				::CloseHandle(hProcess);
				return {};
			}

			std::string modName(MAX_PATH + 1, '\0');
			modName.resize(::GetModuleBaseNameA(hProcess, hMod, modName.data(), MAX_PATH));
			if (modName.empty()) {
				::CloseHandle(hProcess);
			}

			return modName;
		}

		inline std::string GetProcessPath(DWORD a_process) noexcept
		{
			HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, a_process);
			if (!hProcess) {
				return {};
			}

			HMODULE hMod;
			DWORD   cbNeeded;
			if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded) == 0) {
				::CloseHandle(hProcess);
				return {};
			}

			std::string modPath(MAX_PATH + 1, '\0');
			modPath.resize(::GetModuleFileNameExA(hProcess, hMod, modPath.data(), MAX_PATH));
			if (modPath.empty()) {
				::CloseHandle(hProcess);
			}

			return modPath;
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
				dku_assert(a_base, "DKU_H: Failed to initializing module info with null module base");

				_base = AsAddress(a_base);
				_dosHeader = std::bit_cast<::IMAGE_DOS_HEADER*>(a_base);
				_ntHeader = adjust_pointer<::IMAGE_NT_HEADERS64>(_dosHeader, _dosHeader->e_lfanew);
				_sectionHeader = IMAGE_FIRST_SECTION(_ntHeader);

				const auto total = std::min<std::size_t>(_ntHeader->FileHeader.NumberOfSections, std::to_underlying(Section::total));
				for (auto idx = 0; idx < total; ++idx) {
					const auto section = _sectionHeader[idx];
					auto&      sectionNameTbl = dku::static_enum<Section>();
					for (Section name : sectionNameTbl.value_range(Section::textx, Section::gfids)) {
						const auto len = (std::min)(dku::print_enum(name).size(), std::extent_v<decltype(section.Name)>);
						if (std::memcmp(dku::print_enum(name).data(), section.Name + 1, len - 1) == 0) {
							_sections[idx] = std::make_tuple(name, _base + section.VirtualAddress, section.Misc.VirtualSize);
						}
					}
				}

				_version = GetFileVersion(GetModulePath(std::bit_cast<HMODULE>(a_base)));
			}
			explicit Module(std::string_view a_filePath)
			{
				const auto base = AsAddress(::GetModuleHandleA(a_filePath.data())) & ~3;
				dku_assert(base, "DKU_H: Failed to initializing module info with file {}", a_filePath);

				*this = Module(base);
			}

			[[nodiscard]] constexpr auto  base() const noexcept { return _base; }
			[[nodiscard]] constexpr auto* dosHeader() const noexcept { return _dosHeader; }
			[[nodiscard]] constexpr auto* ntHeader() const noexcept { return _ntHeader; }
			[[nodiscard]] constexpr auto* sectionHeader() const noexcept { return _sectionHeader; }
			[[nodiscard]] constexpr auto  section(Section a_section) noexcept
			{
				auto& [sec, addr, size] = _sections[std::to_underlying(a_section)];
				return std::make_pair(addr, size);
			}

			[[nodiscard]] constexpr auto version() const noexcept { return _version; }
			[[nodiscard]] const auto     version_string(std::string_view a_delim = "-"sv)
			{
				return fmt::format("{}{}{}{}{}{}{}", _version[0], a_delim, _version[1], a_delim, _version[2], a_delim, _version[3]);
			}

			[[nodiscard]] constexpr auto version_number() const noexcept
			{
				return static_cast<std::uint32_t>(
					(_version[0] & 0x0FF) << 24u |
					(_version[1] & 0x0FF) << 16u |
					(_version[2] & 0xFFF) << 4u |
					(_version[3] & 0x00F) << 0u);
			}

			[[nodiscard]] static Module& get(const model::concepts::dku_memory auto a_address) noexcept
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
				const auto base = AsAddress(::GetModuleHandleA(a_filePath.empty() ? GetModulePath().data() : a_filePath.data()));
				return get(base);
			}

		private:
			std::uintptr_t                                                    _base;
			::IMAGE_DOS_HEADER*                                               _dosHeader;
			::IMAGE_NT_HEADERS64*                                             _ntHeader;
			::IMAGE_SECTION_HEADER*                                           _sectionHeader;
			std::array<SectionDescriptor, std::to_underlying(Section::total)> _sections;
			std::vector<std::uint32_t>                                        _version;
		};

		// COMPAT
#include "Shared_Compat.hpp"

		inline void WriteData(const model::concepts::dku_memory auto a_dst, const void* a_data, const std::size_t a_size, bool a_requestAlloc = false) noexcept
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

			dku_assert(success != FALSE,
				"DKU_H: Failed to write data, error code {}\n"
				"at   : {:X}\ndata : {:X}\nsize : {}\nalloc: {}",
				success, AsAddress(a_dst), AsAddress(a_data), a_size, a_requestAlloc);
		}

		// imm
		inline void WriteImm(const model::concepts::dku_memory auto a_dst, const model::concepts::dku_trivial auto a_data, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, std::addressof(a_data), sizeof(a_data), a_requestAlloc);
		}

		// pair patch
		inline void WritePatch(const model::concepts::dku_memory auto a_dst, const unpacked_data a_patch, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, a_patch.first, a_patch.second, a_requestAlloc);
		}

		// xbyak patch
		inline void WritePatch(const model::concepts::dku_memory auto a_dst, const Xbyak::CodeGenerator* a_patch, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, a_patch->getCode(), a_patch->getSize(), a_requestAlloc);
		}

		// struct patch
		inline void WritePatch(const model::concepts::dku_memory auto a_dst, const Hook::Patch* a_patch, bool a_requestAlloc = false) noexcept
		{
			return WriteData(a_dst, a_patch->Data, a_patch->Size, a_requestAlloc);
		}

		// util func
		inline constexpr std::uintptr_t TblToAbs(const model::concepts::dku_memory auto a_base, const std::uint16_t a_index, const std::size_t a_size = sizeof(Imm64)) noexcept
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
					std::remove_cv_t<std::remove_reference_t<To>>   to;
				};

				from = std::forward<From>(a_from);
				return to;
			}
		}

		[[nodiscard]] inline void* GetImportAddress(std::string_view a_moduleName, std::string_view a_libraryName, std::string_view a_importName) noexcept
		{
			dku_assert(!a_libraryName.empty() && !a_importName.empty(),
				"DKU_H: IAT hook must have valid library name & method name\nConsider using GetProcessName([Opt]HMODULE)");

			auto&       module = Module::get(a_moduleName);
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
			static std::unordered_map<std::uintptr_t, std::uintptr_t> cached;
			constexpr auto                                            abiBoundary = 0x8;
			constexpr auto                                            minPadding = 0x2;
			constexpr auto                                            maxWalkable = static_cast<size_t>(1) << 12;

			auto it = cached.find(a_addr);
			if (it != cached.end()) {
				return it->second;
			}

			std::uintptr_t start = a_addr - (a_addr % abiBoundary);
			std::uintptr_t end = start > maxWalkable ? start - maxWalkable : 0;

			for (std::uintptr_t current = start; current >= end; current -= abiBoundary) {
				auto epilogue = *adjust_pointer<std::uint16_t>(AsPointer(current), -minPadding);
				if (epilogue == 0xCCCC || epilogue == 0xCCC3) {
					cached[a_addr] = current;
					break;
				}
			}

			return cached.try_emplace(a_addr, a_addr).first->second;
		}

		/**
		 * \brief Get RVA of a full address
		 */
		[[nodiscard]] inline std::uintptr_t GetRva(std::uintptr_t a_address)
		{
			const auto base = Module::get().base();
			dku_assert(a_address > base,
				"DKU_H: Cannot calculate RVA from address {:X} "
				"because it's not in the same memory region",
				a_address);

			return a_address - base;
		}

		/**
		 * \brief Get aslr disabled address from a full relocated address
		 */
		[[nodiscard]] inline std::uintptr_t GetRawAddress(std::uintptr_t a_address)
		{
			return GetRva(a_address) + 0x140000000;
		}
	}  // namespace Hook
}  // namespace DKUtil
