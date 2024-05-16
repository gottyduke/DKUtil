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
			FATAL("DKU_H: Runtime offset failed to relocate for unknown runtime!");
			std::unreachable();
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
			FATAL("DKU_H: Runtime patch failed to relocate for unknown runtime!");
			return (const Xbyak::CodeGenerator*)nullptr;
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
			FATAL("DKU_H: Runtime patch failed to relocate for unknown runtime!");
			return {};
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

#if defined(SFSEAPI)

namespace database
{
	enum : std::uint32_t
	{
		kDatabaseVersion = 2
	};

	struct mapping_t
	{
		std::uint64_t id;
		std::uint64_t offset;
	};

	enum class Platform
	{
		kUnknown = -1,
		kSteam,
		kMsStore,
	};

	class memory_map
	{
	public:
		memory_map() noexcept = default;

		memory_map(const memory_map&) = delete;

		memory_map(memory_map&& a_rhs) noexcept :
			_mapping(a_rhs._mapping),
			_view(a_rhs._view)
		{
			a_rhs._mapping = nullptr;
			a_rhs._view = nullptr;
		}

		~memory_map() { close(); }

		memory_map& operator=(const memory_map&) = delete;

		memory_map& operator=(memory_map&& a_rhs) noexcept
		{
			if (this != std::addressof(a_rhs)) {
				_mapping = a_rhs._mapping;
				a_rhs._mapping = nullptr;

				_view = a_rhs._view;
				a_rhs._view = nullptr;
			}
			return *this;
		}

		[[nodiscard]] void* data() noexcept { return _view; }

		bool open(std::string a_name, std::size_t a_size)
		{
			close();

			::ULARGE_INTEGER bytes{};
			bytes.QuadPart = a_size;

			_mapping = ::OpenFileMappingA(
				FILE_MAP_READ | FILE_MAP_WRITE,
				false,
				a_name.data());

			if (!_mapping) {
				close();
				return false;
			}

			_view = ::MapViewOfFile(
				_mapping,
				FILE_MAP_READ | FILE_MAP_WRITE,
				0,
				0,
				bytes.QuadPart);

			if (!_view) {
				close();
				return false;
			}

			return true;
		}

		bool create(std::string a_name, std::size_t a_size)
		{
			close();

			::ULARGE_INTEGER bytes{};
			bytes.QuadPart = a_size;

			_mapping = ::OpenFileMappingA(
				FILE_MAP_READ | FILE_MAP_WRITE,
				false,
				a_name.data());

			if (!_mapping) {
				_mapping = ::CreateFileMappingA(
					INVALID_HANDLE_VALUE,
					nullptr,
					PAGE_READWRITE,
					bytes.HighPart,
					bytes.LowPart,
					a_name.data());

				if (!_mapping) {
					return false;
				}
			}

			_view = ::MapViewOfFile(
				_mapping,
				FILE_MAP_READ | FILE_MAP_WRITE,
				0,
				0,
				bytes.QuadPart);

			if (!_view) {
				return false;
			}

			return true;
		}

		void close()
		{
			if (_view) {
				(void)::UnmapViewOfFile(AsPointer(_view));
				_view = nullptr;
			}

			if (_mapping) {
				(void)::CloseHandle(_mapping);
				_mapping = nullptr;
			}
		}

	private:
		void* _mapping{ nullptr };
		void* _view{ nullptr };
	};

	struct AddressLibStream
	{
		AddressLibStream(std::string_view a_filename, std::ios_base::openmode a_mode) :
			stream(a_filename.data(), a_mode)
		{
			dku_assert(stream.is_open(),
				"DKU_H: Failed to open address library file");

			stream.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
		}

		void ignore(std::streamsize a_count)
		{
			stream.ignore(a_count);
		}

		template <class T>
		void readin(T& a_val)
		{
			stream.read(std::bit_cast<char*>(std::addressof(a_val)), sizeof(T));
		}

		template <class T>
			requires(std::is_arithmetic_v<T>)
		T readout()
		{
			T val{};
			readin(val);
			return val;
		}

		std::ifstream stream;
	};

	inline static memory_map           Mmap{};
	inline static std::span<mapping_t> Id2offset{};
	inline static Platform             CurrentPlatform = Platform::kUnknown;
	inline constexpr auto              LookUpDir = "Data\\SFSE\\Plugins"sv;

	inline std::string AddresslibFilename()
	{
		const auto version = Module::get().version_string();
		// address lib files are in { runtimeDirectory + "Data\\SFSE\\Plugins" }
		auto file = std::filesystem::path(GetModulePath()).parent_path();

		file /= fmt::format("{}\\versionlib-{}", LookUpDir, version);

		CurrentPlatform = ::GetModuleHandleA("steam_api64") ? Platform::kSteam : Platform::kMsStore;

		// steam version omits the suffix
		if (CurrentPlatform != Platform::kSteam) {
			file += fmt::format("-{}", std::to_underlying(CurrentPlatform));
		}
		file += ".bin";

		dku_assert(std::filesystem::exists(file),
			"DKU_H: Failed to find address library file in directory.\n"
			"Expected: {}",
			file.string());

		return file.string();
	}

	inline bool LoadAddressLibrary()
	{
		char name[20]{};
		auto filename = AddresslibFilename();

		try {
			AddressLibStream in(filename, std::ios::in | std::ios::binary);
			std::uint32_t    format{};
			in.readin(format);

			dku_assert(format == kDatabaseVersion,
				"DKU_H: Unsupported address library format: {}\n"
				"Compiled IDDatabase version: {}\n"
				"This means this script extender plugin is incompatible with the address "
				"library available for this version of the game, and thus does not support it."sv,
				format, std::to_underlying(kDatabaseVersion));

			std::uint32_t version[4]{};
			std::uint32_t nameLen{};
			in.readin(version);
			in.readin(nameLen);

			for (std::uint32_t i = 0; i < nameLen; ++i) {
				in.readin(name[i]);
			}
			name[nameLen] = '\0';

			std::uint32_t pointerSize{};
			std::uint32_t addressCount{};
			in.readin(pointerSize);
			in.readin(addressCount);

			dku_assert(std::ranges::equal(version, Module::get().version()),
				"Address library version mismatch.\n"
				"Read-in : {}-{}-{}-{}\n"
				"Expected: {}"sv,
				version[0], version[1], version[2], version[3], Module::get().version_string());

			auto mapname = fmt::format(
				// kDatabaseVersion, runtimeVersion, runtimePlatform
				"CommonLibSF-Offsets-v{}-{}-{}",
				std::to_underlying(kDatabaseVersion),
				Module::get().version_string(),
				std::to_underlying(CurrentPlatform));

			const auto byteSize = static_cast<std::size_t>(addressCount) * sizeof(mapping_t);
			if (Mmap.open(mapname, byteSize)) {
				Id2offset = { static_cast<mapping_t*>(Mmap.data()), addressCount };
			} else if (Mmap.create(mapname, byteSize)) {
				Id2offset = { static_cast<mapping_t*>(Mmap.data()), addressCount };

				std::uint8_t  type = 0;
				std::uint64_t id = 0;
				std::uint64_t offset = 0;
				std::uint64_t prevID = 0;
				std::uint64_t prevOffset = 0;
				for (auto& mapping : Id2offset) {
					in.readin(type);
					const auto lo = static_cast<std::uint8_t>(type & 0xF);
					const auto hi = static_cast<std::uint8_t>(type >> 4);

					switch (lo) {
					case 0:
						in.readin(id);
						break;
					case 1:
						id = prevID + 1;
						break;
					case 2:
						id = prevID + in.readout<std::uint8_t>();
						break;
					case 3:
						id = prevID - in.readout<std::uint8_t>();
						break;
					case 4:
						id = prevID + in.readout<std::uint16_t>();
						break;
					case 5:
						id = prevID - in.readout<std::uint16_t>();
						break;
					case 6:
						id = in.readout<std::uint16_t>();
						break;
					case 7:
						id = in.readout<std::uint32_t>();
						break;
					default:
						FATAL("unhandled type"sv);
						break;
					}

					const std::uint64_t tmp = (hi & 8) != 0 ? (prevOffset / pointerSize) : prevOffset;

					switch (hi & 7) {
					case 0:
						in.readin(offset);
						break;
					case 1:
						offset = tmp + 1;
						break;
					case 2:
						offset = tmp + in.readout<std::uint8_t>();
						break;
					case 3:
						offset = tmp - in.readout<std::uint8_t>();
						break;
					case 4:
						offset = tmp + in.readout<std::uint16_t>();
						break;
					case 5:
						offset = tmp - in.readout<std::uint16_t>();
						break;
					case 6:
						offset = in.readout<std::uint16_t>();
						break;
					case 7:
						offset = in.readout<std::uint32_t>();
						break;
					default:
						FATAL("unhandled type"sv);
						break;
					}

					if ((hi & 8) != 0) {
						offset *= pointerSize;
					}

					mapping = { id, offset };

					prevOffset = offset;
					prevID = id;
				}

				std::ranges::sort(
					Id2offset,
					[](auto&& a_lhs, auto&& a_rhs) {
						return a_lhs.id < a_rhs.id;
					});
			} else {
				FATAL("failed to create shared mapping"sv);
				return false;
			}
		} catch (const std::system_error&) {
			FATAL(
				"Failed to locate an appropriate address library with the path: {}\n"
				"This means you are either missing the address library for this "
				"specific version of the game or running on unsupported platform.\n"
				"Please continue to the mod page for address library to download "
				"an appropriate version or platform. \nIf one is not available, "
				"then it is likely that address library has not yet added support "
				"for this version of the game or this platform.\n"
				"Current version: {}\n"sv,
				filename,
				Module::get().version_string());
			return false;
		}
		return true;
	}
}  // namespace database

inline std::uintptr_t IDToRva(std::uint64_t a_id) noexcept
{
	if (database::Id2offset.empty()) {
		database::LoadAddressLibrary();
	}

	database::mapping_t elem{ a_id, 0 };
	const auto          it = std::ranges::lower_bound(
        database::Id2offset,
        elem,
        [](auto&& a_lhs, auto&& a_rhs) {
            return a_lhs.id < a_rhs.id;
        });

	dku_assert(it != database::Id2offset.end(),
		"DKU_H: Failed to find the id within the address library: {}\n"
		"Compiled IDDatabase version: {}\n"
		"This means this script extender plugin is incompatible with the address "
		"library for this version of the game, and thus does not support it."sv,
		a_id, std::to_underlying(database::kDatabaseVersion));

	return static_cast<std::uintptr_t>(it->offset);
}

inline std::uintptr_t IDToAbs(std::uint64_t a_id, [[maybe_unused]] std::ptrdiff_t a_offset = 0) noexcept
{
	return Module::get().base() + IDToRva(a_id) + a_offset;
}
#endif
