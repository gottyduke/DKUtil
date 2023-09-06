#pragma once


#include "data.hpp"

#include "nlohmann/json.hpp"


namespace DKUtil::Config::detail
{
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
				std::basic_ifstream<char> file{ _filepath };
				if (!file.is_open()) {
					ERROR("DKU_C: Parser#{}: Loading failed! -> {}", _id, _filepath.c_str());
				}

				file >> _json;
				file.close();
			}

			for (auto& [key, value] : _manager) {
				auto* data = value.first;
				auto raw = _json.find(key.data());
				if (raw == _json.end()) {
					ERROR("DKU_C: Parser#{}: Retrieving config failed!\nFile: {}\nKey: {}", _id, _filepath.c_str(), key);
				}

				switch (data->get_type()) {
				case DataType::kBoolean:
					{
						data->As<bool>()->set_data(raw->get<bool>());
						break;
					}
				case DataType::kDouble:
					{
						if (raw->type() == json::value_t::array) {
							data->As<double>()->set_data(raw->get<std::vector<double>>());
						} else {
							data->As<double>()->set_data(raw->get<double>());
						}
						break;
					}
				case DataType::kInteger:
					{
						if (raw->type() == json::value_t::array) {
							data->As<std::int64_t>()->set_data(raw->get<std::vector<std::int64_t>>());
						} else {
							data->As<std::int64_t>()->set_data(raw->get<std::int64_t>());
						}
						break;
					}
				case DataType::kString:
					{
						if (raw->type() == json::value_t::array) {
							data->As<std::basic_string<char>>()->set_data(raw->get<std::vector<std::basic_string<char>>>());
						} else {
							data->As<std::basic_string<char>>()->set_data(raw->get<std::basic_string<char>>());
						}
						break;
					}
				case DataType::kError:
				default:
					continue;
				}
			}

			_content = std::move(_json.dump());
			DEBUG("DKU_C: Parser#{}: Parsing finished", _id);
		}

		void Write(const std::string_view a_filePath) noexcept override
		{
			auto* filePath = a_filePath.empty() ? _filepath.data() : a_filePath.data();
			std::basic_ofstream<char> file{ filePath };
			if (!file.is_open() || !file) {
				ERROR("DKU_C: Parser#{}: Writing file failed! -> {}\nofstream cannot be opened", _id, filePath);
			}

			file << _json.dump(4);
			file.close();

			DEBUG("DKU_C: Parser#{}: Writing finished", _id);
		}

		void Generate() noexcept override
		{
			_json.clear();
			for (auto& [key, value] : _manager) {
				auto* data = value.first;
				switch (data->get_type()) {
				case DataType::kBoolean:
					{
						_json[key.data()] = data->As<bool>()->get_data();
						break;
					}
				case DataType::kDouble:
					{
						if (auto* raw = data->As<double>(); raw->is_collection()) {
							_json[key.data()] = raw->get_collection();
						} else {
							_json[key.data()] = raw->get_data();
						}
						break;
					}
				case DataType::kInteger:
					{
						if (auto* raw = data->As<std::int64_t>(); raw->is_collection()) {
							_json[key.data()] = raw->get_collection();
						} else {
							_json[key.data()] = raw->get_data();
						}
						break;
					}
				case DataType::kString:
					{
						if (auto* raw = data->As<std::basic_string<char>>(); raw->is_collection()) {
							_json[key.data()] = raw->get_collection();
						} else {
							_json[key.data()] = raw->get_data();
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
		json _json;
	};
}  // namespace DKUtil::Config
