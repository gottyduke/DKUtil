#pragma once


#include "DKUtil/Impl/PCH.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"


namespace DKUtil::Config
{
	inline auto GetPath(const std::string_view a_file) noexcept
	{
		std::filesystem::path dir{ CONFIG_ENTRY };
		std::filesystem::path file{ a_file.data() };

		if (!dir.empty()) {
			file = dir / file;
		}

		return std::move(file.string());
	}


	namespace detail
	{
		inline static std::uint32_t Count{ 0 };

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
			virtual void Write(const std::string_view) noexcept = 0;
			virtual const void* Data() noexcept = 0;

		protected:
			const std::string _file;
			const std::string _filePath;
			const std::uint32_t _id;
			std::string _out;
			DataManager& _manager;
		};
	}
}
