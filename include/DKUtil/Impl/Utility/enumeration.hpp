#pragma once

#include "string.hpp"

// 64 by default, nums(64) or bits(1<<64)
#ifndef DKU_MAX_REFLECTION_ENUM
#	define DKU_MAX_REFLECTION_ENUM 64
#endif

namespace DKUtil::model
{
	namespace detail
	{
		template <class Enum>
		struct enum_meta
		{
			static_assert(DKU_MAX_REFLECTION_ENUM > 0);

			using raw_type = std::underlying_type_t<Enum>;
			using bits_type = std::make_unsigned_t<raw_type>;

			static constexpr std::size_t bit_digits = std::numeric_limits<bits_type>::digits;

			static constexpr std::uint64_t linear_last = (std::min)(static_cast<std::uint64_t>(DKU_MAX_REFLECTION_ENUM),
				static_cast<std::uint64_t>((std::numeric_limits<raw_type>::max)()));
			static constexpr std::size_t   linear_count = static_cast<std::size_t>(linear_last) + 1;

			static constexpr std::size_t bit_first = static_cast<std::size_t>(std::bit_width(linear_last));
			static constexpr std::size_t bit_last = (std::min)(bit_digits, static_cast<std::size_t>(DKU_MAX_REFLECTION_ENUM) + 1);
			static constexpr std::size_t bit_count = bit_last > bit_first ? bit_last - bit_first : 0;

			static constexpr std::size_t sample_count = linear_count + bit_count;

			struct entry
			{
				bits_type        value;
				std::string_view name;
				std::string_view shortName;
			};

			template <Enum E>
			static consteval std::string_view parse_name() noexcept
			{
				constexpr std::string_view prefix = "::parse_name<";
				constexpr std::string_view suffix = ">(void)";

				std::string_view sig = __FUNCSIG__;
				const auto       begin = sig.find(prefix) + prefix.size();
				const auto       name = sig.substr(begin, sig.rfind(suffix) - begin);

				return name.find("(enum") == std::string_view::npos ? name : std::string_view{};
			}

			template <Enum E>
			static constexpr auto name_storage = [] {
				constexpr auto                name = parse_name<E>();
				std::array<char, name.size()> buf{};
				std::ranges::copy(name, buf.begin());
				return buf;
			}();

			template <std::size_t I>
			static consteval bits_type sample() noexcept
			{
				if constexpr (I < linear_count) {
					return static_cast<bits_type>(I);
				} else {
					return static_cast<bits_type>(bits_type{ 1 } << (bit_first + (I - linear_count)));
				}
			}

			template <std::size_t I>
			static consteval entry make_entry() noexcept
			{
				constexpr bits_type        value = sample<I>();
				constexpr auto&            storage = name_storage<static_cast<Enum>(value)>;
				constexpr std::string_view name{ storage.data(), storage.size() };
				constexpr auto             sep = name.rfind("::");

				return { value, name, sep == std::string_view::npos ? name : name.substr(sep + 2) };
			}

			template <std::size_t... I>
			static consteval auto scan(std::index_sequence<I...>) noexcept
			{
				return std::array<entry, sizeof...(I)>{ make_entry<I>()... };
			}

			static consteval auto scan_all() noexcept
			{
				return scan(std::make_index_sequence<sample_count>{});
			}

			static consteval std::size_t count_valid() noexcept
			{
				std::size_t count = 0;
				for (const auto& e : scan_all()) {
					count += !e.name.empty();
				}
				return count;
			}
		};

		template <class Enum>
		inline constexpr auto enum_entries = [] {
			using meta = enum_meta<Enum>;

			std::array<typename meta::entry, meta::count_valid()> result{};
			std::size_t                                           i = 0;
			for (const auto& e : meta::scan_all()) {
				if (!e.name.empty()) {
					result[i++] = e;
				}
			}
			return result;
		}();

		template <class Enum>
		inline constexpr std::string_view enum_type_name = [] {
			if constexpr (enum_entries<Enum>.empty()) {
				return std::string_view{};
			} else {
				constexpr auto name = enum_entries<Enum>[0].name;
				constexpr auto sep = name.rfind("::");
				return sep == std::string_view::npos ? std::string_view{} : name.substr(0, sep);
			}
		}();

		template <class Enum>
		inline constexpr bool enum_is_flag = [] {
			constexpr auto& entries = enum_entries<Enum>;

			std::size_t nonZero = 0;
			for (const auto& e : entries) {
				if (e.value == 0) {
					continue;
				}
				if (!std::has_single_bit(e.value)) {
					return false;
				}
				++nonZero;
			}

			if (nonZero < 2) {
				return false;
			}

			const auto span = static_cast<std::uint64_t>(entries.back().value - entries.front().value);
			return span != entries.size() - 1;
		}();

		template <class Enum>
		[[nodiscard]] constexpr const typename enum_meta<Enum>::entry* find_entry(typename enum_meta<Enum>::bits_type a_value) noexcept
		{
			constexpr auto& entries = enum_entries<Enum>;

			const auto it = std::ranges::lower_bound(entries, a_value, {}, &enum_meta<Enum>::entry::value);
			return it != entries.end() && it->value == a_value ? std::to_address(it) : nullptr;
		}
	}  // namespace detail

	// taken from CommonLib, added reflection + range adaptors
	template <class Enum,
		class Underlying = std::conditional_t<
			std::is_signed_v<std::underlying_type_t<Enum>>,
			std::uint32_t, std::underlying_type_t<Enum>>>
	class enumeration
	{
	public:
		using enum_type = Enum;
		using underlying_type = Underlying;

		static_assert(std::is_enum_v<enum_type>, "enum_type must be a scoped enum");
		static_assert(std::is_integral_v<underlying_type>, "underlying_type must be an integral");

		constexpr enumeration() noexcept = default;
		constexpr enumeration(const enumeration&) noexcept = default;
		constexpr enumeration(enumeration&&) noexcept = default;

		template <class U2>
		constexpr enumeration(enumeration<enum_type, U2> a_rhs) noexcept :
			_impl(static_cast<underlying_type>(a_rhs.get()))
		{}
		constexpr enumeration(const std::same_as<enum_type> auto... a_values) noexcept :
			_impl((static_cast<underlying_type>(a_values) | ...))
		{}
		constexpr enumeration(const std::convertible_to<underlying_type> auto... a_values) noexcept :
			_impl((static_cast<underlying_type>(a_values) | ...))
		{}

		~enumeration() noexcept = default;

		constexpr enumeration& operator=(const enumeration&) noexcept = default;
		constexpr enumeration& operator=(enumeration&&) noexcept = default;

		template <class U2>
		constexpr enumeration& operator=(enumeration<enum_type, U2> a_rhs) noexcept
		{
			_impl = static_cast<underlying_type>(a_rhs.get());
			return *this;
		}

		constexpr enumeration& operator=(enum_type a_value) noexcept
		{
			_impl = static_cast<underlying_type>(a_value);
			return *this;
		}

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return _impl != static_cast<underlying_type>(0); }

		[[nodiscard]] constexpr enum_type       operator*() const noexcept { return get(); }
		[[nodiscard]] constexpr auto            operator<=>(const std::three_way_comparable<underlying_type> auto& a_rhs) const noexcept { return _impl <=> a_rhs; }
		[[nodiscard]] constexpr auto            operator<=>(const enumeration<enum_type, underlying_type>& a_rhs) const noexcept { return _impl <=> a_rhs._impl; }
		[[nodiscard]] constexpr auto            operator==(const enumeration<enum_type, underlying_type>& a_rhs) const noexcept { return _impl == a_rhs._impl; }
		[[nodiscard]] constexpr auto            operator!=(const enumeration<enum_type, underlying_type>& a_rhs) const noexcept { return _impl != a_rhs._impl; }
		[[nodiscard]] constexpr enum_type       get() const noexcept { return static_cast<enum_type>(_impl); }
		[[nodiscard]] constexpr underlying_type underlying() const noexcept { return _impl; }

		constexpr enumeration& set(const std::same_as<enum_type> auto... a_args) noexcept
		{
			_impl |= (static_cast<underlying_type>(a_args) | ...);
			return *this;
		}

		constexpr enumeration& reset(const std::same_as<enum_type> auto... a_args) noexcept
		{
			_impl &= ~(static_cast<underlying_type>(a_args) | ...);
			return *this;
		}

		[[nodiscard]] constexpr bool any(const std::same_as<enum_type> auto... a_args) const noexcept
		{
			return (_impl & (static_cast<underlying_type>(a_args) | ...)) != static_cast<underlying_type>(0);
		}

		[[nodiscard]] constexpr bool all(const std::same_as<enum_type> auto... a_args) const noexcept
		{
			return (_impl & (static_cast<underlying_type>(a_args) | ...)) == (static_cast<underlying_type>(a_args) | ...);
		}

		[[nodiscard]] constexpr bool none(const std::same_as<enum_type> auto... a_args) const noexcept
		{
			return (_impl & (static_cast<underlying_type>(a_args) | ...)) == static_cast<underlying_type>(0);
		}

		[[nodiscard]] static constexpr bool is_flag() noexcept { return detail::enum_is_flag<enum_type>; }

		// static reflection
		[[nodiscard]] static constexpr std::string to_string(enum_type a_enum, bool a_full = false) noexcept
		{
			const auto bits = static_cast<bits_type>(std::to_underlying(a_enum));

			if (const auto* entry = detail::find_entry<enum_type>(bits)) {
				return std::string{ a_full ? entry->name : entry->shortName };
			}

			if constexpr (is_flag()) {
				return compose_flags(bits, a_full);
			} else {
				return {};
			}
		}

		// underlying adaptor
		[[nodiscard]] static constexpr std::string to_string(const std::convertible_to<underlying_type> auto a_value, bool a_full = false) noexcept
		{
			using value_type = std::remove_cvref_t<decltype(a_value)>;

			if constexpr (std::is_signed_v<value_type>) {
				if (a_value < 0) {
					return {};
				}
			}

			std::uint64_t value;
			if constexpr (std::is_integral_v<value_type>) {
				value = static_cast<std::uint64_t>(a_value);
			} else {
				value = static_cast<std::uint64_t>(static_cast<underlying_type>(a_value));
			}

			bits_type bits;
			if constexpr (is_flag()) {
				if (value >= detail::enum_meta<enum_type>::bit_digits) {
					return {};
				}
				bits = static_cast<bits_type>(bits_type{ 1 } << value);
			} else {
				if (value > (std::numeric_limits<bits_type>::max)()) {
					return {};
				}
				bits = static_cast<bits_type>(value);
			}

			const auto* entry = detail::find_entry<enum_type>(bits);
			return entry ? std::string{ a_full ? entry->name : entry->shortName } : std::string{};
		}

		// string cast
		[[nodiscard]] static constexpr std::optional<enum_type> from_string(std::string_view a_enumString, bool a_caseSensitive = false) noexcept
		{
			const auto match = [a_caseSensitive](std::string_view a_token) -> std::optional<bits_type> {
				constexpr std::string_view whitespace = " \t\r\n";

				const auto first = a_token.find_first_not_of(whitespace);
				if (first == std::string_view::npos) {
					return std::nullopt;
				}
				a_token = a_token.substr(first, a_token.find_last_not_of(whitespace) - first + 1);

				for (const auto& entry : detail::enum_entries<enum_type>) {
					if (a_caseSensitive ? entry.shortName == a_token : string::iequals(entry.shortName, a_token)) {
						return entry.value;
					}
				}

				return std::nullopt;
			};

			if constexpr (is_flag()) {
				if (a_enumString.contains('|')) {
					bits_type result{ 0 };
					for (const auto part : a_enumString | std::views::split('|')) {
						const auto bits = match(std::string_view{ part.begin(), part.end() });
						if (!bits) {
							return std::nullopt;
						}
						result = static_cast<bits_type>(result | *bits);
					}
					return static_cast<enum_type>(result);
				}
			}

			if (const auto bits = match(a_enumString)) {
				return static_cast<enum_type>(*bits);
			}

			return std::nullopt;
		}

		// enum name
		[[nodiscard]] static constexpr std::string_view enum_name() noexcept
		{
#ifndef DKU_SLIM
			return detail::enum_type_name<enum_type>;
#else
			return type_name();
#endif
		}

		// type name
		[[nodiscard]] static constexpr std::string_view type_name() noexcept { return typeid(underlying_type).name(); }

		// contiguous enum, linear transitive
		[[nodiscard]] static constexpr auto value_range(enum_type a_begin, enum_type a_end, std::int32_t a_step = 1) noexcept
		{
#ifndef DKU_SLIM
			if constexpr (is_flag()) {
				ERROR("value_range iterator called but enum is flag_type!\nEnum name: {}\nEnum type: {}", enum_name(), type_name());
			}
#endif
			if (a_begin == a_end || !a_step) {
				ERROR("Range iterator mandates different elements AND operable step value to construct a valid range!\nStep value provided: {}", a_step);
			}

			const auto begin = static_cast<std::int64_t>(std::to_underlying(a_begin));
			const auto end = static_cast<std::int64_t>(std::to_underlying(a_end));

			if ((end < begin) == (a_step > 0)) {
				a_step *= -1;
			}

			return std::views::iota(std::int64_t{ 0 }, (end - begin) / a_step + 1) |
			       std::views::transform([=](auto i) { return static_cast<enum_type>(begin + i * a_step); });
		}

		// bitflag enum, base 2 shift, l->m
		[[nodiscard]] static constexpr auto flag_range(enum_type a_begin, enum_type a_end) noexcept
		{
			dku_assert(a_begin != a_end,
				"Range iterator mandates different elements AND operable step value to construct a valid range!");

			auto lo = static_cast<bits_type>(std::to_underlying(a_begin));
			auto hi = static_cast<bits_type>(std::to_underlying(a_end));
			if (lo > hi) {
				std::swap(lo, hi);
			}

			const int first = lo ? std::bit_width(lo) - 1 : 0;
			const int last = std::bit_width(hi) - 1;

			return std::views::iota(first, last + 1) |
			       std::views::transform([](int i) { return static_cast<enum_type>(static_cast<bits_type>(bits_type{ 1 } << i)); });
		}

		[[nodiscard]] static constexpr underlying_type index_of(enum_type a_enum) noexcept
		{
			return static_cast<underlying_type>(std::bit_width(static_cast<underlying_type>(std::to_underlying(a_enum))));
		}

	private:
		using bits_type = typename detail::enum_meta<enum_type>::bits_type;

		static constexpr std::string compose_flags(bits_type a_bits, bool a_full) noexcept
		{
			std::string result;

			while (a_bits) {
				const auto  bit = static_cast<bits_type>(bits_type{ 1 } << std::countr_zero(a_bits));
				const auto* entry = detail::find_entry<enum_type>(bit);
				if (!entry) {
					return {};
				}

				if (!result.empty()) {
					result += '|';
				}
				result += a_full ? entry->name : entry->shortName;

				a_bits = static_cast<bits_type>(a_bits & (a_bits - 1));
			}

			return result;
		}

		underlying_type _impl{ 0 };
	};

	// deduction
	template <class... Args>
	enumeration(Args...) -> enumeration<
		std::common_type_t<Args...>,
		std::underlying_type_t<
			std::common_type_t<Args...>>>;
}  // namespace DKUtil::model
