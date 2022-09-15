#include "DKUtil/Utility.hpp"

//#define STRONG_TYPED_ENUM
#ifdef STRONG_TYPED_ENUM
#	define ENUM_TYPE : std::uint32_t
#	define UNDERLYING
#else
#	define ENUM_TYPE
#define UNDERLYING , std::uint32_t
#endif

namespace Test::Utility
{
	enum class ContiguousFlag ENUM_TYPE
	{
		NONE = 0u,

		RAX = 1u << 0,
		RCX = 1u << 1,
		RDX = 1u << 2,
		RBX = 1u << 3,
		RSP = 1u << 4,
		RBP = 1u << 5,
		RSI = 1u << 6,
		RDI = 1u << 7,
		RF = 1u << 8,
		R8 = 1u << 9,
		R9 = 1u << 10,
		R10 = 1u << 11,
		R11 = 1u << 12,
		R12 = 1u << 13,
		R13 = 1u << 14,
		R14 = 1u << 15,
		R15 = 1u << 16,
	};

	enum class SparseFlag ENUM_TYPE
	{
		NONE = 0u,

		RAX = 1u << 0,
		RCX = 1u << 1,
		//RDX = 1u << 2,
		RBX = 1u << 3,
		RSP = 1u << 4,
		//RBP = 1u << 5,
		RSI = 1u << 6,
		RDI = 1u << 7,
		RF = 1u << 8,
		//R8 = 1u << 9,
		R9 = 1u << 10,
		//R10 = 1u << 11,
		R11 = 1u << 12,
		//R12 = 1u << 13,
		R13 = 1u << 14,
		R14 = 1u << 15,
		R15 = 1u << 16,
	};

	enum class ContiguousValue ENUM_TYPE
	{
		NONE = 0u,

		RAX = 0,
		RCX = 1,
		RDX = 2,
		RBX = 3,
		RSP = 4,
		RBP = 5,
		RSI = 6,
		RDI = 7,
		RF = 8,
		R8 = 9,
		R9 = 10,
		R10 = 11,
		R11 = 12,
		R12 = 13,
		R13 = 14,
		R14 = 15,
		R15 = 16,
	};

	enum class SparseValue ENUM_TYPE
	{

		NONE = 0u,

		RAX = 0,
		RCX = 1,
		//RDX = 2,
		RBX = 3,
		RSP = 4,
		//RBP = 5,
		RSI = 6,
		RDI = 7,
		RF = 8,
		//R8 = 9,
		R9 = 10,
		//R10 = 11,
		R11 = 12,
		//R12 = 13,
		R13 = 14,
		R14 = 15,
		R15 = 16,
	};


	void TestNumbers() noexcept
	{

	}


	void TestModel() noexcept
	{

	}


	void TestString() noexcept
	{

	}


	void TestEnum() noexcept
	{
		// using concept-restraint auto constructor
		DKUtil::enumeration<ContiguousFlag UNDERLYING> cFlags{ 0, 2, 4, 5, 9, 15 };
		DKUtil::enumeration<SparseFlag UNDERLYING> sFlags{ SparseFlag::NONE, SparseFlag::RCX, SparseFlag::RBX, SparseFlag::RSI, SparseFlag::R9, SparseFlag::R14 };
		DKUtil::enumeration<ContiguousValue UNDERLYING> cValues{ 0, 2, 4, 5, 9, 15 };
		DKUtil::enumeration<SparseValue UNDERLYING> sValues{ 0, 2, 4, 5, 9, 15 };


		/* flag setting */


		/* static reflections */

		for (auto i : std::views::iota(0, 64)) {
			INFO("{} {}", i, cValues.get_value_name(i));
			INFO("{} {}", i, sValues.get_value_name(i));
			INFO("{} {}", i, cFlags.get_value_name(i));
			INFO("{} {}", i, sFlags.get_value_name(i));
		}

		INFO("{} {} {} {}", cValues.get_type_name(), sValues.get_type_name(), cFlags.get_type_name(), sFlags.get_type_name());
	}


	void Run() noexcept
	{
		TestNumbers();
		TestModel();
		TestString();
		TestEnum();
    }
}