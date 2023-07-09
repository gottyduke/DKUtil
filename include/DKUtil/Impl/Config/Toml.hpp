#pragma once


#include "data.hpp"

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
			auto result = a_data ? toml::parse(a_data) : toml::parse_file(_filepath);
			if (!result) {
				ERROR("DKU_C: Parser#{}: Parsing failed!\nFile: {}\nDesc: {}", _id, *result.error().source().path.get(), result.error().description());
			}

			_toml = std::move(result).table();
			for (auto& [section, table] : _toml) {
				if (!table.is_table()) {
					INFO("DKU_C: WARNING\nParser#{}: Sectionless configuration present and skipped.\nPossible inappropriate formatting at [{}]", _id, section.str());
					continue;
				} else {
					for (auto& [key, data] : _manager) {
						auto raw = table.as_table()->find(key.data());
						if (table.as_table()->begin() != table.as_table()->end() &&
							raw == table.as_table()->end()) {
							continue;
						}

						switch (data->get_type()) {
						case DataType::kBoolean:
							{
								if (raw->second.as_boolean()) {
									data->As<bool>()->set_data(raw->second.as_boolean()->get());
								}
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

										data->As<double>()->set_data(array);
										break;
									}
								} else {
									if (raw->second.as_floating_point()) {
										input = raw->second.as_floating_point()->get();
									}
								}

								data->As<double>()->set_data(input);
								break;
							}
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

										data->As<std::int64_t>()->set_data(array);
										break;
									}
								} else {
									if (raw->second.as_integer()) {
										input = raw->second.as_integer()->get();
									}
								}

								data->As<std::int64_t>()->set_data(input);
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

										data->As<std::basic_string<char>>()->set_data(array);
										break;
									}
								} else {
									if (raw->second.as_string()) {
										input = raw->second.as_string()->get();
									}
								}

								data->As<std::basic_string<char>>()->set_data(input);
								break;
							}
						case DataType::kError:
						default:
							continue;
						}
					}
				}
			}

			std::stringstream os{};
			os << _toml;
			_content = std::move(os.str());

			DEBUG("DKU_C: Parser#{}: Parsing finished", _id);
		}

		void Write(const std::string_view a_filePath) noexcept override
		{
			auto filePath = a_filePath.empty() ? _filepath.c_str() : a_filePath.data();
			std::basic_ofstream<char> file{ filePath };
			if (!file.is_open() || !file) {
				ERROR("DKU_C: Parser#{}: Writing file failed! -> {}\nofstream cannot be opened", _id, filePath);
			}

			file << _toml;
			file.close();

			DEBUG("DKU_C: Parser#{}: Writing finished", _id);
		}

	private:
		toml::table _toml;
	};
}  // namespace detail
