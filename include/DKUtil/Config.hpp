#pragma once

/*
 * 1.2.0
 * Added Schema parser;
 * 
 * 1.1.6
 * SFSE integration;
 * 
 * 1.1.5
 * Removed DataManager layer between raw data and proxy;
 * 
 * 1.1.4
 * Adaptaion of file structural changes;
 * 
 * 1.1.3
 * out-of-bound adjusts;
 *
 * 1.1.2
 * constexpr specifiers;
 * 
 * 1.1.1
 * Added alias for config proxies;
 * Minor logging practices;
 * 
 * 1.1.0
 * NG-update;
 * Added clamp range;
 * Removed Proxy isLoaded check; 
 *
 * 1.0.2
 * F4SE integration;
 *
 * 1.0.1
 * Fixed toml empty array crash;
 * Removed explicit identifier on copy constructor;
 * 
 * 
 * 1.0.0
 * Proxy, Bind and Load APIs for ini, json and toml type config files;
 * 
 */

#define DKU_C_VERSION_MAJOR 1
#define DKU_C_VERSION_MINOR 2
#define DKU_C_VERSION_REVISION 0

#pragma warning(push)
#pragma warning(disable: 4244)

#include "Impl/pch.hpp"

#if !defined(CONFIG_ENTRY)

#	if defined(F4SEAPI)
#		define CONFIG_ENTRY "Data\\F4SE\\Plugins\\"
#	elif defined(SKSEAPI)
#		define CONFIG_ENTRY "Data\\SKSE\\Plugins\\"
#	elif defined(SFSEAPI)
#		define CONFIG_ENTRY "Data\\SFSE\\Plugins\\"
#	else
#		define CONFIG_ENTRY ""
#	endif

#endif

namespace DKUtil
{
	constexpr auto DKU_C_VERSION = DKU_C_VERSION_MAJOR * 10000 + DKU_C_VERSION_MINOR * 100 + DKU_C_VERSION_REVISION;
}  // namespace DKUtil

#include "Impl/Config/shared.hpp"

#include "Impl/Config/data.hpp"

#include "Impl/Config/ini.hpp"
#include "Impl/Config/json.hpp"
#include "Impl/Config/schema.hpp"
#include "Impl/Config/toml.hpp"

#include "Impl/Config/proxy.hpp"

namespace DKUtil::Config
{
	/* @brief Parse a string into user defined struct
	 * @brief e.g. CustomData d = ParseSchemaString<CustomData>(line, delim...)
	 * @brief This API is an alias of global schema parser for ease of use
	 * @param a_str : non-empty formatted schema string
	 * @param a_delimiters : one or multiple string delimiters used to make segments
	 * @returns user defined struct
	 */
	template <typename SchemaData>
		requires(model::concepts::dku_aggregate<SchemaData>)
	inline static SchemaData ParseSchemaString(std::string a_str, const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
	{
		return detail::Schema::ParseString<SchemaData>(a_str, std::forward<decltype(a_delimiters)>(a_delimiters)...);
	}

	/* @brief Parse a string to user defined series of segments
	 * @brief e.g. auto [a, b, c, d] = ParseSchemaString<int, bool, bool, std::string>(line, delim...)
	 * @brief This API is an alias of global schema parser for ease of use
	 * @param a_str : non-empty formatted schema string
	 * @param a_delimiters : one or multiple string delimiters used to make segments
	 * @returns std::tuple of user defined segments
	 */
	template <typename... SchemaSegment>
		requires(sizeof...(SchemaSegment) > 1)
	inline static std::tuple<SchemaSegment...> ParseSchemaString(std::string a_str, const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
	{
		return detail::Schema::ParseString<std::tuple<SchemaSegment...>>(a_str, std::forward<decltype(a_delimiters)>(a_delimiters)...);
	}
}  // namespace DKUtil::Config

namespace DKUtil::Alias
{
	using Boolean = DKUtil::Config::detail::AData<bool>;
	using Integer = DKUtil::Config::detail::AData<std::int64_t>;
	using Double = DKUtil::Config::detail::AData<double>;
	using String = DKUtil::Config::detail::AData<std::basic_string<char>>;

	using IniConfig = DKUtil::Config::Proxy<DKUtil::Config::FileType::kIni>;
	using JsonConfig = DKUtil::Config::Proxy<DKUtil::Config::FileType::kJson>;
	using TomlConfig = DKUtil::Config::Proxy<DKUtil::Config::FileType::kToml>;
	using SchemaConfig = DKUtil::Config::Proxy<DKUtil::Config::FileType::kSchema>;
	using DynamicConfig = DKUtil::Config::Proxy<DKUtil::Config::FileType::kDynamic>;
}  // namespace DKUtil::Alias

#pragma warning(pop)
