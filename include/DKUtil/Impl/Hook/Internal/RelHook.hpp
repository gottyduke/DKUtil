#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{
	class RelHookHandle : public HookHandle
	{
	public:
		RelHookHandle(
			const std::uintptr_t a_callsite,
			const std::uintptr_t a_tramPtr,
			const std::uintptr_t a_dstAddr,
			const std::uint8_t   a_opSize) noexcept :
			HookHandle(a_callsite, a_tramPtr),
			OpSeqSize(a_opSize),
			OriginalFunc(GetDisp(a_callsite)),
			Destination(a_dstAddr)
		{
			__DEBUG(
				"DKU_H: Relocation<{}>\n"
				"from : {}.{:X}\n"
				"call : {:X}\n"
				"to   : {}.{:X}",
				OpSeqSize, GetProcessName(), Address, OriginalFunc, PROJECT_NAME, Destination);
		}

		void Enable() noexcept override
		{
			WriteData(Address, Detour.data(), Detour.size(), false);
			__DEBUG("DKU_H: Enabled relocation hook @ {:X}", Address);
		}

		void Disable() noexcept override
		{
			WriteData(Address, OldBytes.data(), OldBytes.size(), false);
			__DEBUG("DKU_H: Disabled relocation hook @ {:X}", Address);
		}

		template <typename F>
			requires(model::concepts::dku_memory<F>)
		constexpr operator F() const noexcept
		{
			return std::bit_cast<F>(OriginalFunc);
		}

		const std::uint8_t  OpSeqSize;
		const Imm64         OriginalFunc;
		Imm64               Destination;
		std::vector<OpCode> OldBytes{};
		std::vector<OpCode> Detour{};
	};

	/* @brief Relocate a call/jmp site with target hook function
	 * @param <N> : Length of source instruction
	 * @param <RETN> : Return or branch (call/jmp)
	 * @param a_src : Address of call/jmp instruction
	 * @param a_dst : Destination function
	 * @returns RelHookHandle
	 */
	template <std::size_t N, bool RETN>
	inline auto AddRelHook(
		std::uintptr_t a_src,
		std::uintptr_t a_dst)  // noexcept
	{
		static_assert(N == 5 || N == 6, "unsupported instruction size");
		using DetourAsm = std::conditional_t<N == 5, _BranchNear<RETN>, _BranchIndirect<RETN>>;

		auto tramPtr = TRAM_ALLOC(0);

		// tram entry
		WriteImm(tramPtr, a_dst, true);
		tramPtr += sizeof(a_dst);

		// handle
		auto handle = std::make_unique<RelHookHandle>(a_src, tramPtr, a_dst, N);
		handle->OldBytes.resize(N);
		std::memcpy(handle->OldBytes.data(), AsPointer(a_src), N);
		handle->Detour.resize(N, NOP);

		// detour
		DetourAsm      asmDetour{};
		std::ptrdiff_t disp = tramPtr - a_src - sizeof(asmDetour);
		if constexpr (N == 6) {
			disp -= sizeof(a_dst);
		}
		assert_trampoline_range(disp);

		asmDetour.Disp = static_cast<Disp32>(disp);
		AsMemCpy(handle->Detour.data(), asmDetour);

		if constexpr (N == 5) {
			// branch
			JmpRip asmBranch;
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));

			handle->Write(asmBranch);
		}

		return std::move(handle);
	}
}  // namespace DKUtil::Hook
