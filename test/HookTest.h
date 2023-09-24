#include "DKUtil/Hook.hpp"

namespace Test::Hook
{
	using namespace DKUtil::Alias;
	using namespace dku::Hook::Assembly;

	// taken from MaxSu
	namespace Impl
	{
		class RecalculateCombatRadiusHook
		{
			// 1-6-353-0 @ 0x870CF0
			static constexpr std::uint64_t AE_FuncID = 50643;
			// 1-5-97-0 @ 0x844E40
			static constexpr std::uint64_t SE_FuncID = 49716;

			static constexpr std::ptrdiff_t RadiusL = 0x99;
			static constexpr std::ptrdiff_t RadiusH = 0xA0;
			static constexpr std::ptrdiff_t MedianL = 0x139;
			static constexpr std::ptrdiff_t MedianH = 0x145;

			static constexpr Patch RadiusPatch{
				// lahf
				"\x9F\x50"
				// mov rdx, rdi
				"\x48\x89\xFA"
				// movss xmm2, xmm6
				"\xF3\x0F\x10\xD6"
				// mov r9, rbp
				"\x49\x89\xE9"
				// mov [rsp-0x8], rbx
				"\x48\x89\x5C\x24\xF8",
				17
			};

			static constexpr Patch MedianPatch{
				// xor cl
				"\x30\xC9",
				19
			};

			static constexpr Patch Epilog{
				// sahf
				"\x58\x9E",
				2
			};

			// cl, rdx, xmm2, r9, rsp-0x8
			static void RecalculateCombatRadius(bool a_fullRadius, float* a_radius, float a_delta, RE::Character* a_center, RE::Character* a_actor)
			{
				static_assert(sizeof(float) == 0x4);

				if (!a_radius || !a_center || !a_actor) {
					return;
				}

				auto& inner = a_radius[0];
				auto& outer = a_radius[2];
				// rax rcx rdx r8 r9 r10 r11 AF CF TF
				// xmm0 xmm1 xmm3 xmm4
				// recalc
				DEBUG("Set full radius: {}", a_fullRadius);
				DEBUG("Me: {} | {:X} <-> He: {} | {:X}", a_center->GetName(), a_center->GetFormID(), a_actor->GetName(), a_actor->GetFormID());
				DEBUG("InnerR: {} {:X} | OuterR {} {:X}", inner, AsAddress(&a_radius[0]), outer, AsAddress(&a_radius[2]));
				DEBUG("Delta: {}", a_delta);
			}

		public:
			static void InstallHook()
			{
				SKSE::AllocTrampoline(static_cast<size_t>(1) << 7);

				auto Hook_SetRadius = DKUtil::Hook::AddCaveHook(
					DKUtil::Hook::IDToAbs(50643, 49716),
					{ RadiusL, RadiusH },
					FUNC_INFO(RecalculateCombatRadius),
					&RadiusPatch,
					&Epilog,
					{ HookFlag::kRestoreBeforeProlog, HookFlag::kSkipNOP });

				Hook_SetRadius->Enable();

				auto Hook_SetMedian = DKUtil::Hook::AddCaveHook(
					DKUtil::Hook::IDToAbs(50643, 49716),
					{ MedianL, MedianH },
					FUNC_INFO(RecalculateCombatRadius),
					&MedianPatch,
					&Epilog,
					{ HookFlag::kRestoreBeforeProlog, HookFlag::kSkipNOP });

				// recalculate displacement
				Disp32 disp = *std::bit_cast<Disp32*>(AsPointer(Hook_SetMedian->TramEntry + 0x4));
				DKUtil::Hook::WriteImm(Hook_SetMedian->TramEntry + 0x4, static_cast<Disp32>(Hook_SetMedian->CaveEntry + disp - Hook_SetMedian->TramEntry));

				DKUtil::Hook::WritePatch(Hook_SetMedian->TramEntry + Hook_SetMedian->CaveSize + 0x2, &RadiusPatch);

				Hook_SetMedian->Enable();

				INFO("Hook SetCombatRadius!");
			}
		};

		class RescaleCircleChanceHook
		{
			static float RescaleCircleChance(const float a_circleMult, const float a_minChance, const float a_maxChance, [[maybe_unused]] RE::Character* a_actor = nullptr)
			{
				if (!a_actor) {
					return 0.0f;
				}

				auto f = _RescaleCircleChance(a_circleMult, a_minChance, a_maxChance, a_actor);
				DEBUG("Mult {} Min {} Max {}", a_circleMult, a_minChance, a_maxChance);
				DEBUG("Actor: {} | {:X}", a_actor->GetName(), a_actor->GetFormID());
				DEBUG("Original Result {}", f);

				return 1.f;
			}

			static inline REL::Relocation<decltype(RescaleCircleChance)> _RescaleCircleChance;

		public:
			static void InstallHook()
			{
				SKSE::AllocTrampoline(static_cast<size_t>(1) << 4);
				auto& trampoline = SKSE::GetTrampoline();

				std::array<std::uint8_t, 5> P{ 0x48, 0x8B, 0x4C, 0x24, 0x20 };

				REL::Relocation<std::uintptr_t> AttackDistanceBase{ REL::RelocationID(49720, 50647) };
				auto                            handle = DKUtil::Hook::AddASMPatch(
                    AttackDistanceBase.address(),
                    { 0x22, 0x2C },
                    { "\x4C\x8B\x4C\x24\xB8"
																			"\xE9\x89\xFC\xFF\xFF",
												   10 });

				// recalculate displacement
				const auto _originalFunc = *std::bit_cast<std::uintptr_t*>(AttackDistanceBase.address() + 0x23);
				DKUtil::Hook::WriteImm(handle->TramEntry + 0x6, _originalFunc + 0x5);
				handle->Enable();

				_RescaleCircleChance = trampoline.write_branch<5>(AttackDistanceBase.address() + 0x27, RescaleCircleChance);
				INFO("Hook RescaleCircleChance!");
			}
		};

		class FallbackDistanceHook
		{
			static float RecalculateFallbackDistance(RE::Character* a_me, RE::Character* a_he)
			{
				if (!a_me || !a_he) {
					return 0.f;
				}

				DEBUG("ME: {}|0x{:08X} <-> HE: {}|0x{:08X}", a_me->GetName(), a_me->GetFormID(), a_he->GetName(), a_he->GetFormID());

				return 256.f;
			}

			static constexpr std::uintptr_t FuncID = 0x7D7740;
			static constexpr std::ptrdiff_t OffsetL = 0x246;
			static constexpr std::ptrdiff_t OffsetH = 0x24E;

			static constexpr Patch RelocateReturn{
				// addss xmm6, xmm0
				"\xF3\x0F\x58\xF0",
				4
			};

		public:
			static void InstallHook()
			{
				SKSE::AllocTrampoline(1 << 6);

				auto  funcAddr = REL::Module::get().base() + FuncID;
				Patch RelocatePointer{
					AsPointer(funcAddr + OffsetL + 0x10),
					6
				};

				auto handle = DKUtil::Hook::AddCaveHook(
					funcAddr,
					{ OffsetL, OffsetH },
					FUNC_INFO(RecalculateFallbackDistance),
					&RelocatePointer,
					&RelocateReturn,
					HookFlag::kNoFlag);

				handle->Enable();

				INFO("Installed FallbackDistanceHook");
			}
		};
	}  // namespace Impl

	void TestPattern()
	{
		namespace assembly = DKUtil::Hook::Assembly;
		namespace pattern = DKUtil::Hook::Assembly::Pattern;
		namespace rules = DKUtil::Hook::Assembly::Pattern::rules;

		static_assert(rules::Hexadecimal<'E', 'B'>::match(std::byte{ 0xEB }));
		static_assert(rules::Hexadecimal<'9', '0'>::match(std::byte{ 0x90 }));
		static_assert(rules::Hexadecimal<'0', 'F'>::match(std::byte{ 0x0F }));
		static_assert(rules::Hexadecimal<'C', 'C'>::match(std::byte{ 0xCC }));
		static_assert(rules::Hexadecimal<'1', '2'>::match(std::byte{ 0x12 }));
		static_assert(rules::Wildcard::match(std::byte{ 0xCC }));
		static_assert(rules::Wildcard::match(std::byte{ 0xEB }));
		static_assert(rules::Wildcard::match(std::byte{ 0x90 }));

		static_assert(assembly::make_pattern<"40 10 F2 ??">().match(
			pattern::make_byte_array(0x40, 0x10, 0xF2, 0x41)));
		static_assert(assembly::make_pattern<"B8 D0 ?? ?? D4 6E">().match(
			pattern::make_byte_array(0xB8, 0xD0, 0x35, 0x2A, 0xD4, 0x6E)));
	}

	void TestHooks()
	{
		Impl::RecalculateCombatRadiusHook::InstallHook();
		Impl::RescaleCircleChanceHook::InstallHook();
		Impl::FallbackDistanceHook::InstallHook();
	}

#define PACK_BIG_ENDIAN(lo1, lo2, hi1, hi2) ((((lo1)&0xFF) << 0) | (((lo2)&0xFF) << 8) | (((hi1)&0xFF) << 16) | ((hi2)&0xFF) << 24)
	void TestDispHelpers()
	{
		// clang-format off
		constexpr OpCode asmBuf[] = { 0x8C, 0x05, 0x78, 0x56, 0x34, 0x12, };
		// clang-format on

		auto rip = &asmBuf[0];
		INFO("rip {:X}", AsAddress(rip));
		INFO("Op : 0x{:2X}", rip[0]);
		auto dst = dku::Hook::GetDisp(rip);
		INFO("dst : 0x{:X}", dst);
		auto disp = dst - AsAddress(rip) - sizeof(asmBuf);
		INFO("disp : 0x{:X}", disp);

		auto offset = sizeof(asmBuf) - sizeof(Disp32);
		auto packed = PACK_BIG_ENDIAN(asmBuf[offset + 0], asmBuf[offset + 1], asmBuf[offset + 2], asmBuf[offset + 3]);
		dku_assert(packed == disp, 
			"incorrect");
	}

	void TestJIT()
	{
		// 1) regular registers all
		{
			auto [prolog, epilog] = dku::Hook::JIT::MakeNonVolatilePatch(Register::ALL);
			auto prolog_view = std::span<OpCode>{ prolog };
			auto epilog_view = std::span<OpCode>{ epilog };
			// clang-format off
			constexpr OpCode expected_prolog[] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x9C, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57 };
			constexpr OpCode expected_epilog[] = { 0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x9D, 0x5F, 0x5E, 0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58 };
			// clang-format on

			dku_assert(std::ranges::equal(prolog_view, expected_prolog),
				"prolog incorrect");
			dku_assert(std::ranges::equal(epilog_view, expected_epilog),
				"epilog incorrect");
		}  
		
		// 2) regular registers partial
		{
			// the preserve order is ascending, so actual patch is RBX, RDI
			auto [prolog, epilog] = dku::Hook::JIT::MakeNonVolatilePatch({ Register::RDI, Register::RBX, Register::R13 });
			auto prolog_view = std::span<OpCode>{ prolog };
			auto epilog_view = std::span<OpCode>{ epilog };
			// clang-format off
			constexpr OpCode expected_prolog[] = { 0x53, 0x57, 0x41, 0x55 };
			constexpr OpCode expected_epilog[] = { 0x41, 0x5D, 0x5F, 0x5B };
			// clang-format on

			dku_assert(std::ranges::equal(prolog_view, expected_prolog),
				"prolog incorrect");
			dku_assert(std::ranges::equal(epilog_view, expected_epilog),
				"epilog incorrect");
		}

		// 3) sse registers all
		{
			auto [prolog, epilog] = dku::Hook::JIT::MakeNonVolatilePatch(SIMD::ALL);
			auto prolog_view = std::span<OpCode>{ prolog };
			auto epilog_view = std::span<OpCode>{ epilog };
			// clang-format off
			constexpr OpCode expected_prolog[] = { 0x48, 0x81, 0xEC, 0x00, 0x01, 0x00, 0x00, 0xC5, 0xFA, 0x7F, 0x04, 0x24, 0xC5, 0xFA, 0x7F, 0x4C, 0x24, 0x10, 0xC5, 0xFA, 0x7F, 0x54, 0x24, 0x20, 0xC5, 0xFA, 0x7F, 0x5C, 0x24, 0x30, 0xC5, 0xFA, 0x7F, 0x64, 0x24, 0x40, 0xC5, 0xFA, 0x7F, 0x6C, 0x24, 0x50, 0xC5, 0xFA, 0x7F, 0x74, 0x24, 0x60, 0xC5, 0xFA, 0x7F, 0x7C, 0x24, 0x70, 0xC5, 0x7A, 0x7F, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0x8C, 0x24, 0x90, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0x94, 0x24, 0xA0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0x9C, 0x24, 0xB0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0xA4, 0x24, 0xC0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0xAC, 0x24, 0xD0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0xB4, 0x24, 0xE0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x7F, 0xBC, 0x24, 0xF0, 0x00, 0x00, 0x00 };
			constexpr OpCode expected_epilog[] = { 0xC5, 0x7A, 0x6F, 0xBC, 0x24, 0xF0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0xB4, 0x24, 0xE0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0xAC, 0x24, 0xD0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0xA4, 0x24, 0xC0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0x9C, 0x24, 0xB0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0x94, 0x24, 0xA0, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0x8C, 0x24, 0x90, 0x00, 0x00, 0x00, 0xC5, 0x7A, 0x6F, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, 0xC5, 0xFA, 0x6F, 0x7C, 0x24, 0x70, 0xC5, 0xFA, 0x6F, 0x74, 0x24, 0x60, 0xC5, 0xFA, 0x6F, 0x6C, 0x24, 0x50, 0xC5, 0xFA, 0x6F, 0x64, 0x24, 0x40, 0xC5, 0xFA, 0x6F, 0x5C, 0x24, 0x30, 0xC5, 0xFA, 0x6F, 0x54, 0x24, 0x20, 0xC5, 0xFA, 0x6F, 0x4C, 0x24, 0x10, 0xC5, 0xFA, 0x6F, 0x04, 0x24, 0x48, 0x81, 0xC4, 0x00, 0x01, 0x00, 0x00 };
			// clang-format on

			dku_assert(std::ranges::equal(prolog_view, expected_prolog),
				"prolog incorrect");
			dku_assert(std::ranges::equal(epilog_view, expected_epilog),
				"epilog incorrect");
		}
		
		// 4) sse registers partial
		{
			auto [prolog, epilog] = dku::Hook::JIT::MakeNonVolatilePatch({ SIMD::XMM0, SIMD::XMM7, SIMD::XMM10 });
			auto prolog_view = std::span<OpCode>{ prolog };
			auto epilog_view = std::span<OpCode>{ epilog };
			// clang-format off
			constexpr OpCode expected_prolog[] = { 0x48, 0x81, 0xEC, 0x30, 0x00, 0x00, 0x00, 0xC5, 0xFA, 0x7F, 0x04, 0x24, 0xC5, 0xFA, 0x7F, 0x7C, 0x24, 0x10, 0xC5, 0x7A, 0x7F, 0x54, 0x24, 0x20 };
			constexpr OpCode expected_epilog[] = { 0xC5, 0x7A, 0x6F, 0x54, 0x24, 0x20, 0xC5, 0xFA, 0x6F, 0x7C, 0x24, 0x10, 0xC5, 0xFA, 0x6F, 0x04, 0x24, 0x48, 0x81, 0xC4, 0x30, 0x00, 0x00, 0x00 };
			// clang-format on

			dku_assert(std::ranges::equal(prolog_view, expected_prolog),
				"prolog incorrect");
			dku_assert(std::ranges::equal(epilog_view, expected_epilog),
				"epilog incorrect");
		}

		// 5) combine patch

		{
			auto [prolog1, epilog1] = dku::Hook::JIT::MakeNonVolatilePatch({ Register::RBX, Register::RDI });
			auto [prolog2, epilog2] = dku::Hook::JIT::MakeNonVolatilePatch({ SIMD::XMM0, SIMD::XMM7, SIMD::XMM10 });
			prolog1.Append(prolog2).Append(epilog2).Append(epilog1);
			auto patch_view = std::span<OpCode>{ prolog1 };
			// clang-format off
			constexpr OpCode expected_patch[] = { 0x53, 0x57, 0x48, 0x81, 0xEC, 0x30, 0x00, 0x00, 0x00, 0xC5, 0xFA, 0x7F, 0x04, 0x24, 0xC5, 0xFA, 0x7F, 0x7C, 0x24, 0x10, 0xC5, 0x7A, 0x7F, 0x54, 0x24, 0x20, 0xC5, 0x7A, 0x6F, 0x54, 0x24, 0x20, 0xC5, 0xFA, 0x6F, 0x7C, 0x24, 0x10, 0xC5, 0xFA, 0x6F, 0x04, 0x24, 0x48, 0x81, 0xC4, 0x30, 0x00, 0x00, 0x00, 0x5F, 0x5B };
			// clang-format on

			dku_assert(std::ranges::equal(patch_view, expected_patch),
				"patch incorrect");
		}
	}

	void Run()
	{
		//TestHooks();
		TestPattern();
		TestDispHelpers();
		TestJIT();

		//dku::Hook::write_call_ex<6>(0, Run, { Register::RAX, Register::RCX, Register::RDX, Register::RBX });
	}
}
