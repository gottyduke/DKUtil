#pragma once


#include "assembly.hpp"


#define FUNC_INFO(FUNC)                               \
	DKUtil::Hook::FuncInfo                            \
	{                                                 \
		reinterpret_cast<std::uintptr_t>(FUNC), #FUNC \
	}
#define RT_INFO(FUNC, NAME) \
	DKUtil::Hook::FuncInfo  \
	{                       \
		FUNC, NAME          \
	}


namespace DKUtil::Hook
{
	namespace JIT
	{
		class JitActionExpr : Xbyak::CodeGenerator
		{
		public:
			JitActionExpr(Assembly::Register reg)
			{
			}

			JitActionExpr(const JitActionExpr&){};
			JitActionExpr(JitActionExpr&&){};

		private:
		};
	}  // namespace JIT


	class FuncInfo
	{
	public:
		explicit constexpr FuncInfo(std::uintptr_t a_addr, std::string_view a_name) :
			_address(a_addr), _name(a_name)
		{
			_argCount = 0;
			_stackbase = 0;
			_stackframe = 0;
			std::fill_n(_buf.data(), _buf.size(), Assembly::NOP);
		}

		FuncInfo& operator[](const std::constructible_from<JIT::JitActionExpr> auto&... expr) {}

		// getter
		[[nodiscard]] constexpr auto address() const noexcept { return _address; }
		[[nodiscard]] constexpr auto name() const noexcept { return _name; }
		[[nodiscard]] constexpr auto arg_count() const noexcept { return _argCount; }
		[[nodiscard]] constexpr auto stackbase() const noexcept { return _stackbase; }
		[[nodiscard]] constexpr auto stackframe() const noexcept { return _stackframe; }
		[[nodiscard]] constexpr auto& buffer() const noexcept { return _buf; }
		[[nodiscard]] constexpr auto prolog() const noexcept { return _prolog; }
		[[nodiscard]] constexpr auto epilog() const noexcept { return _epilog; }

	private:
		const std::uintptr_t _address;
		const std::string_view _name;
		std::uint8_t _argCount;
		std::vector<JIT::JitActionExpr> _argExpr;
		std::uintptr_t _stackbase;
		std::uintptr_t _stackframe;
		std::array<OpCode, CAVE_BUF_SIZE> _buf;
		std::span<OpCode> _prolog;
		std::span<OpCode> _epilog;
	};
}  // namespace DKUtil::Hook