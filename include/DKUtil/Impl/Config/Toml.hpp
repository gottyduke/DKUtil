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
					for (auto& [key, value] : _manager) {
						auto* data = value.first;
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
										raw->second.as_array()->front().value<double>()) {
										input = raw->second.as_array()->front().value<double>().value();
									} else if (raw->second.as_array()->size()) {
										std::vector<double> array;
										for (auto& node : *raw->second.as_array()) {
											// default to 0 for numeric types
											array.push_back(node.value_or<double>(0.0));
										}

										data->As<double>()->set_data(array);
										break;
									}
								} else {
									input = raw->second.value_or<double>(0.0);
								}

								data->As<double>()->set_data(input);
								break;
							}
						case DataType::kInteger:
							{
								std::int64_t input;
								if (raw->second.is_array() && raw->second.as_array()) {
									if (raw->second.as_array()->size() == 1) {
										auto& front = raw->second.as_array()->front();
										// downcast
										input = front.value<std::int64_t>() ?
										            front.value<std::int64_t>().value() :
										            front.value_or<double>(0);
									} else if (raw->second.as_array()->size() > 1) {
										std::vector<std::int64_t> array;
										for (auto& node : *raw->second.as_array()) {
											// default to 0 for numeric types
											// downcast
											input = node.value<std::int64_t>() ?
											            node.value<std::int64_t>().value() :
											            node.value_or<double>(0);
											array.push_back(input);
										}

										data->As<std::int64_t>()->set_data(array);
										break;
									}
								} else {
									// downcast
									input = raw->second.value<std::int64_t>() ?
									            raw->second.value<std::int64_t>().value() :
									            raw->second.value_or<double>(0);
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

		void Generate() noexcept override
		{
			auto tableGnt = []<typename RNG>(RNG&& a_rng) {
				toml::array collection;
				std::ranges::for_each(a_rng, [&collection](auto value) {
					collection.push_back(value);
				});
				return collection;
			};

			_toml.clear();
			for (auto& [key, value] : _manager) {
				auto* data = value.first;
				std::string sanitized = value.second.empty() ? "Global" : value.second.data();
				auto [section, success] = _toml.insert(sanitized, toml::table{});
				auto* table = section->second.as_table();

				switch (data->get_type()) {
				case DataType::kBoolean:
					{
						table->insert(key, data->As<bool>()->get_data());
						break;
					}
				case DataType::kDouble:
					{
						if (auto* raw = data->As<double>(); raw->is_collection()) {
							table->insert(key, tableGnt(raw->get_collection()));
						} else {
							table->insert(key, raw->get_data());
						}
						break;
					}
				case DataType::kInteger:
					{
						if (auto* raw = data->As<std::int64_t>(); raw->is_collection()) {
							table->insert(key, tableGnt(raw->get_collection()));
						} else {
							table->insert(key, raw->get_data());
						}
						break;
					}
				case DataType::kString:
					{
						if (auto* raw = data->As<std::basic_string<char>>(); raw->is_collection()) {
							table->insert(key, tableGnt(raw->get_collection()));
						} else {
							table->insert(key, raw->get_data());
						}
						break;
					}
				case DataType::kError:
				default:
					continue;
				}
			}
		}

	private:
		toml::table _toml;
	};
}  // namespace detail
