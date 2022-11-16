#pragma once


#include "Shared.hpp"


namespace DKUtil::Hook::Assembly
{
	constexpr OpCode NOP = 0x90;
	constexpr OpCode INT3 = 0xCC;
	constexpr OpCode RET = 0xC3;

	enum class Register : std::uint32_t
	{
		NONE = 0u,

		RAX = 1u << 0,
		RCX = 1u << 1,
		RDX = 1u << 2,
		RBX = 1u << 3,
		RSP = 1u << 4,
		RBP = 1u << 5,
		RSI = 1u << 6,
		RDI = 1u << 7,

		RF = 1u << 8,

		R8 = 1u << 9,
		R9 = 1u << 10,
		R10 = 1u << 11,
		R11 = 1u << 12,
		R12 = 1u << 13,
		R13 = 1u << 14,
		R14 = 1u << 15,
		R15 = 1u << 16,
	};

#pragma pack(push, 1)
#define DEF_ASM                                 \
	constexpr auto* data() noexcept             \
	{                                           \
		return std::bit_cast<std::byte*>(this); \
	}                                           \
	constexpr auto size() noexcept              \
	{                                           \
		return sizeof(*this);                   \
	}

	struct JmpRel
	{
		constexpr JmpRel(Disp32 disp = 0) :
			Disp(disp)
		{}

		DEF_ASM

		OpCode Jmp = 0xE9;  // cd
		Disp32 Disp = 0x00000000;
	};
	static_assert(sizeof(JmpRel) == 0x5);


	template <bool RETN = false>
	struct JmpRip
	{
		constexpr JmpRip(Disp32 disp = 0) :
			Rip(RETN ? 0x15 : 0x25), Disp(disp)
		{}

		DEF_ASM

		OpCode Jmp = 0xFF;  // 2 | 4
		ModRM Rip = 0x25;   // 1 0 1
		Disp32 Disp = 0x00000000;
	};
	static_assert(sizeof(JmpRip<true>) == 0x6);
	static_assert(sizeof(JmpRip<false>) == 0x6);
	using CallRip = JmpRip<true>;


	struct PushImm64
	{
		constexpr PushImm64(Imm64 addr = 0) :
			Low(addr >> 32), High(addr & 0xFFFFFFFFLL)
		{}

		DEF_ASM

		constexpr auto full() noexcept { return static_cast<Imm64>(Low) << 32 | High; }

		OpCode Push = 0x68;  // id
		Imm32 Low = 0x00000000u;
		OpCode Mov = 0xC7;  // 0 id
		ModRM Sib = 0x44;   // 1 0 0
		SIndex Rsp = 0x24;
		Disp8 Disp = sizeof(Imm32);
		Imm32 High = 0x00000000u;
	};
	static_assert(sizeof(PushImm64) == 0xD);


	template <bool ADD = false>
	struct SubRsp
	{
		constexpr SubRsp(Imm8 s = 0) :
			Rsp(ADD ? 0xC4 : 0xEC), Size(s)
		{}

		DEF_ASM

		REX W = 0x48;
		OpCode Sub = 0x83;  // 0 | 5 ib
		ModRM Rsp = 0xEC;   // 1 0 0
		Imm8 Size = 0x00;
	};
	static_assert(sizeof(SubRsp<true>) == 0x4);
	static_assert(sizeof(SubRsp<false>) == 0x4);
	using AddRsp = SubRsp<true>;


	template <bool POP = false>
	struct PushR64
	{
		constexpr PushR64(model::enumeration<Register> reg = Register::RAX)
		{
			if constexpr (POP) {
				Push += ((reg == Register::RF) ? 0x1 : 0x8);
			}

			constexpr auto rm = std::bit_cast<std::uint32_t>(reg);
			if constexpr (rm > 0xEC) {
				ERROR("Use PushR64W for REX.B operations (2 byte opcode)");
			}

			Push += rm;
		}

		DEF_ASM

		OpCode Push = 0x50;  // id
	};
	static_assert(sizeof(PushR64<true>) == 0x1);
	static_assert(sizeof(PushR64<false>) == 0x1);
	using PopR64 = PushR64<true>;


	template <bool POP = false>
	struct PushR64W
	{
		constexpr PushR64W(model::enumeration<Register> reg = Register::RAX)
		{
			if constexpr (POP) {
				Push += 0x8;
			}

			constexpr auto rm = std::bit_cast<std::uint32_t>(reg);
			if constexpr (rm <= 0xEC) {
				ERROR("Use PushR64 for base operations (1 byte opcode)");
			}

			Push += rm;
		}

		DEF_ASM

		REX B = 0x41;
		OpCode Push = 0x50;  // id
	};
	static_assert(sizeof(PushR64W<true>) == 0x2);
	static_assert(sizeof(PushR64W<false>) == 0x2);
	using PopR64W = PushR64W<true>;
#pragma pack(pop)
}  // namespace DKUtil::Hook
