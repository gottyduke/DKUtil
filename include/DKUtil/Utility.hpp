#pragma once


/*
 * 1.0.0
 * Adaptation of file structural changes;
 * 
 */


#define DKU_U_VERSION_MAJOR 1
#define DKU_U_VERSION_MINOR 0
#define DKU_U_VERSION_REVISION 0


#include "Impl/pch.hpp"
#include "Logger.hpp"


/* Bunch of stuff taken from CommonLibSSE-Util */


namespace std::ranges::views
{
	inline constexpr auto drop_last = [](std::size_t count) { return std::views::reverse | std::views::drop(count) | std::views::reverse; };
	inline constexpr auto take_last = [](std::size_t count) { return std::views::reverse | std::views::take(count) | std::views::reverse; };
}  // namepsace std::views


namespace DKUtil
{
	constexpr auto DKU_U_VERSION = DKU_U_VERSION_MAJOR * 10000 + DKU_U_VERSION_MINOR * 100 + DKU_U_VERSION_REVISION;
}  // namespace DKUtil


#include "Impl/Utility/enumeration.hpp"
#include "Impl/Utility/numbers.hpp"
#include "Impl/Utility/shared.hpp"
#include "Impl/Utility/string.hpp"
#include "Impl/Utility/templates.hpp"


namespace DKUtil
{
	template <class Derived>
	using Singleton = model::Singleton<Derived>;

	template <class Enum, class Underlying = std::underlying_type_t<Enum>>
	using enumeration = model::enumeration<Enum, Underlying>;

	template <class Enum,
		class Underlying = std::conditional_t<
			std::is_signed_v<std::underlying_type_t<Enum>>,
			std::uint32_t, std::underlying_type_t<Enum>>>
	inline constexpr auto& static_enum() noexcept
	{
		static model::enumeration<Enum, Underlying> instance;
		return instance;
	}

	template <class Enum>
	inline constexpr std::string print_enum(Enum a_enum) noexcept
	{
		auto& tbl = static_enum<Enum>();
		return tbl.to_string(a_enum);
	}
}  // namespace DKUtil::Alias