#pragma once


namespace DKUtil::model
{
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

	template <class EF>
	scope_exit(EF) -> scope_exit<EF>;

};  // namespace DKUtil::model