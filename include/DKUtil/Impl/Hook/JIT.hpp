#pragma once

#include "assembly.hpp"

#define FUNC_INFO(FUNC)                           \
	DKUtil::Hook::FuncInfo                        \
	{                                             \
		reinterpret_cast<std::uintptr_t>(FUNC),   \
			DKUtil::Hook::GetFuncArgsCount(FUNC), \
			#FUNC                                 \
	}
#define RT_INFO(FUNC, NAME)                     \
	DKUtil::Hook::FuncInfo                      \
	{                                           \
		reinterpret_cast<std::uintptr_t>(FUNC), \
			0,                                  \
			NAME                                \
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

	using namespace Assembly;

#pragma pack(push, 1)
	using REX = OpCode;
	using VEX = OpCode;
	using ModRM = OpCode;
	using SIndex = OpCode;

	struct REX2
	{
		REX Prefix;
		REX Suffix;
	};

	struct VEX2
	{
		VEX Prefix;
		VEX Suffix;
	};

	// near
	template <bool Retn = false>
	struct _BranchNear
	{
		constexpr _BranchNear(Disp32 a_disp = 0) :
			Op(Retn ? 0xE8 : 0xE9), Disp(a_disp)
		{}

		OpCode Op = 0xE8;  // cd
		Disp32 Disp = 0x00000000;
	};
	using CallRel = _BranchNear<true>;
	using JmpRel = _BranchNear<false>;
	static_assert(sizeof(CallRel) == 0x5);
	static_assert(sizeof(JmpRel) == 0x5);

	// indirect
	template <bool Retn = false>
	struct _BranchIndirect
	{
		constexpr _BranchIndirect(Disp32 a_disp = 0) noexcept :
			Rm(Retn ? 0x15 : 0x25), Disp(a_disp)
		{}

		OpCode Op = 0xFF;  // 2 | 4
		ModRM  Rm = 0x25;  // 1 0 1
		Disp32 Disp = 0x00000000;
	};
	using CallRip = _BranchIndirect<true>;
	using JmpRip = _BranchIndirect<false>;
	static_assert(sizeof(CallRip) == 0x6);
	static_assert(sizeof(JmpRip) == 0x6);

	// 1 byte
	template <bool Add = false>
	struct _SubRsp
	{
		constexpr _SubRsp(Imm8 a_size = 0) noexcept :
			Rm(Add ? 0xC4 : 0xEC), Size(a_size)
		{}

		REX    W = 0x48;
		OpCode Op = 0x83;  // 0 | 5 ib
		ModRM  Rm = 0xEC;  // 1 0 0
		Imm8   Size = 0x00;
	};
	using SubRsp = _SubRsp<false>;
	static_assert(sizeof(SubRsp) == 0x4);
	using AddRsp = _SubRsp<true>;
	static_assert(sizeof(AddRsp) == 0x4);

	// 4 bytes
	template <bool Add = false>
	struct _SubRspEx
	{
		constexpr _SubRspEx(Imm32 a_size = 0) noexcept :
			Rm(Add ? 0xC4 : 0xEC), Size(a_size)
		{}

		REX    W = 0x48;
		OpCode Op = 0x81;  // 0 | 5 ib
		ModRM  Rm = 0xEC;  // 1 0 0
		Imm32  Size = 0x00000000u;
	};
	using SubRspEx = _SubRspEx<false>;
	static_assert(sizeof(SubRspEx) == 0x7);
	using AddRspEx = _SubRspEx<true>;
	static_assert(sizeof(AddRspEx) == 0x7);

#pragma region PushR64

	struct PushImm64
	{
		explicit constexpr PushImm64(Imm64 addr) noexcept :
			Low(addr >> 32), High(addr & 0xFFFFFFFFLL)
		{}

		constexpr auto full() noexcept { return static_cast<Imm64>(Low) << 32 | High; }

		OpCode OpPush = 0x68;  // id
		Imm32  Low = 0x00000000u;
		OpCode OpMov = 0xC7;  // 0 id
		ModRM  Rm = 0x44;     // 1 0 0
		SIndex Sib = 0x24;
		Disp8  Disp = sizeof(Imm32);
		Imm32  High = 0x00000000u;
	};
	static_assert(sizeof(PushImm64) == 0xD);

	template <bool Pop = false>
	struct _PushR64
	{
		explicit constexpr _PushR64(enumeration<Register> a_reg) noexcept
		{
			if constexpr (Pop) {
				Push += a_reg.any(Register::RF) ? 0x1 : 0x8;
			}

			const auto rm = std::bit_width(a_reg.underlying()) + (a_reg.any(Register::RF) ? 0x42 : -0x2);
			Push += rm;

			dku_assert(Push <= 0x5F || Push == 0x9C || Push == 0x9D,
				"Use [Push|Pop]R64B for REX.B operations (2 bytes opcode)");
		}

		OpCode Push = 0x50;  // id
	};
	using PushR64 = _PushR64<false>;
	static_assert(sizeof(PushR64) == 0x1);
	using PopR64 = _PushR64<true>;
	static_assert(sizeof(PopR64) == 0x1);

	template <bool Pop = false>
	struct _PushR64B
	{
		explicit constexpr _PushR64B(enumeration<Register> a_reg) noexcept
		{
			if constexpr (Pop) {
				Push += 0x8;
			}

			const auto rm = std::bit_width(a_reg.underlying()) - 0xB;
			Push += rm;

			dku_assert(Push <= 0x5F,
				"Use [Push|Pop]R64 for base operations (1 byte opcode)");
		}

		REX    B = 0x41;
		OpCode Push = 0x50;  // id
	};
	using PushR64B = _PushR64B<false>;
	static_assert(sizeof(PushR64B) == 0x2);
	using PopR64B = _PushR64B<true>;
	static_assert(sizeof(PopR64B) == 0x2);

#pragma endregion

	// only prefix, the offset encoding is required to append
	template <bool Aligned = false>
	struct _V_MovDq_Rsp_Ex
	{
		explicit constexpr _V_MovDq_Rsp_Ex(enumeration<SIMD> a_simd) noexcept
		{
			const auto rm = std::bit_width(a_simd.underlying()) - 0x2;
			Rm += rm * 0x8;

			// extended
			if (rm >= 8) {
				V.Suffix = Aligned ? 0x79 : 0x7A;
				Rm -= 0x40;
			}
		}

		constexpr auto ReverseOperand() noexcept
		{
			Op = (Op == 0x7F ? 0x6F : 0x7F);
			return *this;
		}

		constexpr auto NoSibAddress() noexcept
		{
			Rm -= 0x40;
			return *this;
		}

		constexpr auto ExtendedRange() noexcept
		{
			Rm += 0x40;
			return *this;
		}

		VEX2   V = { 0xC5, Aligned ? 0xF9 : 0xFA };
		OpCode Op = 0x7F;
		ModRM  Rm = 0x44;   // 1 0 100 // explicit offset encoding
		SIndex Sib = 0x24;  // 0 1 0 // rsp
	};
	using MovDquRsp = _V_MovDq_Rsp_Ex<false>;
	static_assert(sizeof(MovDquRsp) == 0x5);
	using MovDqaRsp = _V_MovDq_Rsp_Ex<true>;
	static_assert(sizeof(MovDqaRsp) == 0x5);

#pragma pack(pop)

	namespace JIT
	{
		/** This namespace contains util functions used internally to generate patches */

		using patch_descriptor = std::pair<Patch, Patch>;
		using patch_block = std::pair<std::vector<OpCode>, std::size_t>;

		/** \brief Allocates a block of memory and generates non volatile patch
		 * \brief push/pop to preserve registers
		 * \param a_regs : { reg1, reg2, reg3... } registers to preserve
		 * \return patch_descriptor : a pair of views of prolog and epilog of this patch
		 */
		inline patch_descriptor MakeNonVolatilePatch(enumeration<Register> a_regs)
		{
			static std::unordered_map<Register, patch_block> Patches;

			if (a_regs.any(Register::NONE)) {
				return {};
			}

			auto& [buf, end] = Patches[a_regs.get()];

			if (buf.empty() || !end) {
				// size
				std::size_t bufSize{ 0 };
#if defined(DKU_H_JIT_MAX_SIZE_BUFFER)
				constexpr auto Block = std::bit_width(std::to_underlying(Register::ALL)) * sizeof(PushR64B);
				bufSize = Block * 2;
#else
				for (auto reg : a_regs.flag_range(Register::RAX, Register::R15)) {
					if (a_regs.any(reg, Register::ALL)) {
						bufSize += (sizeof(PushR64B) + sizeof(PopR64B));
					}
				}
#endif
				buf.resize(bufSize, INT3);
				end = 0;

				// 1 byte
				for (auto reg : a_regs.flag_range(Register::RAX, Register::RDI)) {
					if (a_regs.any(reg, Register::ALL)) {
						buf[end++] = std::bit_cast<OpCode>(PushR64(reg));
						buf[buf.size() - end] = std::bit_cast<OpCode>(PopR64(reg));
					}
				}

				if (a_regs.any(Register::RF, Register::ALL)) {
#if defined(DKU_H_JIT_ALT_PUSHFQ_POPFQ)
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
						AsMemCpy(buf.data() + end, PushR64B(reg));
						end += 2;
						AsMemCpy(buf.data() + buf.size() - end, PopR64B(reg));
					}
				}
			}

			return patch_descriptor{
				{ buf.data(), end },
				{ buf.data() + buf.size() - end, end }
			};
		}

		/** \brief Allocates a block of memory and generates non volatile patch
		 * \brief push/pop to preserve SSE registers
		 * \param a_regs : { simd1, simd2, simd3... } registers to preserve
		 * \return patch_descriptor : a pair of views of prolog and epilog of this patch
		 */
		inline patch_descriptor MakeNonVolatilePatch(enumeration<SIMD> a_simds)
		{
			static std::unordered_map<SIMD, patch_block> Patches;

			if (a_simds.any(SIMD::NONE)) {
				return {};
			}

			auto& [buf, end] = Patches[a_simds.get()];

			if (buf.empty() || !end) {
				// size
				std::size_t bufSize{ sizeof(SubRspEx) };
#if defined(DKU_H_JIT_MAX_SIZE_BUFFER)
				constexpr auto Block = std::bit_width(std::to_underlying(SIMD::ALL)) * sizeof(MovDquRsp);
				bufSize += Block * 2;
#else
				auto count = 0;
				for (auto simd : a_simds.flag_range(SIMD::XMM0, SIMD::XMM15)) {
					if (a_simds.any(simd, SIMD::ALL)) {
						count++;
						bufSize += sizeof(MovDquRsp);
					}
				}

				bufSize += count >= 8 ? count * 0x4 : count;
				bufSize *= 2;
#endif
				buf.resize(bufSize, INT3);
				end = sizeof(SubRspEx);

				// prefix in
				Disp32 stackSize{ 0 };
				for (auto simd : a_simds.flag_range(SIMD::XMM0, SIMD::XMM15)) {
					if (a_simds.any(simd, SIMD::ALL)) {
						auto push = MovDquRsp(simd);
						auto pop = MovDquRsp(simd).ReverseOperand();

						if (stackSize) {
							if (stackSize < 0x80) {
								AsMemCpy(buf.data() + end, push);
								end += sizeof(push);
								buf[end++] = static_cast<Disp8>(stackSize);

								AsMemCpy(buf.data() + buf.size() - end, pop);
								buf[buf.size() - end + sizeof(pop)] = static_cast<Disp8>(stackSize);
							} else {
								AsMemCpy(buf.data() + end, push.ExtendedRange());
								end += sizeof(push);
								AsMemCpy(buf.data() + end, stackSize);
								end += sizeof(stackSize);

								AsMemCpy(buf.data() + buf.size() - end, pop.ExtendedRange());
								AsMemCpy(buf.data() + buf.size() - end + sizeof(pop), stackSize);
							}
						} else {
							AsMemCpy(buf.data() + end, push.NoSibAddress());
							end += sizeof(push);
							AsMemCpy(buf.data() + buf.size() - end, pop.NoSibAddress());
						}

						// reserve
						stackSize += 0x10;
					}
				}

				// alloc
				AsMemCpy(buf.data(), SubRspEx(stackSize));
				AsMemCpy(buf.data() + buf.size() - sizeof(SubRspEx), AddRspEx(stackSize));
			}

			return patch_descriptor{
				{ buf.data(), end },
				{ buf.data() + buf.size() - end, end }
			};
		}
	}  // namespace JIT
}  // namespace DKUtil::Hook
