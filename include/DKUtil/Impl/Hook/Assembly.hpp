#pragma once

#include "shared.hpp"

namespace DKUtil::Hook::Assembly
{
	constexpr OpCode NOP = 0x90;
	constexpr OpCode INT3 = 0xCC;
	constexpr OpCode RET = 0xC3;

	enum class Register : std::uint32_t
	{
		NONE = 1u << 0,

		RAX = 1u << 1,
		RCX = 1u << 2,
		RDX = 1u << 3,
		RBX = 1u << 4,
		RSP = 1u << 5,
		RBP = 1u << 6,
		RSI = 1u << 7,
		RDI = 1u << 8,

		RF = 1u << 9,

		R8 = 1u << 10,
		R9 = 1u << 11,
		R10 = 1u << 12,
		R11 = 1u << 13,
		R12 = 1u << 14,
		R13 = 1u << 15,
		R14 = 1u << 16,
		R15 = 1u << 17,

		ALL = 1u << 18,
	};

	enum class SIMD : std::uint32_t
	{
		NONE = 1u << 0,

		XMM0 = 1u << 1,
		XMM1 = 1u << 2,
		XMM2 = 1u << 3,
		XMM3 = 1u << 4,
		XMM4 = 1u << 5,
		XMM5 = 1u << 6,
		XMM6 = 1u << 7,
		XMM7 = 1u << 8,
		XMM8 = 1u << 9,
		XMM9 = 1u << 10,
		XMM10 = 1u << 11,
		XMM11 = 1u << 12,
		XMM12 = 1u << 13,
		XMM13 = 1u << 14,
		XMM14 = 1u << 15,
		XMM15 = 1u << 16,

		ALL = 1u << 17,
	};

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
			[[nodiscard]] inline constexpr std::byte hexachar_to_hexadec(char a_hi, char a_lo) noexcept
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

			void match_or_fail(std::uintptr_t a_address) const
			{
				if (!this->match(a_address)) {
					ERROR(
						"A pattern has failed to match.\n"
						"This means the plugin is incompatible with the current version of the binary.\n"
						"Check if an update is available.\n");
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

		inline constexpr std::byte WILDCARD{ 0x00 };

		[[nodiscard]] inline std::string sanitize(std::string_view a_pattern)
		{
			std::string pattern = string::trim_copy(string::replace_all(a_pattern, "0x"));

			auto report_sanitizer_problem = [&](std::size_t i, std::string_view err = {}) {
				std::string_view prev{ pattern.data(), i };
				std::string_view next{ pattern.data() + i + 1 };
				FATAL("Can't sanitize pattern:\n{}\n{}{{{}}}{}", err, prev, pattern[i], next);
			};

			// 1) pattern can't begin with wildcards
			if (characters::wildcard(pattern[0])) {
				report_sanitizer_problem(0, "pattern can't begin with wildcards");
			}

			std::size_t digits{ 0 };
			for (std::size_t i = 0; i < pattern.size(); ++i) {
				char c = pattern[i];

				if (!characters::hexadecimal(c) &&
					!characters::whitespace(c) &&
					!characters::wildcard(c)) {
					// 2) xdigit only
					report_sanitizer_problem(i, "invalid hex character");
				}

				digits = characters::whitespace(c) ? 0 : digits + 1;
			}

			// 3) must be 2 char per byte
			if (digits % 2) {
				report_sanitizer_problem(pattern.size() - 1, "each byte must be 2 characters");
			}

			return pattern;
		}

		struct ByteMatch
		{
			std::byte hex;
			bool      wildcard;
		};

		[[nodiscard]] inline std::vector<ByteMatch> make_byte_matches(std::string_view a_pattern)
		{
			std::vector<Pattern::ByteMatch> bytes;

			for (std::size_t i = 0; i < a_pattern.size(); ++i) {
				switch (a_pattern[i]) {
				case ' ':
					break;
				case '?':
					bytes.emplace_back(Pattern::WILDCARD, true);
					++i;
					break;
				default:
					bytes.emplace_back(dku::string::lexical_cast<std::byte>({ a_pattern.data() + i, 2 }, true), false);
					++i;
					break;
				}
			}

			return bytes;
		}
	}  // namespace detail

	/** \brief Search a byte pattern in memory, kmp.
	 * \param a_pattern : hex string pattern of bytes. Spacing is optional. e.g. "FF 15 ????????".
	 * \param a_base : base address of memory block to search, default to module textx section.
	 * \param a_size : size of memory block to search, default to module textx size.
	 * \return void* : pointer of first match, nullptr if none found.
	 */
	[[nodiscard]] inline std::byte* search_pattern(
		std::string_view a_pattern,
		model::concepts::dku_memory auto a_base = 0,
		std::size_t      a_size = 0)
	{
		std::uintptr_t base{ AsAddress(a_base) };

		auto [textx, size] = Module::get().section(Module::Section::textx);

		if (!base) {
			base = textx;
		}

		if (!a_size) {
			a_size = size;
		}

		auto* begin = static_cast<std::byte*>(AsPointer(base));
		auto*  end = adjust_pointer(begin, a_size);
		auto  bytes = Pattern::make_byte_matches(Pattern::sanitize(a_pattern));

		constexpr std::size_t    NPOS = static_cast<std::size_t>(-1);
		std::vector<std::size_t> prefix(bytes.size() + 1);
		std::size_t              pos{ 1 };
		std::size_t              cnd{ 0 };

		prefix[0] = NPOS;

		while (pos < bytes.size()) {
			if (bytes[pos].wildcard ||
				bytes[cnd].wildcard ||
				bytes[pos].hex == bytes[cnd].hex) {
				prefix[pos] = prefix[cnd];
			} else {
				prefix[pos] = cnd;
				cnd = prefix[cnd];
				while (cnd != NPOS &&
					   !bytes[pos].wildcard &&
					   !bytes[cnd].wildcard &&
					   bytes[pos].hex != bytes[cnd].hex) {
					cnd = prefix[cnd];
				}
			}
			++pos;
			++cnd;
		}

		prefix[pos] = cnd;

		std::size_t    j{ 0 };
		std::size_t    k{ 0 };
		std::ptrdiff_t firstMatch{ -1 };

		while (j < a_size) {
			if (bytes[k].wildcard || bytes[k].hex == *adjust_pointer<std::byte>(begin, j)) {
				++j;
				++k;
				if (k == bytes.size()) {
					firstMatch = j - k;
					break;
				}
			} else {
				k = prefix[k];
				if (k == NPOS) {
					++j;
					++k;
				}
			}
		}

		if (firstMatch == -1) {
			return nullptr;
		} else {
			return adjust_pointer<std::byte>(begin, firstMatch);
		}
	}

	/** \brief Search a byte pattern in memory, linear.
	 * \brief To use pattern as argument, use search_pattern<T>(pattern) instead.
	 * \param a_base : base address of memory block to search, default to module textx section.
	 * \param a_size : size of memory block to search, default to module textx size.
	 * \return void* : pointer of first match, nullptr if none found.
	 */
	template <Pattern::PatternMatcher P>
	[[nodiscard]] inline void* search_pattern(std::uintptr_t a_base = 0, std::size_t a_size = 0) noexcept
	{
		auto [textx, size] = Module::get().section(Hook::Module::Section::textx);

		if (!a_base) {
			a_base = textx;
		}

		if (!a_size) {
			a_size = size;
		}

		const auto* begin = static_cast<std::byte*>(AsPointer(a_base));
		const auto* end = adjust_pointer(begin, a_size);
		
		for (auto* mem = begin; mem != end; ++mem) {
			if (P.match(AsAddress(mem))) {
				return AsPointer(mem);
			}
		}

		return nullptr;
	}

	/** \brief Search a byte pattern in memory, linear.
	 * \brief To use pattern as argument, use search_pattern<T>(pattern) instead.
	 * \param a_base : base address of memory block to search, default to module textx section.
	 * \param a_size : size of memory block to search, default to module textx size.
	 * \return void* : pointer of first match, nullptr if none found.
	 */
	template <string::static_string S>
	[[nodiscard]] inline void* search_pattern(std::uintptr_t a_base = 0, std::size_t a_size = 0) noexcept
	{
		return search_pattern<Pattern::do_make_pattern<S>()>();
	}
}  // namespace DKUtil::Hook::Assembly
