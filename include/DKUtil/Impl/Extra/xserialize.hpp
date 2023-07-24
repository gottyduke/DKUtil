#pragma once


/*
* 
 * 1.0.0
 * Basic serialization and deserialization;
 * Support (should) all types that are commonly used;
 * Flatten user defined types for thorough serialization;
 * Concept constrained auto templates;
 * 
 */


#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"


#define DKU_X_MOCK

#include "Serialization/shared.hpp"
#include "Serialization/exception.hpp"
#include "Serialization/interface.hpp"
#include "Serialization/resolver.hpp"
#include "Serialization/variant.hpp"
#include "Serialization/mock.hpp"


namespace DKUtil::serialization
{
	namespace colliding
	{
		inline static constexpr hash_type KnownHash[] = {
			'ISCR',
			'COMP',
		};

		inline constexpr auto make_hash_key(const char* a_key)
		{
			key_type key = dku::string::join({ PROJECT_NAME, a_key }, "_");

			while (std::ranges::contains(KnownHash, dku::numbers::FNV_1A_32(key))) {
				key += "_";
			}

			return std::make_pair(key, dku::numbers::FNV_1A_32(key));
		}
	}  // namespace colliding


	template <typename T, dku::string::static_string HEADER, version_type VERSION = 1>
	struct Serializable : ISerializable
	{
		using type = std::remove_cvref_t<T>;
		using resolver_func_t = std::add_pointer_t<void(type&, ResolveOrder, SKSE::SerializationInterface*)>;

		constexpr Serializable() noexcept
		{
			auto&& [name, hash] = colliding::make_hash_key(HEADER.c);

			_header.name = name;
			_header.hash = hash;
			_header.version = VERSION;

			ISerializable::enable();
			INFO("{} {}", _header.name, ManagedSerializables.size());
		}

		constexpr Serializable(const type& a_data) noexcept :
			Serializable()
		{
			_data = a_data;
		}

		constexpr Serializable(type&& a_data) noexcept :
			Serializable()
		{
			_data = std::move(a_data);
		}

		constexpr Serializable& operator=(const type& a_data) noexcept
		{
			_data = a_data;
			return *this;
		}

		constexpr Serializable& operator=(type&& a_data) noexcept
		{
			_data = std::move(a_data);
			return *this;
		}

		~Serializable() noexcept = default;

		constexpr auto* operator->() noexcept { return std::addressof(_data); }
		constexpr auto& operator*() noexcept { return _data; }
		constexpr auto get() noexcept { return _data; }

		constexpr void add_resolver(resolver_func_t a_func) noexcept
		{
			_resolvers.emplace_back(a_func);
		}

	protected:
		void do_save(SKSE::SerializationInterface* a_intfc) override
		{
			INFO("saving {}...", _header.name);

			resolver::resolve_save(a_intfc, _header, _data);

			for (auto& resolver : _resolvers) {
				resolver(_data, ResolveOrder::kSave, a_intfc);
			}

			INFO("...done saving {}", _header.name);
			DKU_X_MOCK_REPORT();
		}

		void do_load(SKSE::SerializationInterface* a_intfc) override
		{
			INFO("loading {}...", _header.name);

			for (auto& resolver : _resolvers) {
				resolver(_data, ResolveOrder::kLoad, a_intfc);
			}

			INFO("...done loading {}", _header.name);
		}

		void do_revert(SKSE::SerializationInterface* a_intfc) override
		{
			INFO("reverting {}...", _header.name);


			for (auto& resolver : _resolvers) {
				resolver(_data, ResolveOrder::kRevert, a_intfc);
			}

			_data = type{};

			INFO("...done reverting {}", _header.name);
		}

	private:
		/// revert interface

		// clear data
		void revert([[maybe_unused]] SKSE::SerializationInterface* a_intfc, T a_data) noexcept
		{
			INFO(" revert");

			_data = type{};
		}


		type _data;
		Header _header;
		std::vector<resolver_func_t> _resolvers;
	};


	inline static void RegisterSerializable() noexcept
	{
		const auto* serialization = SKSE::GetSerializationInterface();
		serialization->SetSaveCallback(ISerializable::SaveAll);
		serialization->SetLoadCallback(ISerializable::LoadAll);
		serialization->SetRevertCallback(ISerializable::RevertAll);

		INFO("DKU_X: Registered serializables");
	}
} // namespace DKUtil::serialization
