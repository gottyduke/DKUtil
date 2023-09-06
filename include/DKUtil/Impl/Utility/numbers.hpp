#pragma once


namespace DKUtil::numbers
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
	[[nodiscard]] inline constexpr std::uint32_t FNV_1A_32(const Str& a_str, const std::uint32_t a_prime = 2166136261u) noexcept
	{
		return (a_str[0] == '\0') ? a_prime : FNV_1A_32(&a_str[1], (a_prime ^ static_cast<std::uint32_t>(a_str[0])) * 16777619u);
	}

	template <typename Str>
	[[nodiscard]] inline constexpr std::uint64_t FNV_1A_64(const Str& a_str, const std::uint64_t a_prime = 14695981039346656037ull) noexcept
	{
		return (a_str[0] == '\0') ? a_prime : FNV_1A_64(&a_str[1], (a_prime ^ static_cast<std::uint64_t>(a_str[0])) * 1099511628211ull);
	}

	// taken from CommonLibSSE-Util
	static constexpr float EPSILON = std::numeric_limits<float>::epsilon();

	[[nodiscard]] inline bool approximately_equal(float a, float b)
	{
		return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
	}

	[[nodiscard]] inline bool essentially_equal(float a, float b)
	{
		return fabs(a - b) <= ((fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
	}

	[[nodiscard]] inline bool definitely_greater_than(float a, float b)
	{
		return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
	}

	[[nodiscard]] inline bool definitely_less_than(float a, float b)
	{
		return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
	}

	[[nodiscard]] inline constexpr std::size_t roundup(std::size_t a_number, std::size_t a_multiple) noexcept
	{
		if (a_multiple == 0) {
			return 0;
		}

		const auto remainder = a_number % a_multiple;
		return remainder == 0 ?
		           a_number :
		           a_number + a_multiple - remainder;
	}

	[[nodiscard]] inline constexpr std::size_t rounddown(std::size_t a_number, std::size_t a_multiple) noexcept
	{
		if (a_multiple == 0) {
			return 0;
		}

		const auto remainder = a_number % a_multiple;
		return remainder == 0 ?
		           a_number :
		           a_number - remainder;
	}

	[[nodiscard]] inline consteval std::size_t kilobyte(std::size_t a_quantity) noexcept
	{
		return static_cast<std::size_t>(1 << 10) * a_quantity;
	}

	[[nodiscard]] inline consteval std::size_t megabyte(std::size_t a_quantity) noexcept
	{
		return static_cast<std::size_t>(1 << 20) * a_quantity;
	}

	[[nodiscard]] inline consteval std::size_t gigabyte(std::size_t a_quantity) noexcept
	{
		return static_cast<std::size_t>(1 << 30) * a_quantity;
	}
}  // namespace DKUtil::numbers