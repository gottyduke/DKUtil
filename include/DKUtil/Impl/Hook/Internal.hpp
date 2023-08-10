#pragma once


#include "assembly.hpp"
#include "jit.hpp"
#include "trampoline.hpp"


#if defined(SKSEAPI)

// CommonLib - DKUtil should only be integrated into project using the designated SKSEPlugins/F4SEPlugins workspace
#	if defined(F4SEAPI)
#		include "F4SE/API.h"
#	elif defined(SKSEAPI)
#		include "SKSE/API.h"
#		define IS_AE REL::Module::IsAE()
#		define IS_SE REL::Module::IsSE()
#		define IS_VR REL::Module::IsVR()

#	else
#		error "Neither CommonLib nor custom TRAMPOLINE defined"
#	endif

#	define TRAMPOLINE SKSE::GetTrampoline()
#	define TRAM_ALLOC(SIZE) AsAddress((TRAMPOLINE).allocate((SIZE)))
#	define PAGE_ALLOC(SIZE) SKSE::AllocTrampoline((SIZE))

#elif defined(F4SEAPI)
#elif defined(PLUGIN_MODE)
#	define TRAMPOLINE Trampoline::GetTrampoline()
#	define TRAM_ALLOC(SIZE) AsAddress((TRAMPOLINE).allocate((SIZE)))
#	define PAGE_ALLOC(SIZE) Trampoline::AllocTrampoline((SIZE))
#endif


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
			WriteData(TramEntry, PatchBuf, PatchSize, false);
			DEBUG("DKU_H: Enabled ASM patch");
		}


		void Disable() noexcept override
		{
			WriteData(TramEntry, OldBytes, PatchSize, false);
			DEBUG("DKU_H: Disabled ASM patch");
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
	[[nodiscard]] inline auto AddASMPatch(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const unpacked_data a_patch = std::make_pair(nullptr, 0),
		const bool a_forward = true) noexcept
	{
		if (!a_address || !a_patch.first || !a_patch.second) {
			ERROR("DKU_H: Invalid ASM patch");
		}

		auto handle = std::make_unique<ASMPatchHandle>(a_address, a_offset);

		if (a_patch.second > (a_offset.second - a_offset.first)) {
			DEBUG("DKU_H: ASM patch size exceeds the patch capacity, enabled trampoline");
			if ((a_offset.second - a_offset.first) < sizeof(JmpRel)) {
				ERROR("DKU_H: ASM patch size exceeds the patch capacity & cannot fulfill the minimal trampoline requirement");
			}

			JmpRel asmDetour;  // cave -> tram
			JmpRel asmReturn;  // tram -> cave

			handle->TramPtr = TRAM_ALLOC(0);
			DEBUG("DKU_H: ASM patch tramoline entry -> {:X}", handle->TramPtr);

			asmDetour.Disp = static_cast<Imm32>(handle->TramPtr - handle->TramEntry - asmDetour.size());
			std::memcpy(handle->PatchBuf, asmDetour.data(), asmDetour.size());

			WriteData(handle->TramPtr, a_patch.first, a_patch.second, true);
			handle->TramPtr += a_patch.second;

			if (a_forward) {
				asmReturn.Disp = static_cast<Disp32>(handle->TramEntry + handle->PatchSize - handle->TramPtr - asmReturn.size());
			} else {
				asmReturn.Disp = static_cast<Disp32>(handle->TramEntry + a_patch.second - handle->TramPtr - asmReturn.size());
			}

			WriteData(handle->TramPtr, asmReturn.data(), asmReturn.size(), true);
		} else {
			std::memcpy(handle->PatchBuf, a_patch.first, a_patch.second);

			if (a_forward && handle->PatchSize > (a_patch.second * ASM_MINIMUM_SKIP + sizeof(JmpRel))) {
				JmpRel asmForward;

				asmForward.Disp = static_cast<Disp32>(handle->TramEntry + handle->PatchSize - handle->TramEntry - a_patch.second - asmForward.size());
				std::memcpy(handle->PatchBuf + a_patch.second, asmForward.data(), asmForward.size());

				DEBUG("DKU_H: ASM patch forwarded");
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

			DEBUG(
				"DKU_H: Cave capacity: {} bytes\n"
				"cave entry : 0x{:X}\n"
				"tram entry : 0x{:X}",
				CaveSize, CaveEntry, TramEntry);
		}


		void Enable() noexcept override
		{
			WriteData(CavePtr, CaveBuf, CaveSize, false);
			CavePtr += CaveSize;
			DEBUG("DKU_H: Enabled cave hook");
		}


		void Disable() noexcept override
		{
			WriteData(CavePtr - CaveSize, OldBytes, CaveSize, false);
			CavePtr -= CaveSize;
			DEBUG("DKU_H: Disabled cave hook");
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
	[[nodiscard]] inline auto AddCaveHook(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const FuncInfo a_funcInfo,
		const unpacked_data a_prolog = std::make_pair(nullptr, 0),
		const unpacked_data a_epilog = std::make_pair(nullptr, 0),
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

		WriteImm(tramPtr, a_funcInfo.address(), true);
		tramPtr += sizeof(a_funcInfo.address());
		DEBUG(
			"DKU_H: Detouring...\n"
			"from : {}.{:X}\n"
			"to   : {} @ {}.{:X}",
			GetProcessName(), a_address + a_offset.first, a_funcInfo.name(), PROJECT_NAME, a_funcInfo.address());

		auto handle = std::make_unique<CaveHookHandle>(a_address, tramPtr, a_offset);

		asmDetour.Disp = static_cast<Disp32>(handle->TramPtr - handle->CavePtr - asmDetour.size());
		std::memcpy(handle->CaveBuf, asmDetour.data(), asmDetour.size());

		if (a_flag.any(HookFlag::kRestoreBeforeProlog)) {
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize, true);
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
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize, true);
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
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize, true);
			handle->TramPtr += handle->CaveSize;

			a_flag.reset(HookFlag::kRestoreAfterEpilog);
		}

		if (a_epilog.first && a_epilog.second) {
			WriteData(handle->TramPtr, a_epilog.first, a_epilog.second, true);
			handle->TramPtr += a_epilog.second;
		}

		if (a_flag.any(HookFlag::kRestoreAfterEpilog)) {
			WriteData(handle->TramPtr, handle->OldBytes, handle->CaveSize, true);
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
			WriteImm(Address, TramEntry, false);
			DEBUG("DKU_H: Enabled VMT hook");
		}


		void Disable() noexcept override
		{
			WriteImm(Address, OldAddress, false);
			DEBUG("DKU_H: Disabled VMT hook");
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
	[[nodiscard]] inline auto AddVMTHook(
		const void* a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo a_funcInfo,
		const unpacked_data a_patch = std::make_pair(nullptr, 0)) noexcept
	{
		if (!a_funcInfo.address()) {
			ERROR("DKU_H: VMT hook must have a valid function pointer");
		}
		DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.name().data(), PROJECT_NAME, a_funcInfo.address());

		if (a_patch.first && a_patch.second) {
			auto tramPtr = TRAM_ALLOC(0);

			CallRip asmBranch;

			WriteImm(tramPtr, a_funcInfo.address(), true);
			tramPtr += sizeof(a_funcInfo.address());
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

			auto handle = std::make_unique<VMTHookHandle>(*std::bit_cast<std::uintptr_t*>(a_vtbl), tramPtr, a_index);

			WriteData(tramPtr, a_patch.first, a_patch.second, true);
			tramPtr += a_patch.second;
			asmBranch.Disp -= static_cast<Disp32>(a_patch.second);

			asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());
			WriteData(tramPtr, asmBranch.data(), asmBranch.size(), true);
			tramPtr += asmBranch.size();

			return std::move(handle);
		} else {
			auto handle = std::make_unique<VMTHookHandle>(*std::bit_cast<std::uintptr_t*>(a_vtbl), a_funcInfo.address(), a_index);
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
			std::string_view a_importName,
			std::string_view a_funcName) noexcept :
			HookHandle(a_address, a_tramEntry),
			OldAddress(*dku::Hook::unrestricted_cast<std::uintptr_t*>(Address))
		{
			DEBUG(
				"DKU_H: IAT @ {:X}\n"
				"old : {} @ {:X}\n"
				"new : {} @ {}.{:X}",
				a_address, a_importName, OldAddress, a_funcName, PROJECT_NAME, a_tramEntry);
		}


		void Enable() noexcept override
		{
			WriteImm(Address, TramEntry, false);
			DEBUG("DKU_H: Enabled IAT hook");
		}


		void Disable() noexcept override
		{
			WriteImm(Address, OldAddress, false);
			DEBUG("DKU_H: Disabled IAT hook");
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
	[[nodiscard]] inline auto AddIATHook(
		std::string_view a_moduleName,
		std::string_view a_libraryName,
		std::string_view a_importName,
		const FuncInfo a_funcInfo,
		const unpacked_data a_patch = std::make_pair(nullptr, 0)) noexcept
	{
		const auto iat = AsAddress(GetImportAddress(a_moduleName, a_libraryName, a_importName));

		if (a_patch.first && a_patch.second) {
			auto tramPtr = TRAM_ALLOC(0);

			CallRip asmBranch;

			WriteImm(tramPtr, a_funcInfo.address(), true);
			tramPtr += sizeof(a_funcInfo.address());
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

			auto handle = std::make_unique<IATHookHandle>(iat, tramPtr, a_importName, a_funcInfo.name().data());

			WriteData(tramPtr, a_patch.first, a_patch.second, true);
			tramPtr += a_patch.second;
			asmBranch.Disp -= static_cast<Disp32>(a_patch.second);

			asmBranch.Disp -= static_cast<Disp32>(asmBranch.size());
			WriteData(tramPtr, asmBranch.data(), asmBranch.size(), true);
			tramPtr += asmBranch.size();

			return std::move(handle);
		} else {
			auto handle = std::make_unique<IATHookHandle>(iat, a_funcInfo.address(), a_importName, a_funcInfo.name().data());
			return std::move(handle);
		}
		ERROR("DKU_H: IAT reached the end of table\n\nMethod {} not found", a_importName);
	}
}  // namespace DKUtil::Hook
