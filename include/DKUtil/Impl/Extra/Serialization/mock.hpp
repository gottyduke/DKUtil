#pragma once


#include "shared.hpp"
#include "exception.hpp"


#ifdef DKU_X_MOCK
#	include "mock.hpp"
#	include <fmt/color.h>

#	define DKU_X_MOCK_WRITE(D, L) mock::write(D, L)
#	define DKU_X_MOCK_READ(D, L) mock::read(D, L)
#	define DKU_X_MOCK_REPORT() mock::report()


namespace DKUtil::serialization
{
	namespace mock
	{
		inline static std::array<std::byte, 0x1000> Buffer;
		inline static std::size_t ReadPos = 0;
		inline static std::size_t WritePos = 0;


		void read(void* a_buf, std::uint32_t a_length) noexcept
		{
			std::memcpy(a_buf, Buffer.data() + ReadPos, a_length);
			ReadPos += a_length;
			INFO(fmt::format(
				fmt::bg(fmt::terminal_color::green) | fmt::fg(fmt::terminal_color::black), 
				"[mock] read {}B", a_length));
		}

		void write(const void* a_buf, std::uint32_t a_length) noexcept
		{
			std::memcpy(Buffer.data() + WritePos, a_buf, a_length);
			WritePos += a_length;
			INFO(fmt::format(
				fmt::bg(fmt::terminal_color::cyan) | fmt::fg(fmt::terminal_color::black), 
				"[mock] write {}B", a_length));
		}

		void clear() noexcept
		{
			Buffer.fill(std::byte{ 0 });
			ReadPos = 0;
			WritePos = 0;
		}

		void report() noexcept
		{
			INFO("[mock] current read {}B", ReadPos);
			INFO("[mock] current write {}B", WritePos);
		}
	}  // namespace mock
} // namespace DKUtil::serialization

#else
#	define DKU_X_MOCK_WRITE(a_res, a_data)
#	define DKU_X_MOCK_READ(a_res, a_data)
#	define DKU_X_MOCK_READ_WRITE(a_res, a_data)
#	define DKU_X_MOCK_REPORT()
#endif