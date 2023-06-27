#pragma once


#include "Data.hpp"

#include "glaze/glaze.hpp"


namespace DKUtil::Config::detail
{
	class Json final : public IParser
	{
	public:
		using IParser::IParser;

		void check_glz_error(glz::parse_error a_ec)
		{
			if (a_ec != glz::error_code::none) {
				ERROR("DKU_C: Parser#{}: Loading failed! -> {}\n{}", _id, _filePath.c_str(), glz::format_error(a_pe, _buffer));
			}
		}

		// TODO: better glaze impl for current config flow
		// generic json, non-existent exception handling, error prone
		void Parse(const char* a_data) noexcept override
		{
			_buffer.clear();
			check_glz_error(glz::read_file_json(_json, _filePath, _buffer));

			const auto& value = std::get<glz::json_t::object_t>(_json.data);
			for (auto& [key, data] : _manager) {
				if (auto raw = value.find(key); raw != value.end()) {
					switch (data->get_type()) {
					case DataType::kBoolean:
						{
							data->As<bool>()->set_data(raw->second.get<bool>());
							break;
						}
					case DataType::kDouble:
						{
							auto* def_data = data->As<double>();
							if (def_data->is_collection()) {
								def_data->set_data(raw->second.get<std::vector<double>>());
							} else {
								def_data->set_data(raw->second.get<double>());
							}
							break;
						}
					case DataType::kInteger:
						{
							auto* def_data = data->As<std::int64_t>();
							if (def_data->is_collection()) {
								def_data->set_data(raw->second.get<std::vector<std::int64_t>>());
							} else {
								def_data->set_data(raw->second.get<std::int64_t>());
							}
							break;
						}
					case DataType::kString:
						{
							auto* def_data = data->As<std::basic_string<char>>();
							if (def_data->is_collection()) {
								def_data->set_data(raw->second.get<std::vector<std::basic_string<char>>>());
							} else {
								def_data->set_data(raw->second.get<std::basic_string<char>>());
							}
							break;
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
			check_glz_error({ glz::write_file_json(_json, a_filePath.empty() ? _filePath : std::string{ a_filePath }, _buffer).ec, 0});
		}

	private:
		glz::json_t _json{};
		std::string _buffer{};
	};
}  // namespace DKUtil::Config
