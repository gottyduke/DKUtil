#pragma once

#include "data.hpp"

#include "SimpleIni.h"

namespace DKUtil::Config::detail
{
	class Schema final : public IParser
	{
	public:
		using IParser::IParser;
		// reversed
		using segments = std::vector<std::string>;

		void Parse(const char* a_data) noexcept override
		{
			_lines.clear();

			if (a_data) {
				_content = a_data;
			} else {
				std::basic_ifstream<char> file{ _filepath };
				if (!file.is_open()) {
					FATAL("DKU_C: Parser#{}: Cannot open schema file!\nFile {}", _id, _filepath);
				}

				_content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
				file.close();
			}

			_lines = string::split(_content, "\n");
			
			__DEBUG("DKU_C: Parser#{}: sSchema loading finished", _id);
		}

		void Write(const std::string_view a_filePath) noexcept override
		{
		}

		void Generate() noexcept override
		{
		}

		static Schema& GetGlobalParser() noexcept
		{
			static manager ph{};
			static auto    parser = std::make_unique<Schema>("GlobalSchemaParser"sv, _Count++, ph);
			return *parser;
		}

	public:
		enum class ExceptionPolicy
		{
			kLog,
			kError,
		};

		// defaults to kLog
		constexpr void set_exception_on_error(ExceptionPolicy a_policy) noexcept
		{
			_policy = a_policy;
		}

		/* @brief Parse a string into user defined struct
		 * @brief e.g. CustomData d = ParseString<CustomData>(line, delim...)
		 * @brief This alias supports aggregate struct of a 19 members maximum
		 * @param a_str : non-empty formatted schema string
		 * @param a_delimiters : one or multiple string delimiters used to make segments
		 * @returns user defined struct
		 */
		template <typename SchemaData>
			requires(model::concepts::dku_aggregate<SchemaData>)
		static SchemaData ParseString(std::string a_str, const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
		{
			auto& parser = GetGlobalParser();

			if (a_str.empty()) {
				return {};
			}
			
			segments segs = string::split(a_str, std::forward<decltype(a_delimiters)>(a_delimiters)...);
			std::ranges::reverse(segs);

			auto tv = model::tuple_cast(SchemaData{});
			if (!parser.parse_next_bindable(segs, tv, std::make_index_sequence<model::number_of_bindables_v<SchemaData>>{})) {
				__WARN("DKU_C: Parser#{}: Errors encountered parsing schema!\nLine {}\n{}", parser._id, a_str, parser._error);
			}

			return model::struct_cast<SchemaData>(tv);
		}

		/* @brief Parse a string to user defined series of segments
		 * @brief e.g. auto [a, b, c, d] = ParseString<int, bool, bool, std::string>(line, delim...)
		 * @brief This alias supports aggregate struct of a 19 members maximum
		 * @param a_str : non-empty formatted schema string
		 * @param a_delimiters : one or multiple string delimiters used to make segments
		 * @returns std::tuple of user defined segments
		 */
		template <typename... SchemaSegment>
			requires(sizeof...(SchemaSegment) > 1)
		static std::tuple<SchemaSegment...> ParseString(std::string a_str, const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
		{
			return ParseString<std::tuple<SchemaSegment...>>(a_str, std::forward<decltype(a_delimiters)>(a_delimiters)...);
		}

		/* @brief Parse specific line from internal buffered content
		 * @brief e.g. CustomData d = ParseLine<CustomData>(ln, delim...).value_or({})
		 * @brief This alias supports aggregate struct of a 19 members maximum
		 * @param a_ln : line number
		 * @param a_delimiters : one or multiple string delimiters used to make segments
		 * @returns std::optional<tuple> of user defined segments, null if no lines left to parse
		 */
		template <typename SchemaData>
			requires(model::concepts::dku_aggregate<SchemaData>)
		auto ParseLine(std::size_t a_ln, const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
			-> std::optional<SchemaData>
		{
			if (a_ln >= _lines.size()) {
				return std::nullopt;
			}

			return ParseString<SchemaData>(_lines[a_ln], std::forward<decltype(a_delimiters)>(a_delimiters)...);
		}

		/* @brief Parse specific line from internal buffered content
		 * @brief e.g. auto [a, b, c, d] = ParseLine<int, bool, bool, std::string>(delim...).value()
		 * @brief This alias supports aggregate struct of a 19 members maximum
		 * @param a_ln : line number
		 * @param a_delimiters : one or multiple string delimiters used to make segments
		 * @returns std::optional<tuple> of user defined segments, null if no line to parse
		 */
		template <typename... SchemaSegment>
			requires(sizeof...(SchemaSegment) > 1)
		auto ParseLine(std::size_t a_ln, const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
			-> std::optional<std::tuple<SchemaSegment...>>
		{
			return ParseLine<std::tuple<SchemaSegment...>>(a_ln, std::forward<decltype(a_delimiters)>(a_delimiters)...);
		}

		/* @brief Parse next line from internal buffered content
		 * @brief e.g. CustomData d = ParseNextLine<CustomData>(delim...).value_or({})
		 * @brief This alias supports aggregate struct of a 19 members maximum
		 * @param a_delimiters : one or multiple string delimiters used to make segments
		 * @returns std::optional<tuple> of user defined segments, null if no lines left to parse
		 */
		template <typename SchemaData>
			requires(model::concepts::dku_aggregate<SchemaData>)
		auto ParseNextLine(const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
			-> std::optional<SchemaData>
		{
			return ParseLine<SchemaData>(_pos++, std::forward<decltype(a_delimiters)>(a_delimiters)...);
		}

		/* @brief Parse next line from internal buffered content
		 * @brief e.g. auto [a, b, c, d] = ParseNextLine<int, bool, bool, std::string>(delim...).value()
		 * @brief This alias supports aggregate struct of a 19 members maximum
		 * @param a_delimiters : one or multiple string delimiters used to make segments
		 * @returns std::optional<tuple> of user defined segments, null if no lines left to parse
		 */
		template <typename... SchemaSegment>
			requires(sizeof...(SchemaSegment) > 1)
		auto ParseNextLine(const std::convertible_to<std::string_view> auto&... a_delimiters) noexcept
			-> std::optional<std::tuple<SchemaSegment...>>
		{
			return ParseNextLine<std::tuple<SchemaSegment...>>(std::forward<decltype(a_delimiters)>(a_delimiters)...);
		}

		[[nodiscard]] constexpr auto get_lines() const noexcept { return _lines; }
		[[nodiscard]] constexpr auto get_pos() const noexcept { return _pos; }
		[[nodiscard]] constexpr auto get_policy() const noexcept { return _policy; }
		[[nodiscard]] constexpr auto get_error() const noexcept { return _error; }

	private:
		template <typename T>
		bool parse_next_bindable(segments& a_segments, T& a_value)
		{
			if (a_segments.empty()) {
				return false;
			}

			auto& segment = a_segments.back();
			auto  on_exit = model::scope_exit([&] { a_segments.pop_back(); });

			try {
				a_value = string::lexical_cast<T>(segment, string::icontains(segment, "0x"));
			} catch (const std::exception& e) {
				auto what = fmt::format("exception at {} : {}\n", segment, e.what());

				switch (_policy) {
				case ExceptionPolicy::kLog:
					{
						_error.append(what);
						break;
					}
				case ExceptionPolicy::kError:
					{
						ERROR("DKU_C: Parser#{}: Errors encountered parsing schema!\nFile {}\nSegment {}\n{}",
							_id, _filename, segment, what);
						break;
					}
				default:
					break;
				}

				return false;
			}

			return true;
		}

		template <typename Bindable, std::size_t... I>
		bool parse_next_bindable(segments& a_segments, Bindable& a_bindable, std::index_sequence<I...>)
		{
			_error.clear();
			auto res = std::vector<bool>(std::initializer_list<bool>{ parse_next_bindable(a_segments, std::get<I>(a_bindable))... });
			return std::ranges::all_of(res, [](bool a_in) { return a_in; });
		}

		segments        _lines{};
		std::size_t     _pos{ 0 };
		ExceptionPolicy _policy{ ExceptionPolicy::kLog };
		std::string     _error{};
	};
}
