#pragma once


#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"


#define DKU_X_SERIALIZE_MAJOR 1
#define DKU_X_SERIALIZE_MINOR 0
#define DKU_X_SERIALIZE_REVISION 0


namespace DKUtil
{
	constexpr auto DKU_XS_VERSION = DKU_X_SERIALIZE_MAJOR * 10000 + DKU_X_SERIALIZE_MINOR * 100 + DKU_X_SERIALIZE_REVISION;
}  // namespace DKUtil


namespace DKUtil::serialization
{
	using size_type = std::uint32_t;
	using index_type = std::uint32_t;
	using key_type = std::string;
	using hash_type = std::uint32_t;
	using version_type = std::uint32_t;


	namespace colliding
	{
		inline static constexpr hash_type KnownHash[] = {
			'COMP',      // CompletionistNG
			'ISCR',      // IndividualSoutCooldownRemake
			'SAMS',      // achievement system for skyrim
			'SPIS',      // SplitItemStacks
			0xFD34899E,  // NPCsUsePotions & AlchemyExpansion
			0x68ED6325,  // DiseaseOverhaul
		};

		inline static index_type HashIndex = 0;

		inline constexpr auto make_hash_key(const char* a_key)
		{
			key_type key = dku::string::join({ PROJECT_NAME, a_key }, "_");

			while (std::ranges::contains(KnownHash, dku::numbers::FNV_1A_32(key) + HashIndex)) {
				key += "_";
				HashIndex++;
			}

			return std::make_pair(key, dku::numbers::FNV_1A_32(key) + HashIndex);
		}
	}  // namespace colliding


	enum class ResolveOrder : std::uint32_t
	{
		kSave = 0,
		kLoad,
		kRevert,
		kInternal,
	};

	struct ISerializable
	{
		struct Header
		{
			key_type name;
			hash_type hash;
			version_type version;
			std::string typeInfo;
		};

		virtual void enable() noexcept
		{
			disable();
			ManagedSerializables.emplace_back(this);
		}

		virtual void disable() noexcept
		{
			std::erase_if(ManagedSerializables, [this](auto* a_ptr) { return a_ptr == this; });
		}

		virtual ~ISerializable() noexcept
		{
			disable();
		}

		virtual void try_save() noexcept = 0;
		virtual void try_load(hash_type, version_type) noexcept = 0;
		virtual void try_revert() noexcept = 0;

		inline static std::vector<ISerializable*> ManagedSerializables = {};

		Header header;
	};
}  // namespace DKUtil::Serialization