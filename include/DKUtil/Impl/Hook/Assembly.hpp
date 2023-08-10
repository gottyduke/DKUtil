#pragma once


#include "shared.hpp"


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
	static_assert(sizeof(PushR64<false>) == 0x1);

	using PopR64 = PushR64<true>;
	static_assert(sizeof(PopR64) == 0x1);


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
	static_assert(sizeof(PushR64W<false>) == 0x2);

	using PopR64W = PushR64W<true>;
	static_assert(sizeof(PopR64W) == 0x2);

#pragma pack(pop)

	
	namespace Pattern
	{
		namespace characters
		{
			[[nodiscard]] inline constexpr bool hexadecimal(char a_ch) noexcept
			{
				return (a_ch >= '0' && a_ch <= '9') ||
				       (a_ch >= 'a' && a_ch <= 'f') ||
				       (a_ch >= 'A' && a_ch <= 'F');
			}

			[[nodiscard]] inline constexpr bool whitespace(char a_ch) noexcept
			{
				return a_ch == ' ';
			}

			[[nodiscard]] inline constexpr bool wildcard(char a_ch) noexcept
			{
				return a_ch == '?';
			}
		}  // namespace characters

		namespace rules
		{
			[[nodiscard]] inline consteval std::byte hexachar_to_hexadec(char a_hi, char a_lo) noexcept
			{
				constexpr auto lut = []() noexcept {
					std::array<std::uint8_t, std::numeric_limits<unsigned char>::max() + 1> a{};

					const auto iterate = [&](std::uint8_t a_iFirst, unsigned char a_cFirst, unsigned char a_cLast) noexcept {
						for (; a_cFirst <= a_cLast; ++a_cFirst, ++a_iFirst) {
							a[a_cFirst] = a_iFirst;
						}
					};

					iterate(0x0, '0', '9');
					iterate(0xa, 'a', 'f');
					iterate(0xA, 'A', 'F');

					return a;
				}();

				return static_cast<std::byte>(
					lut[static_cast<unsigned char>(a_hi)] * 0x10u +
					lut[static_cast<unsigned char>(a_lo)]);
			}


			template <char HI, char LO>
			class Hexadecimal
			{
			public:
				[[nodiscard]] static constexpr bool match(std::byte a_byte) noexcept
				{
					constexpr auto expected = hexachar_to_hexadec(HI, LO);
					return a_byte == expected;
				}
			};


			class Wildcard
			{
			public:
				[[nodiscard]] static constexpr bool match(std::byte) noexcept { return true; }
			};

			template <char, char>
			void rule_for() noexcept;

			template <char C1, char C2>
			Hexadecimal<C1, C2> rule_for() noexcept
				requires(characters::hexadecimal(C1) && characters::hexadecimal(C2));

			template <char C1, char C2>
			Wildcard rule_for() noexcept
				requires(characters::wildcard(C1) && characters::wildcard(C2));
		}  // namespace rules

		template <class... Rules>
		class PatternMatcher
		{
		public:
			static_assert(sizeof...(Rules) >= 1, "must provide at least 1 rule for the pattern matcher");

			[[nodiscard]] constexpr bool match(std::span<const std::byte, sizeof...(Rules)> a_bytes) const noexcept
			{
				std::size_t i = 0;
				return (Rules::match(a_bytes[i++]) && ...);
			}

			[[nodiscard]] bool match(std::uintptr_t a_address) const noexcept
			{
				return this->match(*reinterpret_cast<const std::byte(*)[sizeof...(Rules)]>(a_address));
			}

			[[nodiscard]] bool match(void* a_address) const noexcept
			{
				return this->match(*reinterpret_cast<const std::byte(*)[sizeof...(Rules)]>(a_address));
			}

			void match_or_fail(std::uintptr_t a_address) const
			{
				if (!this->match(a_address)) {
					ERROR(
						"A pattern has failed to match.\n"
						"This means the plugin is incompatible with the current version of the game.\n"
						"Head to the mod page of this plugin to see if an update is available.\n");
				}
			}

			[[nodiscard]] consteval auto size() const noexcept { return sizeof...(Rules); }
		};

		inline void consteval_error(const char* a_error) noexcept;

		template <string::static_string S, class... Rules>
		[[nodiscard]] inline constexpr auto do_make_pattern() noexcept
		{
			if constexpr (S.length() == 0) {
				return PatternMatcher<Rules...>();
			} else if constexpr (S.length() == 1) {
				constexpr char c = S[0];
				if constexpr (characters::hexadecimal(c) || characters::wildcard(c)) {
					consteval_error("the given pattern has an unpaired rule (rules are required to be written in pairs of 2)");
				} else {
					consteval_error("the given pattern has trailing characters at the end (which is not allowed)");
				}
			} else {
				using rule_t = decltype(rules::rule_for<S[0], S[1]>());
				if constexpr (std::same_as<rule_t, void>) {
					consteval_error("the given pattern failed to match any known rules");
				} else {
					if constexpr (S.length() <= 3) {
						return do_make_pattern<S.template substr<2>(), Rules..., rule_t>();
					} else if constexpr (characters::whitespace(S[2])) {
						return do_make_pattern<S.template substr<3>(), Rules..., rule_t>();
					} else {
						consteval_error("a space character is required to split byte patterns");
					}
				}
			}
		}

		template <class... Bytes>
		[[nodiscard]] inline constexpr auto make_byte_array(Bytes... a_bytes) noexcept
			-> std::array<std::byte, sizeof...(Bytes)>
		{
			static_assert((std::integral<Bytes> && ...), "all bytes must be an integral type");
			return { static_cast<std::byte>(a_bytes)... };
		}
	}  // namespace detail

	template <string::static_string S>
	[[nodiscard]] inline constexpr auto make_pattern() noexcept
	{
		return Pattern::do_make_pattern<S>();
	}

	template <Pattern::PatternMatcher P>
	[[nodiscard]] inline void* search_pattern(std::uintptr_t a_base = 0, std::size_t a_size = 0) noexcept
	{
		auto& base = Module::get();
		auto [textx, size] = base.section(dku::Hook::Module::Section::textx);

		if (!a_base) {
			a_base = textx;
		}

		if (!a_size) {
			a_size = size;
		}

		const auto* begin = static_cast<std::byte*>(AsPointer(a_base));
		const auto* end = adjust_pointer(begin, a_size);

		for (auto* mem = begin; mem != end; ++mem) {
			if (P.match(mem)) {
				return mem;
			}
		}

		return nullptr;
	}

	template <string::static_string S>
	[[nodiscard]] inline void* search_pattern(std::uintptr_t a_base = 0, std::size_t a_size = 0) noexcept
	{
		return search_pattern<make_pattern<S>()>(a_base, a_size);
	}
}  // namespace DKUtil::Hook::Assembly
