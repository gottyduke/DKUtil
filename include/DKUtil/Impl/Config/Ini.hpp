#pragma once


#include "data.hpp"

#include "SimpleIni.h"


namespace DKUtil::Config::detail
{
	class Ini final : public IParser
	{
	public:
		using IParser::IParser;

		void Parse(const char* a_data) noexcept override
		{
			_ini.SetUnicode();
			auto result = a_data ? _ini.LoadData(a_data) : _ini.LoadFile(_filepath.c_str());
			if (result < 0) {
				ERROR("DKU_C: Parser#{}: Loading failed! -> {}\n{}", _id, _filepath.c_str(), err_getmsg());
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

					if (_manager.contains(key.pItem)) {
						std::string raw{ value };

						auto* data = _manager.at(key.pItem);
						switch (data->get_type()) {
						case DataType::kBoolean:
							{
								if (raw == "0" || dku::string::iequals(raw, "false")) {
									data->As<bool>()->set_data(false);
								} else if (raw == "1" || dku::string::iequals(raw, "true")) {
									data->As<bool>()->set_data(true);
								} else {
									err_mismatch("Invalid bool input", key.pItem, "Boolean", value);
								}

								break;
							}
						case DataType::kDouble:
							{
								auto sv = dku::string::split(raw, ",", " ");
								try {
									if (sv.size() <= 1) {
										data->As<double>()->set_data(std::stod(value));
									} else {
										auto tv = sv | std::views::transform([](std::string str) { return std::stod(str); });
										data->As<double>()->set_data({ tv.begin(), tv.end() });
									}
								} catch (const std::exception& e) {
									err_mismatch(e.what(), key.pItem, "Double", value);
								}

								break;
							}
						case DataType::kInteger:
							{
								auto sv = dku::string::split(raw, ",", " ");
								try {
									if (sv.size() <= 1) {
										data->As<std::int64_t>()->set_data(std::stoll(value));
									} else {
										auto tv = sv | std::views::transform([](std::string& str) { return std::stoll(str); });
										data->As<std::int64_t>()->set_data({ tv.begin(), tv.end() });
									}
								} catch (const std::exception& e) {
									err_mismatch(e.what(), key.pItem, "Double", value);
								}

								break;
							}
						case DataType::kString:
							{
								std::string old{ raw };
								dku::string::replace_nth_occurrence(raw, 0, "\\,", "_dku_comma_");
								dku::string::replace_nth_occurrence(raw, 0, "\\\"", "_dku_quote_");

								auto sv = dku::string::split(raw, ",");
								try {
									if (sv.size() <= 1) {
										dku::string::trim(old);
										dku::string::replace_nth_occurrence(old, 0, "\"");
										data->As<std::basic_string<char>>()->set_data(old);
									} else {
										std::ranges::for_each(sv, [](std::string& str) {
											dku::string::replace_nth_occurrence(str, 0, "\"");
											dku::string::replace_nth_occurrence(str, 0, "_dku_comma_", "\\,");
											dku::string::replace_nth_occurrence(str, 0, "_dku_quote_", "\\\"");
											dku::string::trim(str);
										});
										data->As<std::basic_string<char>>()->set_data(sv);
									}
								} catch (const std::exception& e) {
									err_mismatch(e.what(), key.pItem, "String", value);
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

			auto sr = _ini.Save(_content);
			if (sr < 0) {
				ERROR("DKU_C: Parser#{}: Saving data failed!\n{}", _id, err_getmsg());
			}

			DEBUG("DKU_C: Parser#{}: Parsing finished", _id);
		}

		void Write(const std::string_view a_filePath) noexcept override
		{
			auto result = a_filePath.empty() ? _ini.SaveFile(_filepath.c_str()) : _ini.SaveFile(a_filePath.data());
			if (result < 0) {
				ERROR("DKU_C: Parser#{}: Writing file failed!\n{}", _id, err_getmsg());
			}

			DEBUG("DKU_C: Parser#{}: Writing finished", _id);
		}

	private:
		const char* err_getmsg() noexcept
		{
			std::ranges::fill(errmsg, 0);
			strerror_s(errmsg, errno);
			return errmsg;
		}

		void err_mismatch(std::string_view a_key, std::string_view a_type, std::string_view a_value, std::string_view a_what) noexcept
		{
			ERROR("DKU_C: Parser#{}: {}\nValue type mismatch!\nFile: {}\nKey: {}, Expected: {}, Value: {}", _id, a_what, _filepath.c_str(), a_key, a_type, a_value);
		}


		CSimpleIniA _ini;
		char errmsg[72];
	};
}  // namespace DKUtil::Config
