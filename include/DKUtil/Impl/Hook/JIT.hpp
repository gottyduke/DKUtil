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
			std::uintptr_t   a_addr,
			std::uint8_t     a_argsCount,
			std::string_view a_name) :
			_address(a_addr),
			_argsCount(a_argsCount),
			_name(a_name)
		{}

		[[nodiscard]] constexpr auto address() const noexcept { return _address; }
		[[nodiscard]] constexpr auto name() const noexcept { return _name; }
		[[nodiscard]] constexpr auto args_count() const noexcept { return _argsCount; }

	private:
		const std::uintptr_t   _address;
		const std::uint8_t     _argsCount;
		const std::string_view _name;
	};


	namespace JIT
	{
		using patch_descriptor = std::pair<std::span<OpCode>, std::span<OpCode>>;
		using patch_block = std::pair<std::vector<OpCode>, std::size_t>;

		/* @brief Allocates a block of memory and generates non volatile patch
		 * @brief push/pop to preserve registers
		 * @param a_regs : { reg1, reg2, reg3... } registers to preserve
		 * @returns patch_descriptor : a pair of views of prolog and epilog of this patch
		 */
		inline patch_descriptor MakeNonVolatilePatch(model::enumeration<Assembly::Register> a_regs)
		{
			using namespace Assembly;

			static std::unordered_map<Register, patch_block> Patches;
			constexpr auto                                   Block = std::bit_width(std::to_underlying(Register::ALL)) * 0x2;

			dku_assert(a_regs.none(Register::NONE), 
				"DKU_H: Cannot make patch for Register::NONE");

			auto& [buf, end] = Patches[a_regs.get()];

			if (buf.empty() || !end) {
				buf.resize(Block * 2, INT3);

				// 1 byte
				for (auto reg : a_regs.flag_range(Register::RAX, Register::RDI)) {
					if (a_regs.any(reg, Register::ALL)) {
						buf[end++] = std::bit_cast<OpCode>(PushR64(reg));
						buf[buf.size() - end] = std::bit_cast<OpCode>(PopR64(reg));
					}
				}

				if (a_regs.any(Register::RF, Register::ALL)) {
#if defined(DKU_H_ALT_PUSHFQ_POPFQ)
					// rf: lahf sahf
					if (a_regs.none(Register::RAX, Register::ALL)) {
						buf[end++] = std::bit_cast<OpCode>(PushR64(Register::RAX));
						buf[buf.size() - end] = std::bit_cast<OpCode>(PopR64(Register::RAX));
					}

					buf[end++] = std::bit_cast<OpCode>(PushR64(Register::RF)) + 0x3;
					buf[buf.size() - end] = std::bit_cast<OpCode>(PopR64(Register::RF)) + 0x1;

					buf[end++] = std::bit_cast<OpCode>(PushR64(Register::RAX));
					buf[buf.size() - end] = std::bit_cast<OpCode>(PopR64(Register::RAX));
#else
					// rf: pushf popf
					buf[end++] = std::bit_cast<OpCode>(PushR64(Register::RF));
					buf[buf.size() - end] = std::bit_cast<OpCode>(PopR64(Register::RF));
#endif
				}

				// 2 bytes
				for (auto reg : a_regs.flag_range(Register::R8, Register::R15)) {
					if (a_regs.any(reg, Register::ALL)) {
						*std::bit_cast<PushR64B*>(buf.data() + end) = PushR64B(reg);
						end += 2;
						*std::bit_cast<PopR64B*>(buf.data() + buf.size() - end) = PopR64B(reg);
					}
				}
			}

			return patch_descriptor{
				{ buf.data(), end },
				{ buf.data() + buf.size() - end, end }
			};
		}

		/* @brief Allocates a block of memory and generates non volatile patch
		 * @brief push/pop to preserve SSE registers
		 * @param a_regs : { simd1, simd2, simd3... } registers to preserve
		 * @returns patch_descriptor : a pair of views of prolog and epilog of this patch
		 */
		inline void MakeNonVolatilePatch(model::enumeration<Assembly::SIMD> a_regs)
		{
			static std::unordered_map<Assembly::SIMD, patch_block> Patches;

		}
	} // namespace DKUtil::Hook::JIT
}  // namespace DKUtil::Hook
