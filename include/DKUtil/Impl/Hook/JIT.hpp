#pragma once


#include "assembly.hpp"


#define FUNC_INFO(FUNC)                           \
	DKUtil::Hook::FuncInfo                        \
	{                                             \
		reinterpret_cast<std::uintptr_t>(FUNC),   \
			DKUtil::Hook::GetFuncArgsCount(FUNC), \
			#FUNC                                 \
	}


namespace DKUtil::Hook
{
	template <typename Func, typename... Args>
	inline consteval std::size_t GetFuncArgsCount(Func (*)(Args...))
	{
		return decltype(std::integral_constant<unsigned, sizeof...(Args)>{})::value;
	}

	template <typename Func, class Class, typename... Args>
	inline consteval std::size_t GetMemFuncArgsCount(Func (Class::*)(Args...))
	{
		return decltype(std::integral_constant<unsigned, sizeof...(Args)>{})::value;
	}


	class FuncInfo
	{
	public:
		explicit constexpr FuncInfo(
			std::uintptr_t a_addr,
			std::uint8_t a_argsCount,
			std::string_view a_name) :
			_address(a_addr),
			_argsCount(a_argsCount),
			_name(a_name)
		{}

		[[nodiscard]] constexpr auto address() const noexcept { return _address; }
		[[nodiscard]] constexpr auto name() const noexcept { return _name; }
		[[nodiscard]] constexpr auto args_count() const noexcept { return _argsCount; }

	private:
		const std::uintptr_t _address;
		const std::uint8_t _argsCount;
		const std::string_view _name;
	};
}  // namespace DKUtil::Hook