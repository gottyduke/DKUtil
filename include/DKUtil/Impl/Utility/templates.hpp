#pragma once


namespace DKUtil::model
{
	// clang-format off
	namespace concepts
	{
		template <typename T, typename = void>
		struct dku_subtype
		{
			using type = T;
		};

		template <typename T>
		struct dku_bindable_t : std::false_type
		{};

		template <typename T, typename U>
		struct dku_bindable_t<std::pair<T, U>> : std::true_type
		{};

		template <typename... T>
		struct dku_bindable_t<std::tuple<T...>> : std::true_type
		{};

		template <typename T>
		concept dku_bindable = dku_bindable_t<T>::value;

		template <typename T>
		struct dku_queue_t : std::false_type
		{};

		template <typename T>
		struct dku_queue_t<std::queue<T>> : std::true_type
		{};

		template <typename T>
		concept dku_queue = dku_queue_t<T>::value;

		template <typename T>
		struct dku_subtype<T, std::enable_if_t<std::ranges::range<T>>>
		{
			using type = typename std::ranges::range_value_t<T>;
		};

		template <typename T>
		struct dku_subtype<T, std::enable_if_t<dku_queue<T>>>
		{
			using type = typename std::queue<T>::value_type;
		};

		template <typename T>
		using dku_value_type = typename dku_subtype<T>::type;

		template <typename T>
		concept dku_ranges =
			std::ranges::range<T> &&
			!std::is_same_v<T, std::string> &&
			!std::is_same_v<T, std::string_view>;

		template <typename T>
		concept dku_string =
			std::is_same_v<T, std::string> ||
			std::is_same_v<T, std::string_view> ||
			std::is_convertible_v<T, std::string_view>;

		template <typename T>
		concept dku_trivial =
			(std::is_trivial_v<T> || std::is_pod_v<T>) &&
			!dku_ranges<T>;

		template <typename T>
		concept dku_trivial_ranges =
			(std::is_trivial_v<T> || std::is_pod_v<T>) &&
			dku_ranges<T>;

		template <typename T>
		concept dku_aggregate =
			std::is_aggregate_v<T> &&
			!dku_ranges<T>;

		template <typename T>
		concept dku_numeric =
			dku_trivial<T> && 
			!dku_string<T>;
	} // namespace concepts
	// clang-format on

	struct any
	{
		template <typename T>
		constexpr operator T();
	};

	template <typename T>
		requires(concepts::dku_aggregate<T>)
	inline consteval auto number_of_bindables() noexcept
	{
		using type = std::remove_cvref_t<T>;

		// clang-format off
		if constexpr (requires { type{ any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 9; } else if constexpr (requires { type{ any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 8; } else if constexpr (requires { type{ any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 7; } else if constexpr (requires { type{ any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 6; } else if constexpr (requires { type{ any{}, any{}, any{}, any{}, any{}}; }) { return 5; } else if constexpr (requires { type{any{}, any{}, any{}, any{}}; }) { return 4; } else if constexpr (requires { type{any{}, any{}, any{}}; }) { return 3; } else if constexpr (requires { type{any{}, any{}}; }) { return 2; } else if constexpr (requires { type{any{}}; }) { return 1;
			// clang-format on
		} else {
			return 0;
		}
	}

	template <typename T>
		requires(concepts::dku_bindable<T>)
	inline consteval auto number_of_bindables() noexcept
	{
		using type = std::remove_cvref_t<T>;

		return std::tuple_size_v<type>;
	}

	template <typename T>
	inline consteval auto number_of_bindables() noexcept
	{
		using type = std::remove_cvref_t<T>;

		if constexpr (requires { std::extent_v<type>; }) {
			return std::extent_v<type>;
		} else {
			return std::size(type{});
		}
	}

	template <typename T>
	inline constexpr auto number_of_bindables_v = number_of_bindables<T>();

	template <typename First, typename... T>
	[[nodiscard]] inline bool is_in(First&& first, T&&... t)
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


	template <class derived_t>
	class Singleton
	{
	public:
		constexpr static derived_t* GetSingleton()
		{
			static derived_t singleton;
			return std::addressof(singleton);
		}

		constexpr Singleton(const Singleton&) = delete;
		constexpr Singleton(Singleton&&) = delete;
		constexpr Singleton& operator=(const Singleton&) = delete;
		constexpr Singleton& operator=(Singleton&&) = delete;

	protected:
		constexpr Singleton() = default;
		constexpr ~Singleton() = default;
	};


	// ryan is really a wiz, I shamelessly copy
	// https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/master/include/SKSE/Impl/PCH.h
	template <class EF>                                        //
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

	// deduction
	template <class EF>
	scope_exit(EF) -> scope_exit<EF>;


#define PARAMS_MACRO_1(macro) macro(1)
#define PARAMS_MACRO_2(macro) PARAMS_MACRO_1(macro), macro(2)
#define PARAMS_MACRO_3(macro) PARAMS_MACRO_2(macro), macro(3)
#define PARAMS_MACRO_4(macro) PARAMS_MACRO_3(macro), macro(4)
#define PARAMS_MACRO_5(macro) PARAMS_MACRO_4(macro), macro(5)
#define PARAMS_MACRO_6(macro) PARAMS_MACRO_5(macro), macro(6)
#define PARAMS_MACRO_7(macro) PARAMS_MACRO_6(macro), macro(7)
#define PARAMS_MACRO_8(macro) PARAMS_MACRO_7(macro), macro(8)
#define PARAMS_MACRO_9(macro) PARAMS_MACRO_8(macro), macro(9)
#define PARGS_MACRO(n) p##n
#define TARGS_MACRO(n) t##n
#define IMPLICIT_PARAM(n) static_cast<std::remove_cvref_t<decltype(t##n)>>(p##n)
	// clang-format on

#define MAKE_TUPLE_PARAM(n, macro)                    \
	if constexpr (number_of_bindables_v<type> == n) { \
		auto&& [macro(PARGS_MACRO)] = object;         \
		return std::make_tuple(macro(PARGS_MACRO));   \
	} else


#define MAKE_STRUCT_PARAM(n, macro)                      \
	if constexpr (number_of_bindables_v<to_type> == n) { \
		auto&& [macro(TARGS_MACRO)] = to_type{};         \
		auto&& [macro(PARGS_MACRO)] = object;            \
		return to_type{ macro(IMPLICIT_PARAM) };         \
	} else


	template <typename T>
	inline constexpr auto tuple_cast(T&& object) noexcept
	{
		using type = std::remove_cvref_t<T>;

		MAKE_TUPLE_PARAM(9, PARAMS_MACRO_9)
		MAKE_TUPLE_PARAM(8, PARAMS_MACRO_8)
		MAKE_TUPLE_PARAM(7, PARAMS_MACRO_7)
		MAKE_TUPLE_PARAM(6, PARAMS_MACRO_6)
		MAKE_TUPLE_PARAM(5, PARAMS_MACRO_5)
		MAKE_TUPLE_PARAM(4, PARAMS_MACRO_4)
		MAKE_TUPLE_PARAM(3, PARAMS_MACRO_3)
		MAKE_TUPLE_PARAM(2, PARAMS_MACRO_2)
		MAKE_TUPLE_PARAM(1, PARAMS_MACRO_1)
		{
			return std::make_tuple();
		}
	}

	template <typename T, typename F>
	inline constexpr T struct_cast(F&& object) noexcept
	{
		using to_type = std::remove_cvref_t<T>;
		using from_type = std::remove_cvref_t<F>;

		static_assert(number_of_bindables_v<to_type> == number_of_bindables_v<from_type>,
			"number of bindables of <F> and <T> must equal.");

		MAKE_STRUCT_PARAM(9, PARAMS_MACRO_9)
		MAKE_STRUCT_PARAM(8, PARAMS_MACRO_8)
		MAKE_STRUCT_PARAM(7, PARAMS_MACRO_7)
		MAKE_STRUCT_PARAM(6, PARAMS_MACRO_6)
		MAKE_STRUCT_PARAM(5, PARAMS_MACRO_5)
		MAKE_STRUCT_PARAM(4, PARAMS_MACRO_4)
		MAKE_STRUCT_PARAM(3, PARAMS_MACRO_3)
		MAKE_STRUCT_PARAM(2, PARAMS_MACRO_2)
		MAKE_STRUCT_PARAM(1, PARAMS_MACRO_1)
		{
			return to_type{};
		}
	}

	template <typename T>
		requires(concepts::dku_ranges<T>)
	inline constexpr auto vector_cast(T&& object) noexcept
	{
		using type = concepts::dku_value_type<std::remove_cvref_t<T>>;
		return object | std::ranges::to<std::vector<type>>();
	}

	template <typename T, typename F>
		requires(concepts::dku_ranges<T> && concepts::dku_ranges<F>)
	inline constexpr auto range_cast(F&& object) noexcept
	{
		using to_type = std::remove_cvref_t<T>;
		if constexpr (requires { object | std::ranges::to<to_type>(); }) {
			return object | std::ranges::to<to_type>();
		} else {
			return to_type{ std::begin(object), std::end(object) };
		}
	}
};  // namespace DKUtil::model

#undef MAKE_STRUCT_CAST_ERROR
