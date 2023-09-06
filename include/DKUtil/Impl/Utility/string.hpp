#pragma once


namespace DKUtil::string
{
	// CLib
	template <class CharT, std::size_t N>
	struct static_string
	{
		using char_type = CharT;
		using pointer = char_type*;
		using const_pointer = const char_type*;
		using reference = char_type&;
		using const_reference = const char_type&;
		using size_type = std::size_t;

		static constexpr auto npos = static_cast<std::size_t>(-1);

		consteval static_string(const_pointer a_string) noexcept
		{
			for (size_type i = 0; i < N; ++i) {
				c[i] = a_string[i];
			}
		}

		[[nodiscard]] consteval const_reference operator[](size_type a_pos) const noexcept
		{
			assert(a_pos < N);
			return c[a_pos];
		}

		[[nodiscard]] consteval char_type value_at(size_type a_pos) const noexcept
		{
			assert(a_pos < N);
			return c[a_pos];
		}

		[[nodiscard]] consteval const_reference back() const noexcept { return (*this)[size() - 1]; }
		[[nodiscard]] consteval const_pointer data() const noexcept { return c; }
		[[nodiscard]] consteval bool empty() const noexcept { return this->size() == 0; }
		[[nodiscard]] consteval const_reference front() const noexcept { return (*this)[0]; }
		[[nodiscard]] consteval size_type length() const noexcept { return N; }
		[[nodiscard]] consteval size_type size() const noexcept { return length(); }

		template <std::size_t POS = 0, std::size_t COUNT = npos>
		[[nodiscard]] consteval auto substr() const noexcept
		{
			return static_string < CharT, COUNT != npos ? COUNT : N - POS > (this->data() + POS);
		}

		char_type c[N] = {};
	};

	template <class CharT, std::size_t N>
	static_string(const CharT (&)[N]) -> static_string<CharT, N - 1>;

	// trim from left
	[[nodiscard]] inline constexpr std::string& ltrim(std::string& a_str) noexcept
	{
		a_str.erase(0, a_str.find_first_not_of(" \t\n\r\f\v"));
		return a_str;
	}

	// trim from right
	[[nodiscard]] inline constexpr std::string& rtrim(std::string& a_str) noexcept
	{
		a_str.erase(a_str.find_last_not_of(" \t\n\r\f\v") + 1);
		return a_str;
	}

	[[nodiscard]] inline constexpr std::string& trim(std::string& a_str) noexcept
	{
		return ltrim(rtrim(a_str));
	}

	[[nodiscard]] inline constexpr std::string trim_copy(std::string a_str) noexcept
	{
		return trim(a_str);
	}

	[[nodiscard]] inline constexpr bool is_empty(const char* a_char) noexcept
	{
		return a_char == nullptr || a_char[0] == '\0';
	}

	[[nodiscard]] inline constexpr bool is_only_digit(std::string_view a_str) noexcept
	{
		return std::ranges::all_of(a_str, [](char c) {
			return std::isdigit(static_cast<unsigned char>(c));
		});
	}

	[[nodiscard]] inline constexpr bool is_only_hex(std::string_view a_str) noexcept
	{
		if (a_str.compare(0, 2, "0x") == 0 || a_str.compare(0, 2, "0X") == 0) {
			return a_str.size() > 2 && std::all_of(a_str.begin() + 2, a_str.end(), [](char c) {
				return std::isxdigit(static_cast<unsigned char>(c));
			});
		}
		return false;
	}

	[[nodiscard]] inline constexpr bool is_only_letter(std::string_view a_str) noexcept
	{
		return std::ranges::all_of(a_str, [](char c) {
			return std::isalpha(static_cast<unsigned char>(c));
		});
	}

	[[nodiscard]] inline constexpr bool is_only_space(std::string_view a_str) noexcept
	{
		return std::ranges::all_of(a_str, [](char c) {
			return std::isspace(static_cast<unsigned char>(c));
		});
	}

	static inline constexpr auto icmp = [](char ch1, char ch2) -> bool {
		return std::toupper(static_cast<unsigned char>(ch1)) == std::toupper(static_cast<unsigned char>(ch2));
	};

	[[nodiscard]] inline constexpr bool icontains(std::string_view a_str, std::string_view a_pattern) noexcept
	{
		return std::ranges::contains_subrange(a_str, a_pattern, icmp);
	}

	[[nodiscard]] inline constexpr bool iequals(std::string_view a_str1, std::string_view a_str2) noexcept
	{
		return std::ranges::equal(a_str1, a_str2, icmp);
	}

	[[nodiscard]] inline constexpr bool istarts_with(std::string_view a_full, std::string_view a_pattern) noexcept
	{
		return std::ranges::starts_with(a_full, a_pattern, icmp);
	}

	[[nodiscard]] inline constexpr bool iends_with(std::string_view a_full, std::string_view a_pattern) noexcept
	{
		return std::ranges::starts_with(a_full | std::views::reverse, a_pattern | std::views::reverse, icmp);
	}

	template <class T>
	[[nodiscard]] inline T lexical_cast(const std::string& a_str, bool a_hex = false) noexcept
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

	[[nodiscard]] inline std::string remove_non_alphanumeric(std::string a_str) noexcept
	{
		std::ranges::replace_if(
			a_str, [](char c) { return !std::isalnum(static_cast<unsigned char>(c)); }, ' ');
		return trim_copy(a_str);
	}

	[[nodiscard]] inline std::string remove_non_numeric(std::string a_str) noexcept
	{
		std::ranges::replace_if(
			a_str, [](char c) { return !std::isdigit(static_cast<unsigned char>(c)); }, ' ');
		return trim_copy(a_str);
	}

	// positive = forward, starting from 1 (first)
	// negative = reverse, starting from -1 (last)
	// 0 = replace all
	// if n is out of bounds of all matches, the boundary is used instead
	// replacement can be omitted for removing occurrence
	[[nodiscard]] inline constexpr std::string replace_nth_occurrence(std::string_view a_str, const int a_nth, const std::string_view a_pattern, const std::string_view a_replace = {}) noexcept
	{
		if (a_nth == 0) {  // all
			// workaround for P2328#1
			return a_str | std::views::split(a_pattern) | std::views::join_with(a_replace) | std::ranges::to<std::string>();
		} else {  // selective
			auto sv = a_str | std::views::split(a_pattern);
			const int splits = std::ranges::distance(sv) - 1;
			auto nth = std::clamp(a_nth, -splits, splits);
			nth = nth < 0 ? splits + nth + 1 : nth;

			auto res = sv | std::views::take(nth) | std::views::join_with(a_pattern) | std::ranges::to<std::string>();
			res.append(a_replace);
			res.append_range(sv | std::views::drop(nth) | std::views::join_with(a_pattern));

			return res;
		}
	}

	[[nodiscard]] inline constexpr std::vector<std::string> split(const std::string_view a_str, const std::convertible_to<std::string_view> auto&... a_deliminators) noexcept
	{
		std::vector<std::string> list;
		std::string_view chunk{};
		std::size_t p{ 0 }, n{ 0 };

		const auto slide_split = [&](std::string_view delim) {
			if (auto pos = chunk.rfind(delim); pos != std::string_view::npos) {
				p = n;
				if (pos) {
					list.emplace_back(chunk.substr(0, pos));
					return;
				}
			}
		};

		while (n < a_str.length()) {
			chunk = a_str.substr(p, n - p);
			(slide_split(a_deliminators), ...);
			++n;
		}

		if (n == a_str.length()) {
			list.emplace_back(a_str.substr(p, n - p + 1));
		}

		return list;
	}

	[[nodiscard]] inline constexpr std::string join(const std::vector<std::string>& a_vec, const std::string_view a_delimiter = {}) noexcept
	{
		// workaround for P2328#1
		return a_vec | std::views::join_with(a_delimiter) | std::ranges::to<std::string>();
	}

	inline constexpr std::string& toupper(std::string& a_str) noexcept
	{
		return a_str = a_str | std::views::transform([](auto& c) { return c = std::toupper(c); }) | std::ranges::to<std::string>();
	}

	inline constexpr std::string& tolower(std::string& a_str) noexcept
	{
		return a_str = a_str | std::views::transform([](auto& c) { return c = std::tolower(c); }) | std::ranges::to<std::string>();
	}
}  // namespace DKUtil::string