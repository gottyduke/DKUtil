#pragma once


#include "exception.hpp"
#include "shared.hpp"


#ifdef DKU_X_MOCK
#	include <fmt/color.h>


#	define DKU_X_WRITE(D, L, T) mock::write(D, L)
#	define DKU_X_WRITE_SIZE(D) DKU_X_WRITE(std::addressof(D), sizeof(D), decltype(D))
#	define DKU_X_READ(D, L, T) mock::read(D, L)
#	define DKU_X_READ_SIZE(D) DKU_X_READ(std::addressof(D), sizeof(D), decltype(D))
#	define DKU_X_REPORT() mock::report()
#	define DKU_X_FORMID(F) F


namespace DKUtil::serialization
{
	namespace mock
	{
		inline static std::array<std::byte, 0x1000> Buffer;
		inline static size_type ReadPos = 0;
		inline static size_type WritePos = 0;


		inline void read(void* a_buf, size_type a_length) noexcept
		{
			std::memcpy(a_buf, Buffer.data() + ReadPos, a_length);
			ReadPos += a_length;
			INFO(fmt::format(
				fmt::bg(fmt::terminal_color::green) | fmt::fg(fmt::terminal_color::black),
				"[mock] read {}B", a_length));
		}

		inline void write(const void* a_buf, size_type a_length) noexcept
		{
			std::memcpy(Buffer.data() + WritePos, a_buf, a_length);
			WritePos += a_length;
			INFO(fmt::format(
				fmt::bg(fmt::terminal_color::cyan) | fmt::fg(fmt::terminal_color::black),
				"[mock] write {}B", a_length));
		}

		inline void clear() noexcept
		{
			Buffer.fill(std::byte{ 0 });
			ReadPos = 0;
			WritePos = 0;
		}

		inline void report() noexcept
		{
			INFO("[mock] current read {}B", ReadPos);
			INFO("[mock] current write {}B", WritePos);
		}
	}  // namespace mock
}  // namespace DKUtil::serialization
#endif