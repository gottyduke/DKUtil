#pragma once


#include "Data.hpp"

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

		void Write(const std::string_view a_filePath) noexcept override
		{
			auto result = a_filePath.empty() ? _ini.SaveFile(_filePath.c_str()) : _ini.SaveFile(a_filePath.data());
			if (result < 0) {
				ERROR("DKU_C: Parser#{}: Writing file failed!\n{}", _id, err_getmsg());
			}
		}

		const void* Data() noexcept override
		{
			auto result = _ini.Save(_out);
			if (result < 0) {
				ERROR("DKU_C: Parser#{}: Saving data failed!\n{}", _id, err_getmsg());
			}

			return _out.data();
		}

	private:
		const char* err_getmsg() noexcept
		{
			std::ranges::fill(errmsg, 0);
			strerror_s(errmsg, errno);
			return errmsg;
		}

		void err_mismatch(const char* a_key, const char* a_type, const char* a_value) noexcept
		{
			ERROR("DKU_C: Parser#{}: Value type mismatch!\nFile: {}\nKey: {}, Expected: {}, Value: {}", _id, _filePath.c_str(), a_key, a_type, a_value);
		}


		CSimpleIniA _ini;
		char errmsg[72];
	};
}  // namespace DKUtil::Config
