#include "DKUtil/Hook.hpp"


namespace Test::Hook
{
	using namespace DKUtil::Alias;

	class LogCallerHook
	{
		static constexpr auto addr = 0x1652D0;

		struct Prolog : Xbyak::CodeGenerator
		{
			Prolog()
			{
				lahf();
				push(rax);
				push(rcx);
				push(rdx);
				push(r8);
				push(r9);
				push(rbp);
				push(rsi);
				push(rdi);
				push(qword[rsp + 0x40]);
			}
		};

		struct Epilog : Xbyak::CodeGenerator
		{
			Epilog()
			{
				pop(rax);
				pop(rdi);
				pop(rsi);
				pop(rbp);
				pop(r9);
				pop(r8);
				pop(rdx);
				pop(rcx);
				pop(rax);
				sahf();
			}
		};

		static void sub_7FF7FA3852D0(std::int64_t a1, const RE::BSFixedString& a_menuname, RE::UI_MESSAGE_TYPE a_type, RE::IUIMessageData* a_data, std::uintptr_t retn)
		{
			static auto base = REL::Module::get().base();
			auto caller = dku::Hook::GetFuncPrologAddr(retn);
			INFO("caller : {:X} | {:X} | {} {}", caller, caller - base, a_menuname.data(), dku::print_enum(a_type));
		}


	public:
		static void Install()
		{
			auto funcAddr = REL::Module::get().base() + addr;

			Prolog SaveStackVar;
			Epilog UnwindStack;

			SaveStackVar.ready();
			UnwindStack.ready();

			SKSE::AllocTrampoline(static_cast<size_t>(1) << 7);
			auto info = FUNC_INFO(sub_7FF7FA3852D0);

			auto handle = DKUtil::Hook::AddCaveHook(
				funcAddr,
				{ 0, 5 },
				info,
				&SaveStackVar,
				&UnwindStack,
				DKUtil::Hook::HookFlag::kRestoreAfterEpilog);

			handle->Enable();

			INFO("installed");
		}
	};





	void Run()
	{
		LogCallerHook::Install();
	}
}
