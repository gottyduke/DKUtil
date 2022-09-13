#pragma once

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdlib>
#include <limits>
#include <ranges>
#include <string>


// pass PROJECT_NAME to compiler definitions
#ifndef PROJECT_NAME
#	define PROJECT_NAME Plugin::NAME.data()
#endif

#ifdef DKU_DEBUG
#	define DKU_DEBUG(...) DEBUG(__VA_ARGS__)
#else
#	define DKU_DEBUG(...) void(0)
#endif


namespace DKUtil
{
	namespace function
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
	}  // namespace function


	namespace model
	{
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

		// ryan is really a wiz, I shamelessly copy
		// https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/master/include/SKSE/Impl/PCH.h
		template <class EF>                                    //
		requires(std::invocable<std::remove_reference_t<EF>>)  //
			class scope_exit
		{
		public:
			// 1)
			template <class Fn>
			explicit scope_exit(Fn&& a_fn)  //
				noexcept(std::is_nothrow_constructible_v<EF, Fn> ||
						 std::is_nothrow_constructible_v<EF, Fn&>)  //
				requires(!std::is_same_v<std::remove_cvref_t<Fn>, scope_exit> &&
						 std::is_constructible_v<EF, Fn>)
			{
				static_assert(std::invocable<Fn>);

				if constexpr (!std::is_lvalue_reference_v<Fn> &&
							  std::is_nothrow_constructible_v<EF, Fn>) {
					_fn.emplace(std::forward<Fn>(a_fn));
				} else {
					_fn.emplace(a_fn);
				}
			}

			// 2)
			scope_exit(scope_exit&& a_rhs)  //
				noexcept(std::is_nothrow_move_constructible_v<EF> ||
						 std::is_nothrow_copy_constructible_v<EF>)  //
				requires(std::is_nothrow_move_constructible_v<EF> ||
						 std::is_copy_constructible_v<EF>)
			{
				static_assert(!(std::is_nothrow_move_constructible_v<EF> && !std::is_move_constructible_v<EF>));
				static_assert(!(!std::is_nothrow_move_constructible_v<EF> && !std::is_copy_constructible_v<EF>));

				if (a_rhs.active()) {
					if constexpr (std::is_nothrow_move_constructible_v<EF>) {
						_fn.emplace(std::forward<EF>(*a_rhs._fn));
					} else {
						_fn.emplace(a_rhs._fn);
					}
					a_rhs.release();
				}
			}

			// 3)
			scope_exit(const scope_exit&) = delete;

			~scope_exit() noexcept
			{
				if (_fn.has_value()) {
					(*_fn)();
				}
			}

			void release() noexcept { _fn.reset(); }

		private:
			[[nodiscard]] bool active() const noexcept { return _fn.has_value(); }

			std::optional<std::remove_reference_t<EF>> _fn;
		};

		template <class EF>
		scope_exit(EF) -> scope_exit<EF>;

		// taken from CommonLib, added ranged based view iterator for additive/flag enums
		template <
			class Enum,
			class Underlying = std::underlying_type_t<Enum>>
		class enumeration
		{
		public:
			using enum_type = Enum;
			using underlying_type = Underlying;

			static_assert(std::is_scoped_enum_v<enum_type>, "enum_type must be a scoped enum");
			static_assert(std::is_integral_v<underlying_type>, "underlying_type must be an integral");

			constexpr enumeration() noexcept = default;

			constexpr enumeration(const enumeration&) noexcept = default;

			constexpr enumeration(enumeration&&) noexcept = default;

			template <class U2>  // NOLINTNEXTLINE(google-explicit-constructor)
			constexpr enumeration(enumeration<Enum, U2> a_rhs) noexcept :
				_impl(static_cast<underlying_type>(a_rhs.get()))
			{}

			template <class... Args>
			constexpr enumeration(Args... a_values) noexcept  //
				requires(std::same_as<Args, enum_type>&&...) :
				_impl((static_cast<underlying_type>(a_values) | ...))
			{}

			~enumeration() noexcept = default;

			constexpr enumeration& operator=(const enumeration&) noexcept = default;
			constexpr enumeration& operator=(enumeration&&) noexcept = default;

			template <class U2>
			constexpr enumeration& operator=(enumeration<Enum, U2> a_rhs) noexcept
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
			[[nodiscard]] constexpr auto operator<=>(const enumeration<Enum, underlying_type>& a_rhs) const noexcept { return _impl <=> a_rhs._impl; }
			[[nodiscard]] constexpr auto operator==(const enumeration<Enum, underlying_type>& a_rhs) const noexcept { return _impl == a_rhs._impl; }
			[[nodiscard]] constexpr auto operator!=(const enumeration<Enum, underlying_type>& a_rhs) const noexcept { return _impl != a_rhs._impl; }
			[[nodiscard]] constexpr enum_type get() const noexcept { return static_cast<enum_type>(_impl); }
			[[nodiscard]] constexpr underlying_type underlying() const noexcept { return _impl; }
			// continuous enum, linear transitive
			[[nodiscard]] constexpr auto step_range(Enum a_begin, Enum a_end, std::int32_t a_step = 1) const noexcept { return _step_range(a_begin, a_end, a_step); }
			// bitflag enum, base 2 shift, m->l
			[[nodiscard]] constexpr auto flag_range(Enum a_begin, Enum a_end) const noexcept { return _flag_range(a_begin, a_end); }
			// nth bit flag
			[[nodiscard]] constexpr auto index_of(Enum a_val) const noexcept { return std::bit_width<underlying_type>(std::to_underlying(a_val)) - 1; }


			template <class... Args>
			constexpr enumeration& set(Args... a_args) noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				_impl |= (static_cast<underlying_type>(a_args) | ...);
				return *this;
			}

			template <class... Args>
			constexpr enumeration& reset(Args... a_args) noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				_impl &= ~(static_cast<underlying_type>(a_args) | ...);
				return *this;
			}

			template <class... Args>
			[[nodiscard]] constexpr bool any(Args... a_args) const noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				return (_impl & (static_cast<underlying_type>(a_args) | ...)) != static_cast<underlying_type>(0);
			}

			template <class... Args>
			[[nodiscard]] constexpr bool all(Args... a_args) const noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				return (_impl & (static_cast<underlying_type>(a_args) | ...)) == (static_cast<underlying_type>(a_args) | ...);
			}

			template <class... Args>
			[[nodiscard]] constexpr bool none(Args... a_args) const noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				return (_impl & (static_cast<underlying_type>(a_args) | ...)) == static_cast<underlying_type>(0);
			}


		private:
			static constexpr auto _step_range = [](auto a_front, auto a_back, auto a_step) {
				return std::views::iota(
						   std::to_underlying(a_front),
						   (std::to_underlying(a_back) - std::to_underlying(a_front) + a_step - 1) / a_step) |
				       std::views::transform([=](auto e) { return std::bit_cast<Enum>(static_cast<underlying_type>(e * a_step + std::to_underlying(a_front))); });
			};

			static constexpr auto _flag_range = [](auto a_front, auto a_back) {
				return std::views::iota(
						   std::bit_width<underlying_type>(std::to_underlying(a_front)) - 1,
						   std::bit_width<underlying_type>(std::to_underlying(a_back))) |
				       std::views::transform([=](auto bit) { return std::bit_cast<Enum>(static_cast<underlying_type>(1 << bit)); });
			};


			underlying_type _impl{ 0 };
		};

		template <class... Args>
		enumeration(Args...) -> enumeration<
			std::common_type_t<Args...>,
			std::underlying_type_t<
				std::common_type_t<Args...>>>;
	}  // namespace model

	namespace numbers
	{
		class RNG
		{
		public:
			static RNG* GetSingleton()
			{
				static RNG singleton;
				return &singleton;
			}

			template <class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type>
			T Generate(T a_min, T a_max)
			{
				if constexpr (std::is_integral_v<T>) {
					std::uniform_int_distribution<T> distr(a_min, a_max);
					return distr(twister);
				} else {
					std::uniform_real_distribution<T> distr(a_min, a_max);
					return distr(twister);
				}
			}

		private:
			RNG() :
				twister(std::random_device{}())
			{}
			RNG(RNG const&) = delete;
			RNG(RNG&&) = delete;
			~RNG() = default;

			RNG& operator=(RNG const&) = delete;
			RNG& operator=(RNG&&) = delete;

			std::mt19937 twister;
		};

		// https://stackoverflow.com/questions/48896142
		template <typename Str>
		inline constexpr std::uint32_t FNV_1A_32(const Str& a_str, const std::uint32_t a_prime = 2166136261u) noexcept
		{
			return (a_str[0] == '\0') ? a_prime : FNV_1A_32(&a_str[1], (a_prime ^ static_cast<std::uint32_t>(a_str[0])) * 16777619u);
		}

		template <typename Str>
		inline constexpr std::uint64_t FNV_1A_64(const Str& a_str, const std::uint64_t a_prime = 14695981039346656037ull) noexcept
		{
			return (a_str[0] == '\0') ? a_prime : FNV_1A_64(&a_str[1], (a_prime ^ static_cast<std::uint64_t>(a_str[0])) * 1099511628211ull);
		}

		// taken from CommonLibSSE-Util
		constexpr float EPSILON = std::numeric_limits<float>::epsilon();

		inline bool approximately_equal(float a, float b)
		{
			return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
		}

		inline bool essentially_equal(float a, float b)
		{
			return fabs(a - b) <= ((fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
		}

		inline bool definitely_greater_than(float a, float b)
		{
			return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
		}

		inline bool definitely_less_than(float a, float b)
		{
			return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
		}
	}  // namespace numbers

	namespace string
	{
		inline std::wstring to_wstring(const std::string& a_str)
		{
			auto resultSize = MultiByteToWideChar(CP_UTF8, 0, &a_str[0], static_cast<int>(a_str.size()), nullptr, 0);
			std::wstring wStr(resultSize, 0);
			MultiByteToWideChar(CP_UTF8, 0, &a_str[0], static_cast<int>(a_str.size()), &wStr[0], resultSize);
			return std::move(wStr);
		}

		inline std::string to_string(const std::wstring& a_wstr)
		{
			BOOL used{};
			auto resultSize = WideCharToMultiByte(CP_UTF8, 0, &a_wstr[0], static_cast<int>(a_wstr.size()), nullptr, 0, nullptr, &used);
			std::string str(resultSize, 0);
			WideCharToMultiByte(CP_UTF8, 0, &a_wstr[0], static_cast<int>(a_wstr.size()), &str[0], resultSize, nullptr, &used);
			return std::move(str);
		}

		// https://stackoverflow.com/questions/28708497/constexpr-to-concatenate-two-or-more-char-strings
		template <size_t S>
		using size = std::integral_constant<size_t, S>;

		template <class T, size_t N>
		consteval size<N> length(T const (&)[N])
		{
			return {};
		}
		template <class T, size_t N>
		consteval size<N> length(std::array<T, N> const&)
		{
			return {};
		}

		template <class T>
		using length_t = decltype(length(std::declval<T>()));
		consteval size_t sum_string_sizes() { return 0; }
		template <class... Ts>
		consteval size_t sum_string_sizes(size_t i, Ts... ts)
		{
			return (i ? i - 1 : 0) + sum_sizes(ts...);
		}

		template <unsigned N1, unsigned N2, class... Us>
		consteval auto concat(const char (&a1)[N1], const char (&a2)[N2], const Us&... xs)
			-> std::array<char, sum_string_sizes(N1, N2, length_t<Us>::value...) + 1>
		{
			return concat(a1, concat(a2, xs...));
		}

		// Taken from CommonLibSSE-Util
		namespace detail
		{
			// trim from left
			inline std::string& ltrim(std::string& a_str)
			{
				a_str.erase(0, a_str.find_first_not_of(" \t\n\r\f\v"));
				return a_str;
			}

			// trim from right
			inline std::string& rtrim(std::string& a_str)
			{
				a_str.erase(a_str.find_last_not_of(" \t\n\r\f\v") + 1);
				return a_str;
			}
		}

		inline std::string& trim(std::string& a_str)
		{
			return detail::ltrim(detail::rtrim(a_str));
		}

		inline std::string trim_copy(std::string a_str)
		{
			return trim(a_str);
		}

		inline bool is_empty(const char* a_char)
		{
			return a_char == nullptr || a_char[0] == '\0';
		}

		inline bool is_only_digit(std::string_view a_str)
		{
			return std::ranges::all_of(a_str, [](char c) {
				return std::isdigit(static_cast<unsigned char>(c));
			});
		}

		inline bool is_only_hex(std::string_view a_str)
		{
			if (a_str.compare(0, 2, "0x") == 0 || a_str.compare(0, 2, "0X") == 0) {
				return a_str.size() > 2 && std::all_of(a_str.begin() + 2, a_str.end(), [](char c) {
					return std::isxdigit(static_cast<unsigned char>(c));
				});
			}
			return false;
		}

		inline bool is_only_letter(std::string_view a_str)
		{
			return std::ranges::all_of(a_str, [](char c) {
				return std::isalpha(static_cast<unsigned char>(c));
			});
		}

		inline bool is_only_space(std::string_view a_str)
		{
			return std::ranges::all_of(a_str, [](char c) {
				return std::isspace(static_cast<unsigned char>(c));
			});
		}

		inline bool icontains(std::string_view a_str1, std::string_view a_str2)
		{
			if (a_str2.length() > a_str1.length())
				return false;

			auto found = std::ranges::search(a_str1, a_str2,
				[](char ch1, char ch2) {
					return std::toupper(static_cast<unsigned char>(ch1)) == std::toupper(static_cast<unsigned char>(ch2));
				});

			return !found.empty();
		}

		inline bool iequals(std::string_view a_str1, std::string_view a_str2)
		{
			return std::ranges::equal(a_str1, a_str2,
				[](char ch1, char ch2) {
					return std::toupper(static_cast<unsigned char>(ch1)) == std::toupper(static_cast<unsigned char>(ch2));
				});
		}

		inline std::string join(const std::vector<std::string>& a_vec, const char* a_delimiter)
		{
			std::ostringstream os;
			auto begin = a_vec.begin();
			auto end = a_vec.end();

			if (begin != end) {
				std::copy(begin, std::prev(end), std::ostream_iterator<std::string>(os, a_delimiter));
				os << *std::prev(end);
			}

			return os.str();
		}

		template <class T>
		T lexical_cast(const std::string& a_str, bool a_hex = false)
		{
			if constexpr (std::is_floating_point_v<T>) {
				return static_cast<T>(std::stof(a_str));
			} else if constexpr (std::is_signed_v<T>) {
				return static_cast<T>(std::stoi(a_str));
			} else if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
				return static_cast<T>(std::stoull(a_str));
			} else if (a_hex) {
				return static_cast<T>(std::stoul(a_str, nullptr, 16));
			} else {
				return static_cast<T>(std::stoul(a_str));
			}
		}

		inline std::string remove_non_alphanumeric(std::string& a_str)
		{
			std::ranges::replace_if(
				a_str, [](char c) { return !std::isalnum(static_cast<unsigned char>(c)); }, ' ');
			return trim_copy(a_str);
		}

		inline std::string remove_non_numeric(std::string& a_str)
		{
			std::ranges::replace_if(
				a_str, [](char c) { return !std::isdigit(static_cast<unsigned char>(c)); }, ' ');
			return trim_copy(a_str);
		}

		inline void replace_all(std::string& a_str, std::string_view a_search, std::string_view a_replace)
		{
			if (a_search.empty()) {
				return;
			}

			size_t pos = 0;
			while ((pos = a_str.find(a_search, pos)) != std::string::npos) {
				a_str.replace(pos, a_search.length(), a_replace);
				pos += a_replace.length();
			}
		}

		inline void replace_first_instance(std::string& a_str, std::string_view a_search, std::string_view a_replace)
		{
			if (a_search.empty()) {
				return;
			}

			if (auto pos = a_str.find(a_search); pos != std::string::npos) {
				a_str.replace(pos, a_search.length(), a_replace);
			}
		}

		inline void replace_last_instance(std::string& a_str, std::string_view a_search, std::string_view a_replace)
		{
			if (a_search.empty()) {
				return;
			}

			if (auto pos = a_str.rfind(a_search); pos != std::string::npos) {
				a_str.replace(pos, a_search.length(), a_replace);
			}
		}

		inline std::vector<std::string> split(const std::string& a_str, const std::string& a_deliminator)
		{
			std::vector<std::string> list;
			std::string strCopy = a_str;
			size_t pos = 0;
			std::string token;
			while ((pos = strCopy.find(a_deliminator)) != std::string::npos) {
				token = strCopy.substr(0, pos);
				list.push_back(token);
				strCopy.erase(0, pos + a_deliminator.length());
			}
			list.push_back(strCopy);
			return list;
		}
	}  // namespace string

	template <typename First, typename... T>
	[[nodiscard]] bool is_in(First&& first, T&&... t)
	{
		return ((first == t) || ...);
	}

	// owning pointer
	template <
		class T,
		class = std::enable_if_t<
			std::is_pointer_v<T>>>
	using owner = T;

	// non-owning pointer
	template <
		class T,
		class = std::enable_if_t<
			std::is_pointer_v<T>>>
	using observer = T;

	// non-null pointer
	template <
		class T,
		class = std::enable_if_t<
			std::is_pointer_v<T>>>
	using not_null = T;
}  // namespace DKUtil
