#pragma once


#include "Data.hpp"

#include "glaze/glaze.hpp"


namespace DKUtil::Config::detail
{
	class Json final : public IParser
	{
	public:
		using IParser::IParser;

		[[noreturn]] void err_nullable(std::string_view a_key, DataType a_type)
		{
			ERROR("DKU_C: Parser#{}: Parsing failed! -> {}\n\nKey : {}\nType : {}", _id, _filePath, a_key, dku::print_enum(a_type));
		}

		void err_check(glz::parse_error a_pe) 
		{
			if (a_pe) {
				ERROR("DKU_C: Parser#{}: Parsing failed! -> {}\n{}", _id, _filePath, glz::format_error(a_pe, _buffer));
			}
		}

		// TODO: better glaze impl for current config flow
		// generic json, non-existent exception handling, error prone
		void Parse(const char* a_data) noexcept override
		{
			_buffer.clear();
			err_check(glz::read_file_json(_json, _filePath, _buffer));

			const auto& value = std::get<glz::json_t::object_t>(_json.data);
			for (auto& [key, data] : _manager) {
				if (auto raw = value.find(key); raw != value.end()) {
					switch (data->get_type()) {
					case DataType::kBoolean:
						{
							auto* config = data->As<bool>();
							if (const auto* json = raw->second.get_if<bool>(); json) {
								config->set_data(*json);
								break;
							}
							err_nullable(key, DataType::kBoolean);
						}
					case DataType::kDouble:
						{
							auto* config = data->As<double>();
							if (config->is_collection()) {
								if (const auto* json = raw->second.get_if<std::vector<double>>(); json) {
									config->set_data(*json);
									break;
								}
							} else {
								if (const auto* json = raw->second.get_if<double>(); json) {
									config->set_data(*json);
									break;
								}
							}
							err_nullable(key, DataType::kDouble);
						}
					case DataType::kInteger:
						{
							auto* config = data->As<std::int64_t>();
							if (config->is_collection()) {
								if (const auto* json = raw->second.get_if<std::vector<std::int64_t>>(); json) {
									config->set_data(*json);
									break;
								}
							} else {
								if (const auto* json = raw->second.get_if<std::int64_t>(); json) {
									config->set_data(*json);
									break;
								}
							}
							err_nullable(key, DataType::kInteger);
						}
					case DataType::kString:
						{
							auto* config = data->As<std::basic_string<char>>();
							if (config->is_collection()) {
								if (const auto* json = raw->second.get_if<std::vector<std::basic_string<char>>>(); json) {
									config->set_data(*json);
									break;
								}
							} else {
								if (const auto* json = raw->second.get_if<std::basic_string<char>>(); json) {
									config->set_data(*json);
									break;
								}
							}
							err_nullable(key, DataType::kString);
						}
					case DataType::kError:
					default:
						continue;
					}
				}
			}
		}

		void Write(const std::string_view a_filePath) noexcept override
		{
			err_check({ glz::write_file_json(_json, a_filePath.empty() ? _filePath : std::string{ a_filePath }, _buffer).ec, 0 });
		}

	private:
		glz::json_t _json{};
		std::string _buffer{};
	};
}  // namespace DKUtil::Config
