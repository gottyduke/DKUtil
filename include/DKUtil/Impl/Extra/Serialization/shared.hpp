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
			TRACE("{}", ManagedSerializables.size());
		}

		static void SaveAll(SKSE::SerializationInterface* a_intfc) noexcept
		{
			for (auto* serializable : ManagedSerializables) {
				serializable->do_save(a_intfc);
			}
		}

		static void LoadAll(SKSE::SerializationInterface* a_intfc) noexcept
		{
			for (auto* serializable : ManagedSerializables) {
				serializable->do_load(a_intfc);
			}
		}

		static void RevertAll(SKSE::SerializationInterface* a_intfc) noexcept
		{
			for (auto* serializable : ManagedSerializables) {
				serializable->do_revert(a_intfc);
			}
		}

	protected:
		virtual void do_save(SKSE::SerializationInterface* a_intfc) = 0;
		virtual void do_load(SKSE::SerializationInterface* a_intfc) = 0;
		virtual void do_revert(SKSE::SerializationInterface* a_intfc) = 0;

		inline static std::vector<ISerializable*> ManagedSerializables = {};
	};
} // namespace DKUtil::Serialization