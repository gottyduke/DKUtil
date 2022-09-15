#include "DKUtil/Logger.hpp"
#include "DKUtil/Extra.hpp"


namespace Test::Logger
{
	bool bT = true, bF = false;
	int siP = 200, siN = -119;
	std::uint32_t underflowUL = static_cast<std::uint32_t>(-1ul), overflowUL = 0xFFFFFFFFul + 1ul;

	void Run()
	{
		INFO("This is info message:");
	}

	void RunError()
	{

	}
}