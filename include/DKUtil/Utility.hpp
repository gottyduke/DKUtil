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


		constexpr std::uint64_t FNV_1A_64(const char* const a_str, const std::uint64_t a_value = 14695981039346656037u) noexcept
		{
			return (a_str[0] == '\0')
					   ? a_value
					   : FNV_1A_64(&a_str[1], (a_value ^ static_cast<std::uint64_t>(a_str[0])) * 1099511628211u);
		}


		template <typename Func, typename... Args>
		consteval std::size_t GetFuncArgsCount(Func(*)(Args...))
		{
			return decltype(std::integral_constant<unsigned, sizeof ...(Args)>{})::value;
		}


		template <typename Func, class Class, typename... Args>
		consteval std::size_t GetMemFuncArgsCount(Func(Class::*)(Args...))
		{
			return decltype(std::integral_constant<unsigned, sizeof ...(Args)>{})::value;
		}


		template<typename rvalue_t> 
		rvalue_t& lvalue_cast(rvalue_t&& a_rvalue) {
			return const_cast<rvalue_t&>(a_rvalue);
		}


		template <class derived_t>
		class Singleton
		{
		public:
			static derived_t* GetSingleton()
			{
				static derived_t singleton;
				return std::addressof(singleton);
			}

			Singleton(const Singleton&) = delete;
			Singleton(Singleton&&) = delete;
			Singleton& operator=(const Singleton&) = delete;
			Singleton& operator=(Singleton&&) = delete;

		protected:
			Singleton() = default;
			virtual ~Singleton() = default;
		};


		std::wstring String2WString(const std::string& a_str)
		{
			int resultSize = MultiByteToWideChar(CP_UTF8, 0, &a_str[0], static_cast<int>(a_str.size()), nullptr, 0);
			std::wstring wStr(resultSize, 0);
			MultiByteToWideChar(CP_UTF8, 0, &a_str[0], static_cast<int>(a_str.size()), &wStr[0], resultSize);
			return std::move(wStr);
		}
	}
}
