#pragma once


#include "Assembly.hpp"
#include "Shared.hpp"
#include "Trampoline.hpp"


namespace DKUtil::Hook
{
	using namespace Assembly;

	class HookHandle
	{
	public:
		virtual ~HookHandle() = default;


		virtual void Enable() noexcept = 0;
		virtual void Disable() noexcept = 0;


		const std::uintptr_t Address;
		const std::uintptr_t TramEntry;
		std::uintptr_t TramPtr{ 0x0 };


		template <std::derived_from<HookHandle> derived_t>
		constexpr derived_t* As() noexcept
		{
			return dynamic_cast<derived_t*>(this);
		}

	protected:
		HookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry) :
			Address(a_address), TramEntry(a_tramEntry), TramPtr(a_tramEntry)
		{}
	};


	class ASMPatchHandle : public HookHandle
	{
	public:
		// execution address, <cave low offset, cave high offset>
		ASMPatchHandle(
			const std::uintptr_t a_address,
			const offset_pair a_offset) noexcept :
			HookHandle(a_address, a_address + a_offset.first),
			Offset(a_offset), PatchSize(a_offset.second - a_offset.first)
		{
			std::memcpy(OldBytes, AsPointer(TramEntry), PatchSize);
			std::fill_n(PatchBuf, PatchSize, NOP);

			DEBUG("DKU_H: Patch capacity: {} bytes\nPatch entry @ {:X}", PatchSize, TramEntry);
		}


		void Enable() noexcept override
		{
			WriteData(TramEntry, PatchBuf, PatchSize);
			DEBUG("DKU_H: Enabled ASM patch"sv);
		}


		void Disable() noexcept override
		{
			WriteData(TramEntry, OldBytes, PatchSize);
			DEBUG("DKU_H: Disabled ASM patch"sv);
		}


		const offset_pair Offset;
		const std::size_t PatchSize;

		OpCode OldBytes[CAVE_BUF_SIZE]{};
		OpCode PatchBuf[CAVE_BUF_SIZE]{};
	};


	/* Apply assembly patch in the body of execution
	 * @param a_address : Memory address of the BEGINNING of target function
	 * @param a_offset : Offset pairs for <beginning, end> of cave entry from the head of function
	 * @param a_patch : Assembly patch
	 * @param a_forward : Skip the rest of NOPs until next valid opcode
	 * @returns ASMPatchHandle
	 */
	inline auto AddASMPatch(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const unpacked_data a_patch,
		const bool a_forward = true) noexcept
	{
		if (!a_address || !a_patch.first || !a_patch.second) {
			ERROR("DKU_H: Invalid ASM patch");
		}

		auto handle = std::make_unique<ASMPatchHandle>(a_address, a_offset);

		if (a_patch.second > (a_offset.second - a_offset.first)) {
			DEBUG("DKU_H: ASM patch size exceeds the patch capacity, enabled trampoline"sv);
			if ((a_offset.second - a_offset.first) < 0x5) {
				ERROR("DKU_H: ASM patch size exceeds the patch capacity & cannot fulfill the minimal trampoline requirement");
			}

			JmpRel asmDetour;  // cave -> tram
			JmpRel asmReturn;  // tram -> cave

			handle->TramPtr = TRAM_ALLOC(0);
			DEBUG("DKU_H: ASM patch tramoline entry -> {:X}", handle->TramPtr);

			asmDetour.Disp = static_cast<Imm32>(handle->TramPtr - handle->TramEntry - asmDetour.size());
			std::memcpy(handle->PatchBuf, asmDetour.data(), asmDetour.size());

			WriteData(handle->TramPtr, a_patch.first, a_patch.second);

			if (a_forward) {
				asmReturn.Disp = static_cast<Disp32>(handle->TramEntry + handle->PatchSize - handle->TramPtr - asmReturn.size());
			} else {
				asmReturn.Disp = static_cast<Disp32>(handle->TramEntry + a_patch.second - handle->TramPtr - asmReturn.size());
			}

			WriteData(handle->TramPtr, asmReturn.data(), asmReturn.size());
		} else {
			std::memcpy(handle->PatchBuf, a_patch.first, a_patch.second);

			if (a_forward && handle->PatchSize > (a_patch.second * ASM_MINIMUM_SKIP + sizeof(JmpRel))) {
				JmpRel asmForward;

				asmForward.Disp = static_cast<Disp32>(handle->TramEntry + handle->PatchSize - handle->TramEntry - a_patch.second - asmForward.size());
				std::memcpy(handle->PatchBuf + a_patch.second, asmForward.data(), asmForward.size());

				DEBUG("DKU_H: ASM patch forwarded"sv);
			}
		}

		return std::move(handle);
	}

	class CaveHookHandle : public HookHandle
	{
	public:
		// execution address, trampoline address, <cave low offset, cave high offset>
		CaveHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramPtr,
			const offset_pair a_offset) noexcept :
			HookHandle(a_address, a_tramPtr),
			Offset(a_offset), CaveSize(a_offset.second - a_offset.first), CaveEntry(Address + a_offset.first), CavePtr(Address + a_offset.first)
		{
			std::memcpy(OldBytes, AsPointer(CaveEntry), CaveSize);
			std::fill_n(CaveBuf, CaveSize, NOP);

			DEBUG("DKU_H: Cave capacity: {} bytes\nCave entry @ {:X} | Tram entry @ {:X}", CaveSize, CaveEntry, TramEntry);
		}


		void Enable() noexcept override
		{
			WriteData(CavePtr, CaveBuf, CaveSize, FORWARD_PTR, NO_ALLOC);
			DEBUG("DKU_H: Enabled cave hook"sv);
		}


		void Disable() noexcept override
		{
			CavePtr -= CaveSize;
			WriteData(CavePtr, OldBytes, CaveSize);
			DEBUG("DKU_H: Disabled cave hook"sv);
		}


		const offset_pair Offset;
		const std::size_t CaveSize;
		const std::uintptr_t CaveEntry;

		std::uintptr_t CavePtr{ 0x0 };

		OpCode OldBytes[CAVE_BUF_SIZE]{};
		OpCode CaveBuf[CAVE_BUF_SIZE]{};
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
	inline auto AddCaveHook(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const FuncInfo a_funcInfo,
		const unpacked_data a_prolog,
		const unpacked_data a_epilog,
		model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP) noexcept
	{
		if (a_offset.second - a_offset.first == 5) {
			a_flag.reset(HookFlag::kSkipNOP);
		}

		JmpRel asmDetour;  // cave -> tram
		JmpRel asmReturn;  // tram -> cave
		SubRsp asmSub;
		AddRsp asmAdd;
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

		WriteImm(tramPtr, a_funcInfo.Address);
		DEBUG("DKU_H: Detour {}.{:X} -> {} @ {}.{:X}", GetProcessName(), a_address + a_offset.first, a_funcInfo.Name.data(), PROJECT_NAME, a_funcInfo.Address);

		auto handle = std::make_unique<CaveHookHandle>(a_address, tramPtr, a_offset);

		asmDetour.Disp = static_cast<Disp32>(handle->TramPtr - handle->CavePtr - asmDetour.size());
		std::memcpy(handle->CaveBuf, asmDetour.data(), asmDetour.size());

		if (a_flag.any(HookFlag::kRestoreBeforeProlog)) {
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize);
			asmBranch.Disp -= static_cast<Disp32>(handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreBeforeProlog);
		}

		if (a_prolog.first && a_prolog.second) {
			WriteData(handle->TramPtr, a_prolog.first, a_prolog.second);
			asmBranch.Disp -= static_cast<Disp32>(a_prolog.second);
		}

		if (a_flag.any(HookFlag::kRestoreAfterProlog)) {
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize);
			asmBranch.Disp -= static_cast<Disp32>(handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreBeforeEpilog, HookFlag::kRestoreAfterEpilog);
		}

		const auto stackBufSize = sizeof(std::uint64_t) * a_funcInfo.ArgsCount;
		if (stackBufSize) {
			asmSub.Size = stackBufSize;
			asmAdd.Size = stackBufSize;

			WriteData(handle->TramPtr, asmSub.data(), asmSub.size());
			asmBranch.Disp -= static_cast<Disp32>(asmSub.size());
		}

		asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
		asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());
		WriteData(handle->TramPtr, asmBranch.data(), asmBranch.size());

		if (stackBufSize) {
			WriteData(handle->TramPtr, asmAdd.data(), asmAdd.size());
		}

		if (a_flag.any(HookFlag::kRestoreBeforeEpilog)) {
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize);

			a_flag.reset(HookFlag::kRestoreAfterEpilog);
		}

		if (a_epilog.first && a_epilog.second) {
			WriteData(handle->TramPtr, a_epilog.first, a_epilog.second);
		}

		if (a_flag.any(HookFlag::kRestoreAfterEpilog)) {
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize);
		}

		if (a_flag.any(HookFlag::kSkipNOP)) {
			asmReturn.Disp = static_cast<Disp32>(handle->Address + handle->Offset.second - handle->TramPtr - asmReturn.size());
		} else {
			asmReturn.Disp = static_cast<Disp32>(handle->CavePtr + asmDetour.size() - handle->TramPtr - asmReturn.size());
		}

		WriteData(handle->TramPtr, asmReturn.data(), asmReturn.size());

		return std::move(handle);
	}

	class VMTHookHandle : public HookHandle
	{
	public:
		// VTBL address, target func address, VTBL func index
		VMTHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramEntry,
			const std::uint16_t a_index) noexcept :
			HookHandle(TblToAbs(a_address, a_index), a_tramEntry),
			OldAddress(*std::bit_cast<std::uintptr_t*>(Address))
		{
			DEBUG("DKU_H: VMT @ {:X} [{}]\nOld entry @ {:X} | New entry @ {:X}", a_address, a_index, OldAddress, TramEntry);
		}


		void Enable() noexcept override
		{
			WriteImm(Address, TramEntry);
			DEBUG("DKU_H: Enabled VMT hook"sv);
		}


		void Disable() noexcept override
		{
			WriteImm(Address, OldAddress);
			DEBUG("DKU_H: Disabled VMT hook"sv);
		}


		const std::uintptr_t OldAddress;
	};


	/* Swaps a virtual method table function with target function
	 * @param a_vtbl : Pointer to virtual method table
	 * @param a_index : Index of the virtual function in the virtual method table
	 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
	 * @param a_patch : Prolog patch before detouring to target function
	 * @return VMTHookHandle
	 */
	inline auto AddVMTHook(
		const void* a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo a_funcInfo,
		const unpacked_data a_patch) noexcept
	{
		if (!a_funcInfo.Address) {
			ERROR("DKU_H: VMT hook must have a valid function pointer");
		}
		DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.Name.data(), PROJECT_NAME, a_funcInfo.Address);

		if (a_patch.first && a_patch.second) {
			auto tramPtr = TRAM_ALLOC(0);

			CallRip asmBranch;

			WriteImm(tramPtr, a_funcInfo.Address);
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

			auto handle = std::make_unique<VMTHookHandle>(*std::bit_cast<std::uintptr_t*>(a_vtbl), tramPtr, a_index);

			WriteData(tramPtr, a_patch.first, a_patch.second);
			asmBranch.Disp -= static_cast<Disp32>(a_patch.second);

			asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());
			WriteData(tramPtr, asmBranch.data(), asmBranch.size());

			return std::move(handle);
		} else {
			auto handle = std::make_unique<VMTHookHandle>(*std::bit_cast<std::uintptr_t*>(a_vtbl), a_funcInfo.Address, a_index);
			return std::move(handle);
		}
	}

	class IATHookHandle : public HookHandle
	{
	public:
		// IAT address, target func address, IAT func name, target func name
		IATHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramEntry,
			const char* a_methodName,
			const char* a_funcName) noexcept :
			HookHandle(a_address, a_tramEntry),
			OldAddress(*std::bit_cast<std::uintptr_t*>(Address))
		{
			DEBUG("DKU_H: IAT @ {:X}\nOld entry {} @ {:X} | New entry {} @ {}.{:X}", a_address, a_methodName, OldAddress, a_funcName, PROJECT_NAME, a_tramEntry);
		}


		void Enable() noexcept override
		{
			WriteImm(Address, TramEntry);
			DEBUG("DKU_H: Enabled IAT hook"sv);
		}


		void Disable() noexcept override
		{
			WriteImm(Address, OldAddress);
			DEBUG("DKU_H: Disabled IAT hook"sv);
		}


		const std::uintptr_t OldAddress;
	};


	/* Swaps a import address table method with target function
	 * @param a_moduleName : Name of the target module that import address table resides
	 * @param a_methodName : Name of the target method to be swapped
	 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
	 * @param a_patch : Prolog patch before detouring to target function
	 * @return IATHookHandle
	 */
	inline auto AddIATHook(
		const char* a_moduleName,
		const char* a_methodName,
		const FuncInfo a_funcInfo,
		const unpacked_data a_patch) noexcept
	{
		if (!a_moduleName || !a_methodName) {
			ERROR("DKU_H: IAT hook must have valid module name & method name\nConsider using GetProcessName([Opt]HMODULE)");
		}

		if (!a_funcInfo.Address) {
			ERROR("DKU_H: IAT hook must have a valid function pointer");
		}
		DEBUG("DKU_H: Detour {} @ {} -> {} @ {}.{:X}", a_methodName, a_moduleName, a_funcInfo.Name.data(), PROJECT_NAME, a_funcInfo.Address);

		auto base = AsAddress(GetModuleHandleA(a_moduleName));
		auto* dosHeader = std::bit_cast<const IMAGE_DOS_HEADER*>(base);
		auto* ntHeader = std::bit_cast<const IMAGE_NT_HEADERS64*>(dosHeader + dosHeader->e_lfanew);
		auto* importDesc = std::bit_cast<const IMAGE_IMPORT_DESCRIPTOR*>(dosHeader + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		for (auto index = 0; importDesc[index].Characteristics != 0; ++index) {
			const char* moduleName = std::bit_cast<const char*>(dosHeader + importDesc[index].Name);
			if (!string::iequals(a_moduleName, moduleName)) {
				continue;
			}

			if (!importDesc[index].FirstThunk || !importDesc[index].OriginalFirstThunk) {
				ERROR("DKU_H: IAT read invalid thunk pointer");
			}

			auto* thunk = std::bit_cast<const IMAGE_THUNK_DATA*>(dosHeader + importDesc[index].FirstThunk);
			auto* oldThunk = std::bit_cast<const IMAGE_THUNK_DATA*>(dosHeader + importDesc[index].OriginalFirstThunk);

			for (void(0); thunk->u1.Function; ++oldThunk, ++thunk) {
				if (oldThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
					continue;
				}

				auto* tbl = std::bit_cast<IMAGE_IMPORT_BY_NAME*>(dosHeader + oldThunk->u1.AddressOfData);

				if (!string::iequals(a_methodName, std::bit_cast<const char*>(std::addressof(tbl->Name[0])))) {
					continue;
				}

				if (a_patch.first && a_patch.second) {
					auto tramPtr = TRAM_ALLOC(0);

					CallRip asmBranch;

					WriteImm(tramPtr, a_funcInfo.Address);
					asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

					auto handle = std::make_unique<IATHookHandle>(*std::bit_cast<std::uintptr_t*>(thunk->u1.Function), tramPtr, a_methodName, a_funcInfo.Name.data());

					WriteData(tramPtr, a_patch.first, a_patch.second);
					asmBranch.Disp -= static_cast<Disp32>(a_patch.second);

					asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());
					WriteData(tramPtr, asmBranch.data(), asmBranch.size());

					return std::move(handle);
				} else {
					auto handle = std::make_unique<IATHookHandle>(*std::bit_cast<std::uintptr_t*>(thunk->u1.Function), a_funcInfo.Address, a_methodName, a_funcInfo.Name.data());
					return std::move(handle);
				}
			}
		}

		ERROR("DKU_H: IAT reached the end of table\n\nMethod {} not found", a_methodName);
	}
}  // namespace DKUtil::Hook
