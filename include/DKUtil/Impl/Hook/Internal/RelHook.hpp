#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{
	template <std::size_t N, bool RETN>
	std::uintptr_t AddRelHook(std::uintptr_t a_src, std::uintptr_t a_dst)  // noexcept
	{
		static_assert(N == 5 || N == 6, "unsupported instruction size");
		using DetourAsm = std::conditional_t<N == 5, BranchRel<RETN>, BranchRip<RETN>>;

		auto tramPtr = TRAM_ALLOC(0);

		// assumes assembly is safe to read
		Imm64 func = GetDisp(a_src);
		// /2 /4 indirect
		if constexpr (N == 6) {
			func = *std::bit_cast<Imm64*>(func);
		}

		// tram entry
		WriteImm(tramPtr, a_dst, true);
		tramPtr += sizeof(a_dst);

		// detour
		DetourAsm      asmDetour{};
		std::ptrdiff_t disp = tramPtr - a_src - sizeof(asmDetour);
		if constexpr (N == 6) {
			disp -= sizeof(a_dst);
		}
		assert_trampoline_range(disp);

		asmDetour.Disp = static_cast<Disp32>(disp);
		WriteData(a_src, asmDetour.data(), asmDetour.size(), false);

		if constexpr (N == 5) {
			// branch
			JmpRip asmBranch;
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
			asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());

			WriteData(tramPtr, asmBranch.data(), asmBranch.size(), true);
			tramPtr += asmBranch.size();
		}

		DEBUG(
			"DKU_H: Detouring...\n"
			"from : {}.{:X}\n"
			"call : {:X}\n"
			"to   : {}.{:X}",
			GetProcessName(), a_src, func, PROJECT_NAME, a_dst);

		return func;
	}
}  // namespace DKUtil::Hook
