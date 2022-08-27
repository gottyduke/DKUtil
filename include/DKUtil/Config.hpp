#pragma once


/*
 * 1.1.0
 * NG-update;
 * Added clamp range;
 * Removed Proxy isLoaded check; 
 *
 * 1.0.2
 * F4SE integration;
 *
 * 1.0.1
 * Fixed toml empty array crash;
 * Removed explicit identifier on copy constructor;
 * 
 * 
 * 1.0.0
 * Proxy, Bind and Load APIs for ini, json and toml type config files;
 * 
 */


#define DKU_C_VERSION_MAJOR     1
#define DKU_C_VERSION_MINOR     1
#define DKU_C_VERSION_REVISION  0


#include <algorithm>
#include <compare>
#include <concepts>
#include <filesystem>
#include <initializer_list>
#include <unordered_set>
#include <optional>
#include <string>

#include "Logger.hpp"

#ifdef DKU_C_DEBUG
#define DKU_DEBUG
#define DEBUG(...)	INFO(__VA_ARGS__)
#endif

#include "Utility.hpp"

#include "nlohmann/json.hpp"
#include "SimpleIni.h"
#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"


#pragma warning( push )
#pragma warning( disable : 4244 )

#ifndef LOG_ENTRY

#if defined( F4SEAPI )
#define LOG_ENTRY "Data\\F4SE\\Plugins\\"
#elif defined ( SKSEAPI )
#define LOG_ENTRY "Data\\SKSE\\Plugins\\"
#else
#error "Neither CommonLib nor custom LOG_ENTRY defined"
#endif

#endif


#define EVAL_HELPER(SRC)	DKUtil::Config::EvaluateConfig([]() { return SRC; })
// compile-time evaluation
#define COMPILE_PROXY(SRC)	DKUtil::Config::Proxy<EVAL_HELPER(SRC)>(SRC)
// runtime dynamic
#define RUNTIME_PROXY(SRC)	DKUtil::Config::Proxy<DKUtil::Config::FileType::kDynamic>(SRC)


namespace DKUtil
{
	constexpr auto DKU_C_VERSION = DKU_C_VERSION_MAJOR * 10000 + DKU_C_VERSION_MINOR * 100 + DKU_C_VERSION_REVISION;
} // namespace DKUtil


namespace DKUtil::Config
{
	inline auto GetPath(const std::string_view a_file) noexcept
	{
		std::filesystem::path pluginDir(LOG_ENTRY);
		std::filesystem::path file(a_file.data());
		return std::move((pluginDir / file).string());
	}


	namespace detail
	{
		template <typename data_t>
		concept dku_c_numeric = (
			std::convertible_to<data_t, std::int64_t> ||
			std::convertible_to<data_t, double>) &&
			!std::convertible_to<data_t, std::basic_string<char>>;


		template <typename data_t>
		concept dku_c_trivial_t = (
			dku_c_numeric<data_t> ||
			std::convertible_to<data_t, bool>) &&
			!std::convertible_to<data_t, std::basic_string<char>>;


		enum class DataType
		{
			kInteger,
			kDouble,
			kBoolean,
			kString,

			kError
		};


		class IData
		{
		public:
			virtual ~IData() {}
		};


		// automatic data with collection enabled
		template <typename underlying_data_t>
		class AData : public IData
		{
			using collection = std::vector<underlying_data_t>;
			using IData::IData;

		public:
			constexpr AData() noexcept = delete;
			explicit constexpr AData(const std::string& a_key, const std::string& a_section = "") :
				_key(std::move(a_key)), _section(std::move(a_section))
			{}


			constexpr AData(const AData&) noexcept = delete;
			constexpr AData(AData&&) noexcept = delete;
			constexpr ~AData() = default;

			// return the front if out of bounds
			[[nodiscard]] auto& operator[](const std::size_t a_index) noexcept
				requires (!std::is_same_v<underlying_data_t, bool>)
			{ 
				if (_isCollection && a_index < _collection->size()) {
					return _collection->at(a_index);
				} else {
					return _data;
				}
			}
			[[nodiscard]] auto& operator*() noexcept { return _data; }
			[[nodiscard]] auto	operator<=>(const std::three_way_comparable<underlying_data_t> auto& a_rhs) const noexcept { return _data <=> a_rhs; }
			[[nodiscard]] auto	operator<=>(const AData<underlying_data_t>& a_rhs) const noexcept { return _data <=> a_rhs._data; }
			[[nodiscard]] auto	operator==(const AData<underlying_data_t>& a_rhs) const noexcept { return _data == a_rhs._data; }
			[[nodiscard]] auto	operator!=(const AData<underlying_data_t>& a_rhs) const noexcept { return _data != a_rhs._data; }

			[[nodiscard]] constexpr auto get_key() const noexcept { return _key.c_str(); }
			[[nodiscard]] constexpr auto get_section() const noexcept { return _section.c_str(); }
			[[nodiscard]] constexpr auto is_collection() const noexcept { return _isCollection; }

			[[nodiscard]] constexpr const auto& get_data() const noexcept { return _data; }
			[[nodiscard]] constexpr auto& get_collection() noexcept 
			{
				if (_isCollection) {
					return *_collection;
				} else {
					ERROR(".get_collection is called on config value {} while it holds singular data!\n\nCheck .is_collection before accessing collcetion!", _key);
				}
			}
			[[nodiscard]] constexpr auto& get_type() const noexcept { return typeid(underlying_data_t); }

			constexpr void set_data(const std::initializer_list<underlying_data_t>& a_list)
			{
				_collection.reset();

				_isCollection = (a_list.size() > 1);
				if (_isCollection) {
					_collection = std::make_unique<collection>(a_list);
				}

				_data = *a_list.begin();

				clamp();
			}

			constexpr void set_data(const collection& a_collection)
			{
				_collection.reset();

				_isCollection = (a_collection.size() > 1);
				if (_isCollection) {
					_collection = std::make_unique<collection>(std::move(a_collection));
				}

				_data = a_collection.front();

				clamp();
			}

			inline constexpr void set_range(std::pair<double, double> a_range)
			{
				if (dku_c_numeric<underlying_data_t>) {
					_range = a_range;
				}
			}

			constexpr void clamp()
			{
				if (!dku_c_numeric<underlying_data_t> || _range.first > _range.second) {
					return;
				}

				if (_isCollection) {
					std::transform(_collection->begin(), _collection->end(), _collection->begin(), [&](underlying_data_t data) -> underlying_data_t
						{
							return data < _range.first ? _range.first : data > _range.second ? _range.second : data;
						});
					_data = _collection->front();
				} else {
					_data = _data < _range.first ? _range.first : _data > _range.second ? _range.second : _data;
				}
			}


		private:
			const std::string								_key;
			const std::string								_section;
			bool											_isCollection = false;
			underlying_data_t								_data;
			std::pair<underlying_data_t, underlying_data_t>	_range;
			std::unique_ptr<collection>						_collection = nullptr;
		};


		extern template class AData<bool>;
		extern template class AData<std::int64_t>;
		extern template class AData<double>;
		extern template class AData<std::basic_string<char>>;


		class DataManager
		{
		public:
			[[nodiscard]] auto Visit(const std::string_view a_key) noexcept
			{
				if (!_managed.contains(a_key)) {
					return detail::DataType::kError;
				}

				if (auto intData = dynamic_cast<AData<std::int64_t>*>(_managed[a_key]); intData) {
					return DataType::kInteger;
				} else if (auto doubleData = dynamic_cast<AData<double>*>(_managed[a_key]); doubleData) {
					return DataType::kDouble;
				} else if (auto boolData = dynamic_cast<AData<bool>*>(_managed[a_key]); boolData) {
					return DataType::kBoolean;
				} else if (auto strData = dynamic_cast<AData<std::basic_string<char>>*>(_managed[a_key]); strData) {
					return DataType::kString;
				} else {
					return DataType::kError;
				}
			}


			void SetByKey(const std::string_view a_key, const dku_c_trivial_t auto&... a_value) noexcept
			{
				if (!_managed.contains(a_key)) {
					return;
				}

				if (auto intData = dynamic_cast<AData<std::int64_t>*>(_managed[a_key]); intData) {
					intData->set_data({ static_cast<std::int64_t>(a_value)... });
				} else if (auto doubleData = dynamic_cast<AData<double>*>(_managed[a_key]); doubleData) {
					doubleData->set_data({ static_cast<double>(a_value)... });
				} else if (auto boolData = dynamic_cast<AData<bool>*>(_managed[a_key]); boolData) {
					boolData->set_data({ static_cast<bool>(a_value)... });
				}
			}


			// string is such a pain in the 
			void SetByKey(const std::string_view a_key, const std::convertible_to<std::basic_string<char>> auto&... a_value) noexcept
			{
				if (!_managed.contains(a_key)) {
					return;
				}

				if (auto data = dynamic_cast<AData<std::basic_string<char>>*>(_managed[a_key]); data) {
					data->set_data({ static_cast<std::basic_string<char>>(a_value)... });
				}
			}


			void Add(const std::string_view a_key, IData* a_data) noexcept { _managed.try_emplace(a_key, a_data); }
			[[nodiscard]] auto size() const noexcept { return _managed.size(); }
			[[nodiscard]] auto& get_managed() noexcept { return _managed; }

		private:
			std::unordered_map<std::string_view, IData*> _managed; // key, data*
		};


		class IParser
		{
		public:
			explicit IParser(std::string&& a_file, const std::uint32_t a_id, DataManager& a_manager) :
				_file(a_file), _filePath(GetPath(a_file)), _id(a_id), _manager(a_manager)
			{}

			constexpr IParser() = delete;
			constexpr IParser(const IParser&) noexcept = default;
			constexpr IParser(IParser&&) noexcept = default;

			constexpr virtual ~IParser() = default;

			virtual void Parse(const char* = nullptr) noexcept = 0;
			virtual void WriteData(const char*&) noexcept = 0;
			virtual void WriteFile(const std::string_view) noexcept = 0;
		protected:
			const std::string	_file;
			const std::string	_filePath;
			const std::uint32_t	_id;
			DataManager&		_manager;
		};


		class Ini final : public IParser
		{
		public:
			using IParser::IParser;

			void Parse(const char* a_data) noexcept override
			{
				_ini.SetUnicode();
				auto result = a_data ? _ini.LoadData(a_data) : _ini.LoadFile(_filePath.c_str());
				if (result < 0) {
					ERROR("DKU_C: Parser#{}: Loading failed! -> {}\n{}", _id, _filePath.c_str(), err_getmsg());
				}

				CSimpleIniA::TNamesDepend sections;
				_ini.GetAllSections(sections);

				for (auto& section : sections) {
					CSimpleIniA::TNamesDepend keys;
					_ini.GetAllKeys(section.pItem, keys);

					for (auto& key : keys) {
						const char* value = _ini.GetValue(section.pItem, key.pItem);
						if (!value) {
							continue;
						}

						switch (_manager.Visit(key.pItem)) {
						case DataType::kInteger:
						{
							try {
								_manager.SetByKey(key.pItem, std::stoll(value));
							} catch (const std::exception&) {
								err_mismatch(key.pItem, "Integer", value);
							}
							break;
						}
						case DataType::kDouble:
						{
							try {
								_manager.SetByKey(key.pItem, std::stod(value));
							} catch (const std::exception&) {
								err_mismatch(key.pItem, "Double", value);
							}
							break;
						}
						case DataType::kBoolean:
						{
							std::string raw{ value };
							std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char a_char) { return std::tolower(a_char); });
							if (raw == "0" || raw == "false") {
								_manager.SetByKey(key.pItem, false);
							} else if (raw == "1" || raw == "true") {
								_manager.SetByKey(key.pItem, true);
							} else {
								err_mismatch(key.pItem, "Boolean", value);
							}
							break;
						}
						case DataType::kString:
						{
							std::string raw{ value };
							if (raw.front() == '"' && raw.back() == '"') {
								raw.pop_back();
								raw.erase(raw.begin());
							}
							_manager.SetByKey(key.pItem, raw);
							break;
						}
						case DataType::kError:
						default:
							continue;
						}
					}
				}
				DEBUG("DKU_C: Parser#{}: Parsing finished", _id);
			}

			void WriteFile(const std::string_view a_filePath) noexcept override
			{
				auto result = a_filePath.empty() ? _ini.SaveFile(_filePath.c_str()) : _ini.SaveFile(a_filePath.data());
				if (result < 0) {
					ERROR("DKU_C: Parser#{}: Saving file failed!\n{}", _id, err_getmsg());
				}
			}

			void WriteData(const char*& a_data) noexcept override
			{
				auto result = _ini.Save(_out);
				if (result < 0) {
					ERROR("DKU_C: Parser#{}: Saving data failed!\n{}", _id, err_getmsg());
				}

				a_data = _out.c_str();
			}
		private:
			inline auto err_getmsg() noexcept -> const char*
			{
				memset(errmsg, 0, sizeof(errmsg));
				strerror_s(errmsg, errno);
				return errmsg;
			}

			inline void err_mismatch(const char* a_key, const char* a_type, const char* a_value) noexcept
			{
				ERROR("DKU_C: Parser#{}: Value type mismatch!\nFile: {}\nKey: {}, Expected: {}, Value: {}", _id, _filePath.c_str(), a_key, a_type, a_value);
			}


			CSimpleIniA _ini;
			std::string _out;
			char errmsg[36];
		};


		class Json final : public IParser
		{
		public:
			using IParser::IParser;
			using json = nlohmann::json;

			void Parse(const char* a_data) noexcept override
			{
				if (a_data) {
					_json = json::parse(a_data);
				} else {
					std::basic_ifstream<char> file(_filePath);
					if (!file.is_open()) {
						ERROR("DKU_C: Parser#{}: Loading failed! -> {}", _id, _filePath.c_str());
					}

					file >> _json;
					file.close();
				}

				for (auto& data : _manager.get_managed()) {
					auto raw = _json.find(data.first.data());
					if (raw == _json.end()) {
						ERROR("DKU_C: Parser#{}: Retrieving config failed!\nFile: {}\nKey: {}", _id, _filePath.c_str(), data.first);
					}

					switch (_manager.Visit(data.first)) {
					case DataType::kInteger:
					{
						if (raw->type() == json::value_t::array) {
							dynamic_cast<AData<std::int64_t>*>(data.second)->set_data(raw->get<std::vector<std::int64_t>>());
						} else {
							_manager.SetByKey(data.first, raw->get<std::int64_t>());
						}
						break;
					}
					case DataType::kDouble:
					{
						if (raw->type() == json::value_t::array) {
							dynamic_cast<AData<double>*>(data.second)->set_data(raw->get<std::vector<double>>());
						} else {
							_manager.SetByKey(data.first, raw->get<double>());
						}
						break;
					}
					case DataType::kBoolean:
					{
						_manager.SetByKey(data.first, raw->get<bool>());
						break;
					}
					case DataType::kString:
					{
						if (raw->type() == json::value_t::array) {
							dynamic_cast<AData<std::basic_string<char>>*>(data.second)->set_data(raw->get<std::vector<std::basic_string<char>>>());
						} else {
							_manager.SetByKey(data.first, raw->get<std::basic_string<char>>());
						}
						break;
					}
					case DataType::kError:
					default:
						continue;
					}
				}
				DEBUG("DKU_C: Parser#{}: Parsing finished", _id);
			}

			void WriteData(const char*& a_data) noexcept override
			{
				_out = _json.dump();

				if (_out.empty()) {
					ERROR("DKU_C: Parser#{}: Saving data failed!", _id);
				}

				a_data = _out.c_str();
			}

			void WriteFile(const std::string_view a_filePath) noexcept override
			{
				auto filePath = a_filePath.empty() ? _filePath.c_str() : a_filePath.data();
				std::basic_ofstream<char> file(filePath);
				if (!file.is_open()) {
					ERROR("DKU_C: Parser#{}: Saving file failed! -> {}", _id, filePath);
				}

				file << _json;
				file.close();
			}
		private:
			json		_json;
			std::string _out;
		};


		class Toml final : public IParser
		{
		public:
			using IParser::IParser;

			void Parse(const char* a_data) noexcept override
			{
				auto result = a_data ? toml::parse(a_data) : toml::parse_file(_filePath);
				if (!result) {
					ERROR("DKU_C: Parser#{}: Parsing failed!\nFile: {}\nDesc: {}", _id, *result.error().source().path.get(), result.error().description());
				}

				_toml = std::move(result).table();
				for (auto& [section, table] : _toml) {
					if (!table.is_table()) {
						INFO("DKU_C: WARNING\nParser#{}: Sectionless configuration present and skipped\nPossible inappropriate formatting", _id);
						continue;
					} else {
						for (auto& [dataKey, dataPtr] : _manager.get_managed()) {
							auto raw = table.as_table()->find(dataKey.data());
							if (table.as_table()->begin() != table.as_table()->end() &&
								raw == table.as_table()->end()) {
								continue;
							}

							switch (_manager.Visit(dataKey)) {
							case DataType::kInteger:
							{
								std::int64_t input;
								if (raw->second.is_array()) {
									if (raw->second.as_array()->size()) {
										std::vector<std::int64_t> array;
										for (auto& node : *raw->second.as_array()) {
											array.push_back(node.as_integer()->get());
										}

										auto data = dynamic_cast<AData<std::int64_t>*>(dataPtr);
										data->set_data(array);

										break;
									} else {
										input = raw->second.as_array()->front().as_integer()->get();
									}
								} else {
									input = raw->second.as_integer()->get();
								}
								_manager.SetByKey(dataKey, input);
								break;
							}
							case DataType::kDouble:
							{
								double input;
								if (raw->second.is_array()) {
									if (raw->second.as_array()->size()) {
										std::vector<double> array;
										for (auto& node : *raw->second.as_array()) {
											array.push_back(node.as_floating_point()->get());
										}

										auto data = dynamic_cast<AData<double>*>(dataPtr);
										data->set_data(array);

										break;
									} else {
										input = raw->second.as_array()->front().as_floating_point()->get();
									}
								} else {
									input = raw->second.as_floating_point()->get();
								}
								_manager.SetByKey(dataKey, input);
								break;
							}
							case DataType::kBoolean:
							{
								_manager.SetByKey(dataKey, raw->second.as_boolean()->get());
								break;
							}
							case DataType::kString:
							{
								std::basic_string<char> input;
								if (raw->second.is_array()) {
									if (raw->second.as_array()->size()) {
										std::vector<std::basic_string<char>> array;
										for (auto& node : *raw->second.as_array()) {
											array.push_back(node.as_string()->get());
										}

										auto data = dynamic_cast<AData<std::basic_string<char>>*>(dataPtr);
										data->set_data(array);

										break;
									} else {
										input = raw->second.as_array()->front().as_string()->get();
									}
								} else {
									input = raw->second.as_string()->get();
								}
								_manager.SetByKey(dataKey, input);
								break;
							}
							case DataType::kError:
							default:
								continue;
							}
						}
					}
				}
				DEBUG("DKU_C: Parser#{}: Parsing finished", _id);
			}

			void WriteData(const char*& a_data) noexcept override
			{
				_out << _toml;
				a_data = _out.str().c_str();
			}

			void WriteFile(const std::string_view a_filePath) noexcept override
			{
				auto filePath = a_filePath.empty() ? _filePath.c_str() : a_filePath.data();
				std::basic_ofstream<char> file(filePath);
				if (!file.is_open()) {
					ERROR("DKU_C: Parser#{}: Saving file failed! -> {}", _id, filePath);
				}

				file << _toml;
				file.close();
			}

		private:
			toml::table			_toml;
			std::stringstream	_out;
		};
	} // namespace detail


	enum class FileType
	{
		kDynamic = 0,
		kIni,
		kJson,
		kToml,

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
		requires (ConfigFileType != FileType::kError)
	class Proxy
	{
		using parser_t = std::conditional_t<
			ConfigFileType == FileType::kIni, detail::Ini, std::conditional_t<
			ConfigFileType == FileType::kJson, detail::Json, std::conditional_t<
			ConfigFileType == FileType::kToml, detail::Toml, detail::IParser>>>;

	public:
		// compile defined
		constexpr explicit Proxy(const std::string_view a_file) noexcept
			requires (ConfigFileType != FileType::kDynamic) :
			_id(numbers::FNV_1A_32(a_file.data())), _file(a_file.data()),
			_type(ConfigFileType), _parser(std::make_unique<parser_t>(a_file.data(), _id, _manager))
		{
			DEBUG("DKU_C: Proxy#{}: Compile -> {}", _id, _file);
		}

		// runtime defined
		constexpr explicit Proxy(const std::string_view a_file) noexcept
			requires (ConfigFileType == FileType::kDynamic) :
			_id(numbers::FNV_1A_32(a_file.data())), _file(a_file.data())
		{
			// bogus, need fixing
			const auto extension = a_file.substr(a_file.size() - 4);
			if (extension[0] == '.' &&
				(extension[1] == 'i' || extension[1] == 'I') &&
				(extension[2] == 'n' || extension[2] == 'N') &&
				(extension[3] == 'i' || extension[3] == 'I')) {
				_type = FileType::kIni;
				_parser = std::make_unique<detail::Ini>(a_file.data(), _id, _manager);
			} else if (
				(extension[0] == 'j' || extension[0] == 'J') &&
				(extension[1] == 's' || extension[1] == 'S') &&
				(extension[2] == 'o' || extension[2] == 'O') &&
				(extension[3] == 'n' || extension[3] == 'N')) {
				_type = FileType::kJson;
				_parser = std::make_unique<detail::Json>(a_file.data(), _id, _manager);
			} else if (
				(extension[0] == 't' || extension[0] == 'T') &&
				(extension[1] == 'o' || extension[1] == 'O') &&
				(extension[2] == 'm' || extension[2] == 'M') &&
				(extension[3] == 'l' || extension[3] == 'L')) {
				_type = FileType::kToml;
				_parser = std::make_unique<detail::Toml>(a_file.data(), _id, _manager);
			} else {
				ERROR("DKU_C: Proxy#{}: No suitable parser found for file -> {}", _id, a_file);
			}
			DEBUG("DKU_C: Proxy#{}: Runtime -> {}", _id, _file);
		}


		Proxy() = default;
		Proxy(const Proxy&) = delete;
		Proxy(Proxy&&) = default;
		~Proxy() = default;

		Proxy& operator=(const Proxy&) = delete;
		Proxy& operator=(Proxy&&) = default;


		void Load(const char* a_data = nullptr)
		{
			DEBUG("DKU_C: Proxy#{}: Loading -> {}", _id, _file);

			_parser->Parse(a_data);
		}

		template <const detail::dku_c_numeric auto min = 1, const detail::dku_c_numeric auto max = 0, typename data_t>
		constexpr inline void Bind(detail::AData<data_t>& a_data, const std::convertible_to<data_t> auto&... a_value) noexcept
		{
			_manager.Add(a_data.get_key(), std::addressof(a_data));
			a_data.set_range({ min, max });
			a_data.set_data({ static_cast<data_t>(a_value)... });
		}


		constexpr inline auto get_id() const noexcept { return _id; }
		constexpr inline auto get_file() const noexcept { return _file; }
		constexpr inline auto get_type() const noexcept { return _type; }
		constexpr inline auto get_size() const noexcept { return _manager.size(); }
		constexpr inline auto& get_manager() const noexcept { return _manager; }

	private:
		const std::uint32_t			_id;
		const std::string			_file;
		FileType					_type;
		std::unique_ptr<parser_t>	_parser;
		detail::DataManager			_manager;
	};
} // namespace DKUtil::Config


namespace DKUtil::Alias
{
	using Boolean = DKUtil::Config::detail::AData<bool>;
	using Integer = DKUtil::Config::detail::AData<std::int64_t>;
	using Double = DKUtil::Config::detail::AData<double>;
	using String = DKUtil::Config::detail::AData<std::basic_string<char>>;
} // namespace DKUtil::Alias


#pragma warning( pop )