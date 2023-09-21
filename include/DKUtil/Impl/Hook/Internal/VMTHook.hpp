#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{

	class VMTHookHandle : public HookHandle
	{
	public:
		// VTBL address, target func address, VTBL func index
		VMTHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramEntry,
			const std::uint16_t  a_index) noexcept :
			HookHandle(TblToAbs(a_address, a_index), a_tramEntry),
			OldAddress(*std::bit_cast<std::uintptr_t*>(Address))
		{
			__DEBUG("DKU_H: VMT @ {:X} [{}]\nOld entry @ {:X} | New entry @ {:X}", a_address, a_index, OldAddress, TramEntry);
		}

		void Enable() noexcept override
		{
			WriteImm(Address, TramEntry, false);
			__DEBUG("DKU_H: Enabled VMT hook");
		}

		void Disable() noexcept override
		{
			WriteImm(Address, OldAddress, false);
			__DEBUG("DKU_H: Disabled VMT hook");
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
		const void*         a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo      a_funcInfo,
		const unpacked_data a_patch = std::make_pair(nullptr, 0)) noexcept
	{
		if (!a_funcInfo.address()) {
			ERROR("DKU_H: VMT hook must have a valid function pointer");
		}
		__DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.name().data(), PROJECT_NAME, a_funcInfo.address());

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
}  // namespace DKUtil::Hook
