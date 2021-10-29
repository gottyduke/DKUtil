#pragma once

#include <compare>
#include <initializer_list>
#include <optional>

#include "Logger.hpp"
#include "Utility.hpp"

#include "external/ini_config.hpp"
#include "external/json.hpp"
#include "external/toml.h"


// compile-time evaluation
#define GET_PROXY(SRC)			DKUtil::Config::Proxy::Proxy<DKUtil::Config::Proxy::EvaluateConfig([]() { return SRC; })>(SRC)
// runtime dynamic
#define GET_PATH(SRC)			DKUtil::Config::Proxy::GetPath(SRC)
#define GET_RUNTIME_PROXY(SRC)	DKUtil::Config::Proxy::Proxy<DKUtil::Config::Proxy::FileType::kDynamic>(SRC)


namespace DKUtil::Config
{
	namespace Data
	{
		// base
		template <typename underlying_data_t>
		class IData
		{
		public:
			constexpr IData() noexcept = default;
			constexpr IData(const underlying_data_t& a_data) :
				_data(a_data)
			{}

			constexpr IData(underlying_data_t&& a_data) :
				_data(std::move(a_data))
			{}

			IData(const IData&) noexcept = default;
			IData(IData&&) noexcept = default;


			[[maybe_unused]] constexpr IData& operator=(const underlying_data_t& a_rhs) noexcept
			{
				_data = a_rhs;
				return *this;
			}

			[[maybe_unused]] constexpr IData& operator=(underlying_data_t&& a_rhs) noexcept
			{
				_data = std::move(a_rhs);
				return *this;
			}

			[[maybe_unused]] IData& operator=(const IData&) noexcept = default;
			[[maybe_unused]] IData& operator=(IData&&) noexcept = default;
			[[nodiscard]] auto& operator*() noexcept { return _data; }


			[[nodiscard]] auto& Get() noexcept { return _data; }

		protected:
			constexpr ~IData() = default;

			underlying_data_t _data;
		};


		// automatic, collection-enabled
		template <typename underlying_data_t>
			requires std::integral<underlying_data_t> || std::floating_point<underlying_data_t>
		class AData final : public IData<underlying_data_t>
		{
		public:
			using base = IData<underlying_data_t>;
			using data = std::unique_ptr<std::vector<underlying_data_t>>;
			using IData<underlying_data_t>::IData;

			constexpr explicit AData(std::initializer_list<underlying_data_t> a_dataList)
			{
				if (a_dataList.size() == 1) {
					base::_data = a_dataList[0];
					return;
				}

				Free();
				_dataA = std::make_unique<std::vector<underlying_data_t>>(a_dataList);
				MapFront();

				_isArray = true;
			}

			AData(const AData& a_copy) noexcept
			{
				_isArray = a_copy._isArray;
				if (_isArray) {
					_dataA = std::make_unique<std::vector<underlying_data_t>>();
					_dataA->insert(_dataA->begin(), a_copy._dataA->begin(), a_copy._dataA->end());
					MapFront();
				} else {
					base::_data = a_copy.base::_data;
				}
			}

			AData(AData&& a_move) noexcept
			{
				_isArray = a_move._isArray;
				if (_isArray) {
					_dataA = std::move(a_move._dataA);
					a_move.Free();
					MapFront();
				} else {
					base::_data = std::move(a_move.base::_data);
				}
			}

			constexpr ~AData() noexcept = default;


			constexpr auto& operator=(std::initializer_list<underlying_data_t> a_rhs) noexcept
			{
				_isArray = (a_rhs.size() > 1);
				if (_isArray) {
					Free();
					_dataA = std::make_unique<std::vector<underlying_data_t>>(a_rhs);
					MapFront();
				} else {
					base::_data = a_rhs[0];
				}

				return _dataA;
			}

			auto& operator=(const AData<underlying_data_t>& a_rhs) noexcept
			{
				_isArray = a_rhs._isArray;
				if (_isArray) {
					Free();
					_dataA->insert(_dataA->begin(), a_rhs._dataA->begin(), a_rhs._dataA->end());
					MapFront();
				} else {
					base::_data = a_rhs.base::_data;
				}

				return *this;
			}

			auto& operator=(AData<underlying_data_t>&& a_rhs) noexcept
			{
				_isArray = a_rhs._isArray;
				if (_isArray) {
					_dataA = std::move(a_rhs);
					a_rhs.Free();
					MapFront();
				} else {
					base::_data = a_rhs.base::_data;
				}

				return *this;
			}

			[[nodiscard]] auto	operator[](const std::size_t a_index) noexcept -> std::optional<underlying_data_t&> { return a_index < _dataA->size() ? _dataA->at(a_index) : std::nullopt; }
			[[nodiscard]] auto	operator<=>(const std::three_way_comparable<underlying_data_t> auto a_rhs) const noexcept { return base::_data <=> a_rhs; }
			[[nodiscard]] auto	operator<=>(const AData<underlying_data_t>& a_rhs) const noexcept { return base::_data <=> a_rhs.base::_data; }
			[[nodiscard]] auto	operator==(const AData<underlying_data_t>& a_rhs) const noexcept { return base::_data == a_rhs.base::_data; }
			[[nodiscard]] auto	operator!=(const AData<underlying_data_t>& a_rhs) const noexcept { return base::_data != a_rhs.base::_data; }


			[[nodiscard]] auto	IsArray() const noexcept { return _isArray; }
			[[nodiscard]] auto&	GetArray() noexcept { return _isArray ? _dataA : base::_data; }
			inline void			Free() noexcept { _dataA.reset(); }
			inline void			MapFront() noexcept { if (_isArray) { base::_data = _dataA->front(); } }

		private:
			bool _isArray = false;
			data _dataA = nullptr;
		};


		template <typename value_type>
		concept char_pointer_t = std::is_convertible_v<const value_type&, const char*>;

		template <typename value_type>
		concept char_view_t = std::is_convertible_v<const value_type&, std::basic_string_view<char>>;

		template <typename value_type>
		concept is_view_not_pointer_t = char_view_t<value_type> && (!char_pointer_t<value_type>);

		// string-ish data type
		template <typename underlying_data_t>
			requires char_view_t<underlying_data_t> || char_pointer_t<underlying_data_t>
		class String final : public IData<underlying_data_t>
		{
		public:			
			using base = IData<underlying_data_t>;

			String(const is_view_not_pointer_t auto& a_data)
			{
				if (const std::basic_string_view<char> data = a_data; !data.empty()) { base::_data = data.data(); }
			}

			constexpr ~String() noexcept = default;

			auto operator=(char_pointer_t auto a_rhs) noexcept
			{
				if (a_rhs) { base::_data = a_rhs; }

				return base::_data;
			}

			auto operator=(const is_view_not_pointer_t auto& a_rhs) noexcept
			{
				if (const std::basic_string_view<char> data = a_rhs; !data.empty()) { base::_data = data.data(); }

				return base::_data;
			}

			String& operator=(const String&) noexcept = default;

			String& operator=(String&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					base::_data = a_rhs._data;
					a_rhs._data = nullptr;
				}

				return *this;
			}
		};

		extern template class IData<bool>;
		extern template class AData<std::int64_t>;
		extern template class AData<double>;
		extern template class String<const char*>;
		extern template class String<std::basic_string_view<char>>;


		class DataManager
		{
		public:
			struct BaseData
			{
				int a;

				BaseData* data;
			};


			static auto& GetSingleton() noexcept
			{
				static DataManager dm;
				return dm;
			}


		private:
		};
	} // namespace Data

	using Integer = Data::AData<std::int64_t>;
	using Double = Data::AData<double>;
	using Boolean = Data::IData<bool>;
	using String = Data::String<const char*>;


	namespace Parser
	{
		class Parser
		{
		public:
			explicit Parser(std::string&& a_file) :
				_file(a_file) { }

			Parser() = delete;
			Parser(const Parser&) = delete;
			Parser(Parser&&) = delete;

			virtual void LoadFile() noexcept = 0;
			virtual void LoadData(const char*) noexcept = 0;
			virtual void WriteFile() noexcept = 0;
			virtual void WriteData(const char*) noexcept = 0;

		protected:
			~Parser() = default;
			const std::string  _file;
			Data::DataManager& _dm = Data::DataManager::GetSingleton();
		};


		class Ini final : public Parser
		{
		public:
			using Parser::Parser;

			void LoadFile() noexcept override { }
			void LoadData(const char* a_data) noexcept override { if (!a_data) { } }
			void WriteFile() noexcept override { }
			void WriteData(const char* a_data) noexcept override { }
		private:
		};


		class Json final : public Parser
		{
		public:
			using Parser::Parser;

			void LoadFile() noexcept override { }
			void LoadData(const char* a_data) noexcept override { }
			void WriteFile() noexcept override { }
			void WriteData(const char* a_data) noexcept override { }
		};


		class Toml final : public Parser
		{
		public:
			using Parser::Parser;

			void LoadFile() noexcept override { }
			void LoadData(const char* a_data) noexcept override { }
			void WriteFile() noexcept override { }
			void WriteData(const char* a_data) noexcept override { }
		};
	} // namespace Parser

	namespace Proxy
	{
		enum class FileType
		{
			kDynamic = 0,
			kIni,
			kJson,
			kToml,
			kError
		};


		// compile-time file type evaluation from literal
		template <typename input_string_t>
		constexpr FileType EvaluateConfig(const input_string_t a_file)
		{
			constexpr std::basic_string_view<char> file = a_file();
			static_assert(!file.empty(), "Empty filename passed to proxy");
			static_assert(file.size() >= 4, "Config filename length too short");

			constexpr auto extension = file.substr(file.size() - 4);
			if constexpr (extension[0] == '.' && (extension[1] == 'i' || extension[1] == 'I') && (extension[2] == 'n' ||
				extension[2] == 'N') && (extension[3] == 'i' || extension[3] == 'I')) { return FileType::kIni; }
			if constexpr ((extension[0] == 'j' || extension[0] == 'J') && (extension[1] == 's' || extension[1] == 'S')
				&& (extension[2] == 'o' || extension[2] == 'O') && (extension[3] == 'n' || extension[3] == 'N')) {
				return FileType::kJson;
			}
			if constexpr ((extension[0] == 't' || extension[0] == 'T') && (extension[1] == 'o' || extension[1] == 'O')
				&& (extension[2] == 'm' || extension[2] == 'M') && (extension[3] == 'l' || extension[3] == 'L')) {
				return FileType::kToml;
			}

			return FileType::kError;
		}


		static_assert(EvaluateConfig([]() { return "ConfigType.ini"sv; }) == FileType::kIni);
		static_assert(EvaluateConfig([]() { return "ConfigType.json"sv; }) == FileType::kJson);
		static_assert(EvaluateConfig([]() { return "ConfigType.toml"sv; }) == FileType::kToml);
		static_assert(EvaluateConfig([]() { return "ConfigType.wtf"sv; }) == FileType::kError);


		inline auto GetPath(const std::string_view a_file) noexcept
		{
			return std::move(fmt::format("{}{}", "Data\\SKSE\\Plugins\\", a_file));
		}


		template <FileType proxy_t>
		class Proxy
		{
			using Parser = std::conditional_t<
				proxy_t == FileType::kIni, Parser::Ini, std::conditional_t<
					proxy_t == FileType::kJson, Parser::Json, std::conditional_t<
						proxy_t == FileType::kToml, Parser::Toml, std::conditional_t<
							proxy_t == FileType::kDynamic, Parser::Parser*, std::nullopt_t>>>>;
			static_assert(proxy_t != FileType::kError);

		public:
			explicit Proxy(const std::string_view a_file) :
				_id(Utility::FNV_1A_32(a_file.data())), _loaded(false), _file(a_file.data()), _type(proxy_t),
				_parser(GetPath(a_file))
			{
				if (_type == FileType::kError) {
					ERROR("Proxy: Define config file type failed"sv);
					throw std::runtime_error("DKUtil failed to load config file"s);
				}

				DEBUG("Proxy: #{} Generated -> {}", _id, _file);
			}


			Proxy() = delete;
			Proxy(Proxy&&) = delete;
			~Proxy() = default;

			Proxy& operator=(const Proxy&) = delete;
			Proxy& operator=(Proxy&&) = delete;


			void Load(const bool a_override = false, const char* a_data = nullptr)
			{
				DEBUG("Proxy: Loading -> {}", _file);

				if (a_override && a_data) {
					DEBUG("");
					_parser.LoadData(a_data);
				} else {
					_parser.LoadFile();
				}

				_loaded = true;
			}


		private:
			std::uint32_t     _id;
			bool              _loaded;
			const std::string _file;
			FileType          _type;
			Parser            _parser;
		};
	} // namespace Proxy
} // namespace DKUtil::Config
