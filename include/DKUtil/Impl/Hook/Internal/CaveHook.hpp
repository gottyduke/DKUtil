#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{
	class CaveHookHandle : public HookHandle
	{
	public:
		// execution address, trampoline address, <cave low offset, cave high offset>
		CaveHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramPtr,
			const offset_pair    a_offset) noexcept :
			HookHandle(a_address, a_tramPtr),
			Offset(a_offset), CaveSize(a_offset.second - a_offset.first), CaveEntry(Address + a_offset.first), CavePtr(Address + a_offset.first)
		{
			OldBytes.resize(CaveSize);
			CaveBuf.resize(CaveSize);
			std::memcpy(OldBytes.data(), AsPointer(CaveEntry), CaveSize);
			std::ranges::fill(CaveBuf, NOP);

			__DEBUG(
				"DKU_H: Cave capacity: {} bytes\n"
				"cave entry : 0x{:X}\n"
				"tram entry : 0x{:X}",
				CaveSize, CaveEntry, TramEntry);
		}

		void Enable() noexcept override
		{
			WriteData(CavePtr, CaveBuf.data(), CaveSize, false);
			CavePtr += CaveSize;
			__DEBUG("DKU_H: Enabled cave hook");
		}

		void Disable() noexcept override
		{
			WriteData(CavePtr - CaveSize, OldBytes.data(), CaveSize, false);
			CavePtr -= CaveSize;
			__DEBUG("DKU_H: Disabled cave hook");
		}

		template <typename F = std::uintptr_t>
		F ReDisp(const std::ptrdiff_t a_offset = 0) noexcept
		{
			Disp32 rel{ 0 };
			if (a_offset <= CaveSize - sizeof(Assembly::JmpRel)) {
				rel = *adjust_pointer<Disp32>(OldBytes.data(), a_offset + sizeof(OpCode));
				rel += sizeof(Assembly::JmpRel);
			}

			return std::bit_cast<F>(CaveEntry + a_offset + rel);
		}

		const offset_pair    Offset;
		const std::size_t    CaveSize;
		const std::uintptr_t CaveEntry;
		std::uintptr_t       CavePtr{ 0x0 };
		std::vector<OpCode>  OldBytes{};
		std::vector<OpCode>  CaveBuf{};
	};

	/* Branch to hook function in the body of execution from target function.
	 * If stack manipulation is involved in epilog patch, add stack offset (sizeof(std::uintptr_t) * (target function argument(s) count))
	 * @param a_offset : Offset pairs for <beginning, end> of cave entry from the head of function
	 * @param a_address : Memory address of the BEGINNING of target function
	 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapper of hook function
	 * @param a_prolog : Prolog patch before detouring to hook function
	 * @param a_epilog : Epilog patch after returning from hook function
	 * @param a_flag : Specifies operation on cave hook
	 * @returns CaveHookHandle
	 */
	[[nodiscard]] inline auto AddCaveHook(
		const std::uintptr_t         a_address,
		const offset_pair            a_offset,
		const FuncInfo               a_funcInfo,
		const unpacked_data          a_prolog = std::make_pair(nullptr, 0),
		const unpacked_data          a_epilog = std::make_pair(nullptr, 0),
		model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP) noexcept
	{
		if (a_offset.second - a_offset.first == 5) {
			a_flag.reset(HookFlag::kSkipNOP);
		}

		JmpRel  asmDetour;  // cave -> tram
		JmpRel  asmReturn;  // tram -> cave
		SubRsp  asmSub;
		AddRsp  asmAdd;
		CallRip asmBranch;

		// trampoline layout
		// [qword imm64] <- tram entry after this
		// [stolen] <- kRestoreBeforeProlog
		// [prolog] <- cave detour entry
		// [stolen] <- kRestoreAfterProlog
		// [alloc stack]
		// [call qword ptr [rip + disp]]
		// [dealloc stack]
		// [stolen] <- kRestoreBeforeEpilog
		// [epilog]
		// [stolen] <- kRestoreAfterEpilog
		// [jmp rel32]
		auto tramPtr = TRAM_ALLOC(0);

		WriteImm(tramPtr, a_funcInfo.address(), true);
		tramPtr += sizeof(a_funcInfo.address());
		__DEBUG(
			"DKU_H: Detouring...\n"
			"from : {}.{:X}\n"
			"to   : {} @ {}.{:X}",
			GetProcessName(), a_address + a_offset.first, a_funcInfo.name(), PROJECT_NAME, a_funcInfo.address());

		auto handle = std::make_unique<CaveHookHandle>(a_address, tramPtr, a_offset);

		std::ptrdiff_t disp = handle->TramPtr - handle->CavePtr - asmDetour.size();
		assert_trampoline_range(disp);

		asmDetour.Disp = static_cast<Disp32>(disp);
		std::memcpy(handle->CaveBuf.data(), asmDetour.data(), asmDetour.size());

		if (a_flag.any(HookFlag::kRestoreBeforeProlog)) {
			WriteData(handle->TramPtr, handle->OldBytes.data(), handle->CaveSize, true);
			handle->TramPtr += handle->CaveSize;
			asmBranch.Disp -= static_cast<Disp32>(handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreBeforeProlog);
		}

		if (a_prolog.first && a_prolog.second) {
			WriteData(handle->TramPtr, a_prolog.first, a_prolog.second, true);
			handle->TramPtr += a_prolog.second;
			asmBranch.Disp -= static_cast<Disp32>(a_prolog.second);
		}

		if (a_flag.any(HookFlag::kRestoreAfterProlog)) {
			WriteData(handle->TramPtr, handle->OldBytes.data(), handle->CaveSize, true);
			handle->TramPtr += handle->CaveSize;
			asmBranch.Disp -= static_cast<Disp32>(handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreBeforeEpilog, HookFlag::kRestoreAfterEpilog);
		}

		// alloc stack space
		asmSub.Size = ASM_STACK_ALLOC_SIZE;
		asmAdd.Size = ASM_STACK_ALLOC_SIZE;

		WriteData(handle->TramPtr, asmSub.data(), asmSub.size(), true);
		handle->TramPtr += asmSub.size();
		asmBranch.Disp -= static_cast<Disp32>(asmSub.size());

		// write call
		asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
		asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());
		WriteData(handle->TramPtr, asmBranch.data(), asmBranch.size(), true);
		handle->TramPtr += asmBranch.size();

		// dealloc stack space
		WriteData(handle->TramPtr, asmAdd.data(), asmAdd.size(), true);
		handle->TramPtr += asmAdd.size();

		if (a_flag.any(HookFlag::kRestoreBeforeEpilog)) {
			WriteData(handle->TramPtr, handle->OldBytes.data(), handle->CaveSize, true);
			handle->TramPtr += handle->CaveSize;

			a_flag.reset(HookFlag::kRestoreAfterEpilog);
		}

		if (a_epilog.first && a_epilog.second) {
			WriteData(handle->TramPtr, a_epilog.first, a_epilog.second, true);
			handle->TramPtr += a_epilog.second;
		}

		if (a_flag.any(HookFlag::kRestoreAfterEpilog)) {
			WriteData(handle->TramPtr, handle->OldBytes.data(), handle->CaveSize, true);
			handle->TramPtr += handle->CaveSize;
		}

		if (a_flag.any(HookFlag::kSkipNOP)) {
			asmReturn.Disp = static_cast<Disp32>(handle->Address + handle->Offset.second - handle->TramPtr - asmReturn.size());
		} else {
			asmReturn.Disp = static_cast<Disp32>(handle->CavePtr + asmDetour.size() - handle->TramPtr - asmReturn.size());
		}

		WriteData(handle->TramPtr, asmReturn.data(), asmReturn.size(), true);
		handle->TramPtr += asmReturn.size();

		return std::move(handle);
	}
}  // namespace DKUtil::Hook
