#include "DKUtil/Utility.hpp"

//#define STRONG_TYPED_ENUM
#ifdef STRONG_TYPED_ENUM
#	define ENUM_TYPE : std::uint32_t
#	define UNDERLYING
#else
#	define ENUM_TYPE
#	define UNDERLYING , std::uint32_t
#endif

namespace Test::Utility
{
	using namespace DKUtil::Alias;

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

		void TestEnum() noexcept
		{
			// concept-restraint auto ctor
			dku::enumeration<ContiguousValue UNDERLYING> cValues{ 0, 2, 4, 5, 9, 15 };
			dku::enumeration<SparseValue UNDERLYING> sValues{ 0, 2, 4, 5, 9, 15 };
			dku::enumeration<ContiguousFlag UNDERLYING> cFlags{ 0, 2, 4, 5, 9, 15 };
			dku::enumeration<SparseFlag UNDERLYING> sFlags{ SparseFlag::NONE, SparseFlag::RCX, SparseFlag::RBX, SparseFlag::RSI, SparseFlag::R9, SparseFlag::R14 };

			// static reflections
			// 1) check for value-type enum reflection
			// 2) check for flag-type enum reflection
			// 3) check for mixed sparse enum reflection
			// 4) check for mixed contiguous enum reflection
			// 5) direct invocation between raw underlying value and enum value
			INFO("Reflecting enum value names");
			for (auto i : std::views::iota(0, 17)) {
				INFO("{}{} {}", cValues.is_flag() ? "1<<" : "", i, cValues.to_string(static_cast<ContiguousValue>(i)));
				INFO("{}{} {}", sValues.is_flag() ? "1<<" : "", i, sValues.to_string(i));
				INFO("{}{} {}", cFlags.is_flag() ? "1<<" : "", i, cFlags.to_string(i));
				INFO("{}{} {}", sFlags.is_flag() ? "1<<" : "", i, sFlags.to_string(i));
			}

			// 5) check for enum class name reflection
			INFO("Enum name:\n{}\n{}\n{}\n{}", cValues.enum_name(), sValues.enum_name(), cFlags.enum_name(), sFlags.enum_name());
			// 6) check for enum underlying type name
			INFO("Type name:\n{}\n{}\n{}\n{}", cValues.type_name(), sValues.type_name(), cFlags.type_name(), sFlags.type_name());

			// 7) value range iterator
			INFO("Iterating by value_range");
			for (const auto e : cValues.value_range(ContiguousValue::NONE, ContiguousValue::R15)) {
				INFO("{}", cValues.to_string(e));
			}

			INFO("Iterating by flag_range");
			for (const auto e : cFlags.flag_range(ContiguousFlag::NONE, ContiguousFlag::R15)) {
				INFO("{}", cFlags.to_string(e));
			}

			// 8) string to enum cast
			std::string nameStr1{ "rsi" };
			std::string nameStr2{ "RaX" };
			std::string nameStr3{ "Rdx" };

			auto nameEnum1 = cValues.from_string(nameStr1);
			auto nameEnum2 = cValues.from_string(nameStr2);
			auto nameEnum3 = cValues.from_string(nameStr3);

			INFO("String {} -> Enum {}", nameStr1, std::to_underlying(nameEnum1.value()));
			INFO("String {} -> Enum {}", nameStr2, std::to_underlying(nameEnum2.value()));
			INFO("String {} -> Enum {}", nameStr3, std::to_underlying(nameEnum3.value()));
			ContiguousValue Enum1 = nameEnum1.value();
		}

		void TestEnumBitwidth() noexcept
		{
			auto& sf = dku::static_enum<SparseFlag>();
			auto& cf = dku::static_enum<ContiguousFlag>();

			auto str = "RDX";
			auto strEnum = sf.from_string(str);
			INFO("{} {}", dku::print_enum(strEnum.value_or(SparseFlag::RAX)), std::to_underlying(strEnum.value_or(SparseFlag::RAX)));
		}
	}  // namespace Enum

	consteval void TestNumbers() noexcept
	{
		// 64B3BC14
		constexpr const char* hashBase32 = "CONSTEXPR_HASH_STRING_32";
		// 7873E548866D72BB
		constexpr const char* hashBase64 = "CONSTEXPR_HASH_STRING_64";

		constexpr auto hash32 = dku::numbers::FNV_1A_32(hashBase32);
		constexpr auto hash64 = dku::numbers::FNV_1A_64(hashBase64);

		static_assert(hash32 == 0x64B3BC14U);
		static_assert(hash64 == 0x7873E548866D72BBULL);
	}


	void TestModel() noexcept
	{
		struct TestAggregate
		{
			int i;
			std::string s;
			char c;
			bool b;
		};

		auto tv = dku::model::tuple_cast(TestAggregate{});
		auto sv = dku::model::struct_cast<TestAggregate>(tv);
		int av[] = { 1, 2, 3, 4 };

		// bindables
		static_assert(dku::model::number_of_bindables<TestAggregate>() == 4);
		static_assert(dku::model::number_of_bindables<decltype(tv)>() == 4);
		static_assert(dku::model::number_of_bindables<decltype(av)>() == 4);

		// concepts
		static_assert(dku::model::concepts::dku_aggregate<TestAggregate>);
		static_assert(dku::model::concepts::dku_bindable<decltype(tv)>);
		static_assert(dku::model::concepts::dku_ranges<decltype(av)>);
		static_assert(dku::model::concepts::dku_trivial_ranges<decltype(av)>);
	}


	void TestString() noexcept
	{
		auto str = "D:\\WorkSpace\\SKSEPlugins\\Plugins\\EnderalHeroMenu\\src\\LoadGame.cpp:36"sv;
		auto pat = "\\"sv;
		auto rep = "|"sv;

		// 1) replace_nth
		INFO("replace all \\ with |");
		INFO(dku::string::replace_nth_occurrence(str, 0, pat, rep));
		
		INFO("remove all \\");
		INFO(dku::string::replace_nth_occurrence(str, 0, pat));

		INFO("replace 2nd \\ with |");
		INFO(dku::string::replace_nth_occurrence(str, 2, pat, rep));

		INFO("replace 2nd to last \\ with |");
		INFO(dku::string::replace_nth_occurrence(str, -2, pat, rep));

		INFO("replace 100th \\ with | (out of bound)");
		INFO(dku::string::replace_nth_occurrence(str, 100, pat, rep));

		
		// 2) split, join
		constexpr std::string_view words{ "DKUtil|TestUsage|Schema|String 002| That " };
		auto token = dku::string::split(words, "|");

		for (auto& t : token) {
			INFO(t);
		}

		INFO(dku::string::join(token, "|"));
	}


	void Run() noexcept
	{
		//TestNumbers();
		//TestModel();
		
		//TestString();
		
		Enum::TestEnum();
		Enum::TestEnumBitwidth();
	}
}