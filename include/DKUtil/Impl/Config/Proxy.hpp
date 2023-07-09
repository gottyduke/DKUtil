#pragma once


#include "shared.hpp"


#define __eval_helper(SRC) DKUtil::Config::EvaluateConfig([]() { return SRC; })
// compile-time evaluation
#define COMPILE_PROXY(SRC) DKUtil::Config::Proxy<__eval_helper(SRC)>(SRC)
// runtime dynamic
#define RUNTIME_PROXY(SRC) DKUtil::Config::Proxy<DKUtil::Config::FileType::kDynamic>(SRC)
// schema configs
#define SCHEMA_PROXY(SRC) DKUtil::Config::Proxy<DKUtil::Config::FileType::kSchema>(SRC)


namespace DKUtil::Config
{
	enum class FileType
	{
		kDynamic = 0,
		kIni,
		kJson,
		kToml,
		kSchema,

		kError
	};


	// compile-time file type evaluation from literal/literal view
	template <typename input_string_t>
	consteval FileType EvaluateConfig(const input_string_t a_file)
	{
		constexpr std::basic_string_view<char> file = a_file();
		static_assert(!file.empty(), "Empty filename passed");
		static_assert(file.size() > 4, "Filename too short");

		// where is case insensitive consteval compare?!
		constexpr auto extension = file.substr(file.size() - 4);
		if constexpr (extension[0] == '.' &&
					  (extension[1] == 'i' || extension[1] == 'I') &&
					  (extension[2] == 'n' || extension[2] == 'N') &&
					  (extension[3] == 'i' || extension[3] == 'I')) {
			return FileType::kIni;
		}
		if constexpr ((extension[0] == 'j' || extension[0] == 'J') &&
					  (extension[1] == 's' || extension[1] == 'S') &&
					  (extension[2] == 'o' || extension[2] == 'O') &&
					  (extension[3] == 'n' || extension[3] == 'N')) {
			return FileType::kJson;
		}
		if constexpr ((extension[0] == 't' || extension[0] == 'T') &&
					  (extension[1] == 'o' || extension[1] == 'O') &&
					  (extension[2] == 'm' || extension[2] == 'M') &&
					  (extension[3] == 'l' || extension[3] == 'L')) {
			return FileType::kToml;
		}

		return FileType::kError;
	}


	template <const FileType ConfigFileType>
		requires(ConfigFileType != FileType::kError)
	class Proxy
	{
		// clang-format off
		using parser_t =
			std::conditional_t<ConfigFileType == FileType::kIni, detail::Ini,
			std::conditional_t<ConfigFileType == FileType::kJson, detail::Json,
			std::conditional_t<ConfigFileType == FileType::kToml, detail::Toml,
			std::conditional_t<ConfigFileType == FileType::kSchema, detail::Schema, detail::IParser>>>>;
		// clang-format on
	public:
		// compile defined
		constexpr explicit Proxy(const std::string_view a_file) noexcept
			requires(ConfigFileType != FileType::kDynamic)
			:
			_id(detail::_Count++),
			_filename(a_file),
			_type(ConfigFileType), _parser(std::make_unique<parser_t>(a_file, _id, _manager))
		{
			DEBUG("DKU_C: Proxy#{}: Compile -> {}", _id, _filename);
		}

		// runtime defined
		constexpr explicit Proxy(const std::string_view a_file) noexcept
			requires(ConfigFileType == FileType::kDynamic)
			:
			_id(detail::_Count++),
			_filename(a_file)
		{
			if (dku::string::iends_with(a_file, "ini")) {
				_parser = std::make_unique<detail::Ini>(a_file, _id, _manager);
			} else if (dku::string::iends_with(a_file, "json")) {
				_parser = std::make_unique<detail::Json>(a_file, _id, _manager);
			} else if (dku::string::iends_with(a_file, "toml")) {
				_parser = std::make_unique<detail::Toml>(a_file, _id, _manager);
			} else {
				ERROR("DKU_C: Proxy#{}: No suitable parser found for file -> {}", _id, a_file);
			}

			DEBUG("DKU_C: Proxy#{}: Runtime -> {}", _id, _filename);
		}


		Proxy() = default;
		Proxy(const Proxy&) = delete;
		Proxy(Proxy&&) = default;
		~Proxy() = default;

		Proxy& operator=(const Proxy&) = delete;
		Proxy& operator=(Proxy&&) = default;


		void Load(const char* a_data = nullptr) noexcept
		{
			DEBUG("DKU_C: Proxy#{}: Loading -> {}", _id, _filename);

			_parser->Parse(a_data);
		}

		void Write(const std::string_view a_file = {}) noexcept
		{
			DEBUG("DKU_C: Proxy#{}: Writing -> {}", _id, _filename);

			_parser->Write(a_file);
		}

		template <
			const double min = 1,
			const double max = 0,
			typename data_t>
		constexpr void Bind(detail::AData<data_t>& a_data, const std::convertible_to<data_t> auto&... a_value) noexcept
		{
			static_assert(ConfigFileType != FileType::kSchema, "Schema parser cannot use regular data bindings!");

			_manager.try_emplace(a_data.get_key(), std::addressof(a_data));
			a_data.set_range({ min, max });
			a_data.set_data({ static_cast<data_t>(a_value)... });
		}


		[[nodiscard]] constexpr auto get_id() const noexcept { return _id; }
		[[nodiscard]] constexpr auto get_filename() const noexcept { return _filename; }
		[[nodiscard]] constexpr auto get_type() const noexcept { return _type; }
		[[nodiscard]] constexpr auto& get_parser() const noexcept { return *_parser; }
		[[nodiscard]] constexpr auto* data() noexcept { return _parser->data(); }

	private:
		const std::uint32_t _id;
		const std::string _filename;
		FileType _type;
		std::unique_ptr<parser_t> _parser;
		std::unordered_map<std::string_view, detail::IData*> _manager;  // key, data*
	};
}  // namespace DKUtil::Config
