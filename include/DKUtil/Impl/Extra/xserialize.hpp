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


//#define DKU_X_MOCK


#ifndef DKU_X_STRICT_SERIALIZATION
#	define DKU_X_STRICT_SERIALIZATION false
#endif


#include "Serialization/exception.hpp"
#include "Serialization/interface.hpp"
#include "Serialization/mock.hpp"
#include "Serialization/resolver.hpp"
#include "Serialization/shared.hpp"
#include "Serialization/variant.hpp"


namespace DKUtil::serialization
{
	template <typename T, dku::string::static_string HEADER, version_type VERSION = 1>
	struct Serializable : ISerializable
	{
		using type = std::remove_cvref_t<T>;
		using resolver_func_t = std::add_pointer_t<void(type&, ResolveOrder)>;

		constexpr Serializable() noexcept
		{
			auto&& [name, hash] = colliding::make_hash_key(HEADER.c);

			header.name = name;
			header.hash = hash;
			header.version = VERSION;
			header.typeInfo = typeid(type).name();

			ISerializable::enable();
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

		virtual void try_save() noexcept override
		{
			DEBUG("saving {}...", header.name);

			resolver::resolve_save(header, _data);

			for (auto& resolver : _resolvers) {
				resolver(_data, ResolveOrder::kSave);
			}

			DEBUG("...done saving {}", header.name);
		}

		virtual void try_load(hash_type a_hash, version_type a_version) noexcept override
		{
			if (a_hash != header.hash) {
				return;
			}

			if (a_version != header.version) {
				// TODO: handle version variant
				return;
			}

			DEBUG("loading {}...", header.name);

			_data = resolver::resolve_load(header, _data);

			for (auto& resolver : _resolvers) {
				resolver(_data, ResolveOrder::kLoad);
			}

			DEBUG("...done loading {}", header.name);
		}

		virtual void try_revert() noexcept override
		{
			DEBUG("reverting {}...", header.name);

			_data = type{};

			for (auto& resolver : _resolvers) {
				resolver(_data, ResolveOrder::kRevert);
			}

			DEBUG("...done reverting {}", header.name);
		}

	private:
		type _data;
		std::vector<resolver_func_t> _resolvers;
	};
}  // namespace DKUtil::serialization
