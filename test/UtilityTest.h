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
	namespace Enum
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

			RAX = 1,
			RCX = 2,
			RDX = 3,
			RBX = 4,
			RSP = 5,
			RBP = 6,
			RSI = 7,
			RDI = 8,
			RF = 9,
			R8 = 10,
			R9 = 11,
			R10 = 12,
			R11 = 13,
			R12 = 14,
			R13 = 15,
			R14 = 16,
			R15 = 17,
		};

		enum class SparseValue ENUM_TYPE
		{
			NONE = 0u,

			RAX = 1,
			RCX = 2,
			//RDX = 3,
			RBX = 4,
			RSP = 5,
			//RBP = 6,
			RSI = 7,
			RDI = 8,
			RF = 9,
			//R8 = 10,
			R9 = 11,
			//R10 = 12,
			R11 = 13,
			//R12 = 14,
			R13 = 15,
			R14 = 16,
			R15 = 17,
		};
	} // namespace Enum

	constexpr void TestNumbers() noexcept
	{
		// 64B3BC14
		constexpr const char* hashBase32 = "CONSTEXPR_HASH_STRING_32";
		// 7873E548866D72BB
		constexpr const char* hashBase64 = "CONSTEXPR_HASH_STRING_64";
		constexpr auto hash32 = DKUtil::numbers::FNV_1A_32(hashBase32);
		constexpr auto hash64 = DKUtil::numbers::FNV_1A_64(hashBase64);

		static_assert(hash32 == 0x64B3BC14U);
		static_assert(hash64 == 0x7873E548866D72BBULL);
	}


	void TestModel() noexcept
	{

	}


	void TestString() noexcept
	{

	}


	void TestEnum() noexcept
	{
		using namespace Enum;

		// concept-restraint auto ctor
		DKUtil::enumeration<ContiguousValue UNDERLYING> cValues{ 0, 2, 4, 5, 9, 15 };
		DKUtil::enumeration<SparseValue UNDERLYING> sValues{ 0, 2, 4, 5, 9, 15 };
		DKUtil::enumeration<ContiguousFlag UNDERLYING> cFlags{ 0, 2, 4, 5, 9, 15 };
		DKUtil::enumeration<SparseFlag UNDERLYING> sFlags{ SparseFlag::NONE, SparseFlag::RCX, SparseFlag::RBX, SparseFlag::RSI, SparseFlag::R9, SparseFlag::R14 };

		/* static reflections */
		// 1) check for value-type enum reflection
		// 2) check for flag-type enum reflection
		// 3) check for mixed sparse enum reflection
		// 4) check for mixed contiguous enum reflection
		// 5) direct invocation between raw underlying value and enum value
		INFO("Reflecting enum value names");
		for (auto i : std::views::iota(0, 17)) {
			INFO("{}{} {}", cValues.is_flag() ? "1<<" : "", i, cValues.value_name(static_cast<ContiguousValue>(i)));
			INFO("{}{} {}", sValues.is_flag() ? "1<<" : "", i, sValues.value_name(i));
			INFO("{}{} {}", cFlags.is_flag() ? "1<<" : "", i, cFlags.value_name(i));
			INFO("{}{} {}", sFlags.is_flag() ? "1<<" : "", i, sFlags.value_name(i));
		}

		// 5) check for enum class name reflection
		INFO("Enum name:\n{}\n{}\n{}\n{}", cValues.enum_name(), sValues.enum_name(), cFlags.enum_name(), sFlags.enum_name());
		// 6) check for enum underlying type name
		INFO("Type name:\n{}\n{}\n{}\n{}", cValues.type_name(), sValues.type_name(), cFlags.type_name(), sFlags.type_name());
		
		/* value range iterator */
		INFO("Iterating by value_range");
		for (const auto e : cValues.value_range(ContiguousValue::NONE, ContiguousValue::R15)) {
			INFO("{}", cValues.value_name(e));
		}

		INFO("Iterating by flag_range");
		for (const auto e : cFlags.flag_range(ContiguousFlag::NONE, ContiguousFlag::R15)) {
			INFO("{}", cFlags.value_name(e));
		}
	}


	void Run() noexcept
	{
		TestNumbers();
		TestModel();
		TestString();
		TestEnum();
    }
}