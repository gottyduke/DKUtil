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
			CaveBuf.resize(CaveSize, NOP);
			std::memcpy(OldBytes.data(), AsPointer(CaveEntry), CaveSize);

			__DEBUG(
				"DKU_H: Cave capacity: {} bytes\n"
				"cave entry : {:X}\n"
				"tram entry : {:X}",
				CaveSize, CaveEntry, TramEntry);
		}

		void Enable() noexcept override
		{
			WriteData(CavePtr, CaveBuf.data(), CaveSize, false);
			CavePtr += CaveSize;
			__DEBUG("DKU_H: Enabled cave hook @ {:X}", CaveEntry);
		}

		void Disable() noexcept override
		{
			WriteData(CavePtr - CaveSize, OldBytes.data(), CaveSize, false);
			CavePtr -= CaveSize;
			__DEBUG("DKU_H: Disabled cave hook @ {:X}", CaveEntry);
		}

		const offset_pair    Offset;
		const std::size_t    CaveSize;
		const std::uintptr_t CaveEntry;
		std::uintptr_t       CavePtr{ 0x0 };
		std::vector<OpCode>  OldBytes{};
		std::vector<OpCode>  CaveBuf{};
	};

	/* @brief Branch to hook function in the body of execution from target function.
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

		// tram entry
		WriteImm(tramPtr, a_funcInfo.address(), true);
		tramPtr += sizeof(a_funcInfo.address());
		__DEBUG(
			"DKU_H: Detouring...\n"
			"from : {}.{:X}\n"
			"to   : {} @ {}.{:X}",
			GetModuleName(), a_address + a_offset.first, a_funcInfo.name(), PROJECT_NAME, a_funcInfo.address());

		auto handle = std::make_unique<CaveHookHandle>(a_address, tramPtr, a_offset);

		std::ptrdiff_t disp = handle->TramPtr - handle->CavePtr - sizeof(asmDetour);
		assert_trampoline_range(disp);

		asmDetour.Disp = static_cast<Disp32>(disp);
		AsMemCpy(handle->CaveBuf.data(), asmDetour);

		if (a_flag.any(HookFlag::kRestoreBeforeProlog)) {
			handle->Write(handle->OldBytes.data(), handle->CaveSize);
			asmBranch.Disp -= static_cast<Disp32>(handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreBeforeProlog);
		}

		if (a_prolog.first && a_prolog.second) {
			handle->Write(a_prolog.first, a_prolog.second);
			asmBranch.Disp -= static_cast<Disp32>(a_prolog.second);
		}

		if (a_flag.any(HookFlag::kRestoreAfterProlog)) {
			handle->Write(handle->OldBytes.data(), handle->CaveSize);
			asmBranch.Disp -= static_cast<Disp32>(handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreBeforeEpilog, HookFlag::kRestoreAfterEpilog);
		}

		// alloc stack space
		asmSub.Size = ASM_STACK_ALLOC_SIZE;
		asmAdd.Size = ASM_STACK_ALLOC_SIZE;

		handle->Write(asmSub);
		asmBranch.Disp -= static_cast<Disp32>(sizeof(asmSub));

		// write call
		asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
		asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));
		handle->Write(asmBranch);

		// dealloc stack space
		handle->Write(asmAdd);

		if (a_flag.any(HookFlag::kRestoreBeforeEpilog)) {
			handle->Write(handle->OldBytes.data(), handle->CaveSize);
			a_flag.reset(HookFlag::kRestoreAfterEpilog);
		}

		if (a_epilog.first && a_epilog.second) {
			handle->Write(a_epilog.first, a_epilog.second);
		}

		if (a_flag.any(HookFlag::kRestoreAfterEpilog)) {
			handle->Write(handle->OldBytes.data(), handle->CaveSize);
		}

		if (a_flag.any(HookFlag::kSkipNOP)) {
			asmReturn.Disp = static_cast<Disp32>(handle->Address + handle->Offset.second - handle->TramPtr - sizeof(asmReturn));
		} else {
			asmReturn.Disp = static_cast<Disp32>(handle->CavePtr + sizeof(asmDetour) - handle->TramPtr - sizeof(asmReturn));
		}

		handle->Write(asmReturn);

		return std::move(handle);
	}
}  // namespace DKUtil::Hook
