#pragma once


#include "Data.hpp"

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

		void Write(const std::string_view a_filePath) noexcept override
		{
			auto* filePath = a_filePath.empty() ? _filePath.data() : a_filePath.data();
			std::basic_ofstream<char> file{ filePath };
			if (!file.is_open()) {
				ERROR("DKU_C: Parser#{}: Writing file failed! -> {}\nofstream cannot be opened", _id, filePath);
			}

			file << _json;
			file.close();
		}

		const void* Data() noexcept override
		{
			_out = std::move(_json.dump());
			return _out.data();
		}

	private:
		json _json;
	};
}  // namespace DKUtil::Config
