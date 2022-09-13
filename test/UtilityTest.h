#include "DKUtil/Utility.hpp"


namespace Test::Utility
{
	enum class en : std::uint32_t
	{
		NONE = 0u,

		RAX = 1u << 0,
		RCX = 1u << 1,
		RDX = 1u << 2,
		RBX = 1u << 3,
		//RSP = 1u << 4,
		RBP = 1u << 5,
		//RSI = 1u << 6,
		RDI = 1u << 7,

		RF = 1u << 8,

		R8 = 1u << 9,
		//R9 = 1u << 10,
		R10 = 1u << 11,
		//R11 = 1u << 12,
		R12 = 1u << 13,
		R13 = 1u << 14,
		//R14 = 1u << 15,
		R15 = 1u << 16,
	};

	void StartTest() noexcept
    {
		DKUtil::model::enumeration<en> enumer;
		enumer.set(en::RAX, en::RCX, en::RDX, en::RBX);

		for (const en n : enumer.flag_range(en::RAX, en::R15)) {
			if (enumer.any(n)) {
				INFO("{} is set", std::bit_width(std::to_underlying(n)) - 1);
			}
		}
    }

	
	void EndTest() noexcept
	{
	}
}