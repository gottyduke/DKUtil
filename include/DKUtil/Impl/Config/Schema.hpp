#pragma once


#include "Data.hpp"

#include "SimpleIni.h"


namespace DKUtil::Config::detail
{
	class Schema final : public IParser
	{
	public:
		using IParser::IParser;

		void Parse(const char* a_data) noexcept override
		{
		}

		void Write(const std::string_view a_filePath) noexcept override
		{
		}

		const void* Data() noexcept override
		{
		}

	private:
		std::vector<std::string> _tokens;
	};
}