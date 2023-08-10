#pragma once


#include "string.hpp"


namespace DKUtil::model
{
	// 64 by default, nums(64) or bits(1<<64)
#ifndef DKU_MAX_REFLECTION_ENUM
#	define DKU_MAX_REFLECTION_ENUM 64
#endif
#define __cache_suffix ">(void) noexcept const"
#define DKU_BUILD_STEP_CACHE(N)                             \
	info.erase(info.length() - sizeof(__cache_suffix) + 1); \
	if (!string::icontains(info, "(enum")) {                \
		_reflection.nameTbl.try_emplace(N - 1, info);       \
	}                                                       \
	info = cache<static_cast<enum_type>(N)>();
#define DKU_BUILD_FLAG_CACHE(B)                             \
	info.erase(info.length() - sizeof(__cache_suffix) + 1); \
	if (!string::icontains(info, "(enum")) {                \
		_reflection.nameTbl.try_emplace(B - 1, info);       \
	}                                                       \
	info = cache<static_cast<enum_type>(1 << B)>();

	// clang-format off
#define DKU_FOR_EACH_ENUM(B) { { B(1) B(2) B(3) B(4) B(5) B(6) B(7) B(8) B(9) B(10) B(11) B(12) B(13) B(14) B(15) B(16) B(17) }	if constexpr (DKU_MAX_REFLECTION_ENUM > 16) { B(18) B(19) B(20)	B(21) B(22) B(23) B(24) B(25) B(26) B(27) B(28) B(29) B(30) B(31) B(32) B(33) }	if constexpr (DKU_MAX_REFLECTION_ENUM > 32) { B(34) B(35) B(36) B(37) B(38)	B(39) B(40) B(41) B(42) B(43) B(44) B(45) B(46) B(47) B(48) B(49) B(50) B(51) B(52)	B(53) B(54) B(55) B(56) B(57) B(58) B(59) B(60) B(61) B(62) B(63) B(64) B(65) }	if constexpr (DKU_MAX_REFLECTION_ENUM > 64) { B(66) B(67) B(68) B(69) B(70) B(71) B(72) B(73) B(74) B(75) B(76) B(77) B(78) B(79) B(80) B(81) B(82) B(83) B(84) B(85) B(86) B(87) B(88) B(89) B(90) B(91) B(92) B(93) B(94) B(95) B(96)	B(97) B(98) B(99) B(100) B(101) B(102) B(103) B(104) B(105) B(106) B(107) B(108) B(109) B(110) B(111) B(112) B(113) B(114) B(115) B(116) B(117) B(118) B(119) B(120) B(121) B(122) B(123) B(124) B(125) B(126) B(127) B(128) } }
	// clang-format on


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

		struct Reflection
		{
			bool isCached;
			bool isFlag;
			std::string type;
			std::map<underlying_type, const std::string> nameTbl;
		};


		constexpr enumeration() noexcept = default;
		constexpr enumeration(const enumeration& a_rhs) noexcept :
			_impl(a_rhs._impl), _reflection(a_rhs._reflection)
		{}
		constexpr enumeration(enumeration&&) noexcept = default;

		template <class U2>  // NOLINTNEXTLINE(google-explicit-constructor)
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
		}

		constexpr enumeration& operator=(enum_type a_value) noexcept
		{
			_impl = static_cast<underlying_type>(a_value);
			return *this;
		}

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return _impl != static_cast<underlying_type>(0); }

		[[nodiscard]] constexpr enum_type operator*() const noexcept { return get(); }
		[[nodiscard]] constexpr auto operator<=>(const std::three_way_comparable<underlying_type> auto& a_rhs) const noexcept { return _impl <=> a_rhs; }
		[[nodiscard]] constexpr auto operator<=>(const enumeration<enum_type, underlying_type>& a_rhs) const noexcept { return _impl <=> a_rhs._impl; }
		[[nodiscard]] constexpr auto operator==(const enumeration<enum_type, underlying_type>& a_rhs) const noexcept { return _impl == a_rhs._impl; }
		[[nodiscard]] constexpr auto operator!=(const enumeration<enum_type, underlying_type>& a_rhs) const noexcept { return _impl != a_rhs._impl; }
		[[nodiscard]] constexpr enum_type get() const noexcept { return static_cast<enum_type>(_impl); }
		[[nodiscard]] constexpr underlying_type underlying() const noexcept { return _impl; }
		[[nodiscard]] constexpr bool is_flag() const noexcept { return _reflection.isFlag; }

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

		// static reflection
		[[nodiscard]] constexpr std::string to_string(enum_type a_enum, bool a_full = false) noexcept
		{
			build_cache();
			return to_string(is_flag() ? (std::bit_width<underlying_type>(std::to_underlying(a_enum)) - 1) : std::to_underlying(a_enum), a_full);
		}

		// underlying adaptor
		[[nodiscard]] constexpr std::string to_string(const std::convertible_to<underlying_type> auto a_value, bool a_full = false) noexcept
		{
			build_cache();

			underlying_type idx = a_value >= DKU_MAX_REFLECTION_ENUM ? DKU_MAX_REFLECTION_ENUM : a_value;

			if (!_reflection.nameTbl.contains(idx)) {
				return {};
			}

			if (!a_full) {
				return _reflection.nameTbl[idx].substr(_reflection.type.length() + 2, _reflection.nameTbl[idx].length() - _reflection.type.length());
			}

			return _reflection.nameTbl[idx];
		}

		// string cast
		[[nodiscard]] std::optional<enum_type> from_string(std::string_view a_enumString, bool a_caseSensitive = false) noexcept
		{
			if (a_enumString.empty()) {
				return {};
			}

			build_cache();

			for (auto& [idx, name] : _reflection.nameTbl) {
				std::string shortName = name.substr(_reflection.type.length() + 2, name.length() - _reflection.type.length());
				if ((a_caseSensitive && shortName.compare(a_enumString) == 0) ||
					(!a_caseSensitive && string::iequals(shortName, a_enumString))) {
					return is_flag() ? static_cast<enum_type>(1 << idx) : static_cast<enum_type>(idx);
				} else {
					continue;
				}
			}

			return {};
		}

		// enum name
		[[nodiscard]] constexpr std::string_view enum_name() noexcept
		{
#ifndef DKU_SLIM
			build_cache();
			return _reflection.type;
#else
			return type_name();
#endif
		}

		// type name
		[[nodiscard]] constexpr std::string_view type_name() const noexcept { return typeid(underlying_type).name(); }

		// contiguous enum, linear transitive
		[[nodiscard]] constexpr auto value_range(enum_type a_begin, enum_type a_end, std::int32_t a_step = 1) noexcept
		{
#ifndef DKU_SLIM
			build_cache();
			if (is_flag()) {
				ERROR("value_range iterator called but enum is flag_type!\nEnum name: {}\nEnum type: {}", enum_name(), type_name());
			}
#endif
			if (a_begin == a_end || !a_step) {
				ERROR("Range iterator mandates different elements AND operable step value to construct a valid range!\nStep value provided: {}", a_step);
			}

			if ((a_end < a_begin) && a_step > 0) {
				a_step *= -1;
			}

			return std::views::iota(
					   std::to_underlying(a_begin),
					   (std::to_underlying(a_end) - std::to_underlying(a_begin) + a_step) / a_step) |
			       std::views::transform([=](auto e) { return std::bit_cast<enum_type>(static_cast<underlying_type>(e * a_step + std::to_underlying(a_begin))); });
		}

		// bitflag enum, base 2 shift, l->m
		[[nodiscard]] constexpr auto flag_range(enum_type a_begin, enum_type a_end) noexcept
		{
#ifndef DKU_SLIM
			build_cache();
			if (!is_flag()) {
				ERROR("flag_range iterator called but enum is value_type!\nEnum name: {}\nEnum type: {}", enum_name(), type_name());
			}
#endif
			if (a_begin == a_end) {
				ERROR("Range iterator mandates different elements AND operable step value to construct a valid range!");
			}

			return std::views::iota(
					   std::bit_width<underlying_type>(std::to_underlying(a_begin)),
					   std::bit_width<underlying_type>(std::to_underlying(a_end))) |
			       std::views::transform([](auto i) { auto bit = (!i ? 0 : (1 << i));
						   return std::bit_cast<enum_type>(static_cast<underlying_type>(bit)); });
		}

		[[nodiscard]] constexpr underlying_type index_of(enum_type a_enum) const noexcept { return std::bit_width<underlying_type>(std::to_underlying(a_enum)); }

	private:
		template <enum_type E>
		constexpr const char* cache() const noexcept
		{
			static std::regex r("::cache<(.*?)>");
			std::cmatch m;
			std::regex_search(__FUNCSIG__, m, r);

			return m[1].first;
		}

		constexpr void build_cache() noexcept
		{
			static_assert(DKU_MAX_REFLECTION_ENUM > 0);

			if (_reflection.isCached) {
				return;
			}

			std::string info = cache<static_cast<enum_type>(0)>();
			DKU_FOR_EACH_ENUM(DKU_BUILD_STEP_CACHE);

			if (_reflection.nameTbl.size() <= 1) {
				_reflection.isFlag = false;
				return;
			}

			// test for flag trait, samples affected by DKU_MAX_REFLECTION_ENUM
			auto contiguity = std::unordered_set<underlying_type>();
			for (const auto valid : std::views::keys(_reflection.nameTbl)) {
				if (contiguity.contains(std::bit_width<underlying_type>(valid))) {
					_reflection.isFlag = false;
					break;
				} else {
					contiguity.emplace(std::bit_width<underlying_type>(valid));
				}

				if (_reflection.type.empty()) {
					auto ns = string::split(_reflection.nameTbl[valid], "::");
					_reflection.type = _reflection.nameTbl[valid].substr(0, _reflection.nameTbl[valid].length() - ns[ns.size() - 1].length() - 2);
				}
			}

			if (_reflection.isFlag) {
				_reflection.nameTbl.clear();

				std::string info = cache<static_cast<enum_type>(0)>();
				DKU_FOR_EACH_ENUM(DKU_BUILD_FLAG_CACHE);
			}

			_reflection.isCached = true;
		}

		Reflection _reflection{ false, true };
		underlying_type _impl{ 0 };
	};

	// deduction
	template <class... Args>
	enumeration(Args...) -> enumeration<
		std::common_type_t<Args...>,
		std::underlying_type_t<
			std::common_type_t<Args...>>>;
}  // namespace DKUtil::model


#undef DKU_BUILD_STEP_CACHE
#undef DKU_BUILD_FLAG_CACHE
#undef DKU_FOR_EACH_ENUM
