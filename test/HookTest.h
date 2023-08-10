#include "DKUtil/Hook.hpp"


namespace Test::Hook
{
	using namespace DKUtil::Alias;

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
				auto handle = DKUtil::Hook::AddASMPatch(
					AttackDistanceBase.address(), 
					{ 0x22, 0x2C }, 
					{ "\x4C\x8B\x4C\x24\xB8""\xE9\x89\xFC\xFF\xFF", 10 });

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

				auto funcAddr = REL::Module::get().base() + FuncID;
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


	void Run()
	{
		Impl::RecalculateCombatRadiusHook::InstallHook();
		Impl::RescaleCircleChanceHook::InstallHook();
		Impl::FallbackDistanceHook::InstallHook();

		TestPattern();
	}
}
