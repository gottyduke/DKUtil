#pragma once


#include "DKUtil/Impl/PCH.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"


namespace DKUtil::Config
{
	inline auto GetPath(const std::string_view a_file) noexcept
	{
		std::filesystem::path pluginDir(CONFIG_ENTRY);
		std::filesystem::path file(a_file.data());
		return std::move((pluginDir / file).string());
	}


	namespace detail
	{
		class DataManager;
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
			const std::string _file;
			const std::string _filePath;
			const std::uint32_t _id;
			DataManager& _manager;
		};
	}
}
