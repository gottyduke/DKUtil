#pragma once


#include "Data.hpp"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"


namespace DKUtil::Config::detail
{
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
					INFO("DKU_C: WARNING\nParser#{}: Sectionless configuration present and skipped.\nPossible inappropriate formatting at [{}]", _id, section.str());
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
								if (raw->second.is_array() && raw->second.as_array()) {
									if (raw->second.as_array()->size() == 1 &&
										raw->second.as_array()->front().as_integer()) {
										input = raw->second.as_array()->front().as_integer()->get();
									} else if (raw->second.as_array()->size() > 1) {
										std::vector<std::int64_t> array;
										for (auto& node : *raw->second.as_array()) {
											if (node.as_integer()) {
												array.push_back(node.as_integer()->get());
											}
										}

										auto data = dynamic_cast<AData<std::int64_t>*>(dataPtr);
										data->set_data(array);

										break;
									}
								} else {
									if (raw->second.as_integer()) {
										input = raw->second.as_integer()->get();
									}
								}
								_manager.SetByKey(dataKey, input);
								break;
							}
						case DataType::kDouble:
							{
								double input;
								if (raw->second.is_array() && raw->second.as_array()) {
									if (raw->second.as_array()->size() == 1 &&
										raw->second.as_array()->front().as_floating_point()) {
										input = raw->second.as_array()->front().as_floating_point()->get();
									} else if (raw->second.as_array()->size()) {
										std::vector<double> array;
										for (auto& node : *raw->second.as_array()) {
											if (node.as_floating_point()) {
												array.push_back(node.as_floating_point()->get());
											}
										}

										auto data = dynamic_cast<AData<double>*>(dataPtr);
										data->set_data(array);

										break;
									}
								} else {
									if (raw->second.as_floating_point()) {
										input = raw->second.as_floating_point()->get();
									}
								}
								_manager.SetByKey(dataKey, input);
								break;
							}
						case DataType::kBoolean:
							{
								if (raw->second.as_boolean()) {
									_manager.SetByKey(dataKey, raw->second.as_boolean()->get());
								}
								break;
							}
						case DataType::kString:
							{
								std::basic_string<char> input;
								if (raw->second.is_array() && raw->second.as_array()) {
									if (raw->second.as_array()->size() == 1 &&
										raw->second.as_array()->front().as_string()) {
										input = raw->second.as_array()->front().as_string()->get();
									} else if (raw->second.as_array()->size()) {
										std::vector<std::basic_string<char>> array;
										for (auto& node : *raw->second.as_array()) {
											if (node.as_string()) {
												array.push_back(node.as_string()->get());
											}
										}

										auto data = dynamic_cast<AData<std::basic_string<char>>*>(dataPtr);
										data->set_data(array);

										break;
									}
								} else {
									if (raw->second.as_string()) {
										input = raw->second.as_string()->get();
									}
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

		void Write(const std::string_view a_filePath) noexcept override
		{
			auto filePath = a_filePath.empty() ? _filePath.c_str() : a_filePath.data();
			std::basic_ofstream<char> file{ filePath };
			if (!file.is_open()) {
				ERROR("DKU_C: Parser#{}: Writing file failed! -> {}\nofstream cannot be opened", _id, filePath);
			}

			file << _toml;
			file.close();
		}

		const void* Data() noexcept override
		{
			std::stringstream os{};
			os << _toml;
			_out = std::move(os.str());

			return _out.data();
		}

	private:
		toml::table _toml;
	};
}  // namespace detail
