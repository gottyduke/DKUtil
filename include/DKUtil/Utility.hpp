#pragma once


#include <algorithm>
#include <concepts>
#include <string>


namespace DKUtil
{
	namespace Utility
	{
		constexpr std::uint32_t FNV_1A_32(const char* const a_str, const std::uint32_t a_value = 2166136261u) noexcept
		{
			return (a_str[0] == '\0')
					   ? a_value
					   : FNV_1A_32(&a_str[1], (a_value ^ static_cast<std::uint32_t>(a_str[0])) * 16777619u);
		}


		constexpr std::uint64_t FNV_1A_64(const char* const a_str,
										  const std::uint64_t a_value = 14695981039346656037u) noexcept
		{
			return (a_str[0] == '\0')
					   ? a_value
					   : FNV_1A_64(&a_str[1], (a_value ^ static_cast<std::uint64_t>(a_str[0])) * 1099511628211u);
		}
	}
}
