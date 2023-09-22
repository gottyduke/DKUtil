#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{
	class IATHookHandle : public HookHandle
	{
	public:
		// IAT address, target func address, IAT func name, target func name
		IATHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramEntry,
			std::string_view     a_importName,
			std::string_view     a_funcName) noexcept :
			HookHandle(a_address, a_tramEntry),
			OldAddress(*dku::Hook::unrestricted_cast<std::uintptr_t*>(Address))
		{
			__DEBUG(
				"DKU_H: IAT @ {:X}\n"
				"old : {} @ {:X}\n"
				"new : {} @ {}.{:X}",
				a_address, a_importName, OldAddress, a_funcName, PROJECT_NAME, a_tramEntry);
		}

		void Enable() noexcept override
		{
			WriteImm(Address, TramEntry, false);
			__DEBUG("DKU_H: Enabled IAT hook");
		}

		void Disable() noexcept override
		{
			WriteImm(Address, OldAddress, false);
			__DEBUG("DKU_H: Disabled IAT hook");
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
		std::string_view    a_moduleName,
		std::string_view    a_libraryName,
		std::string_view    a_importName,
		const FuncInfo      a_funcInfo,
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

			handle->Write(a_patch.first, a_patch.second);
			asmBranch.Disp -= static_cast<Disp32>(a_patch.second);

			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));
			handle->Write(asmBranch);

			return std::move(handle);
		} else {
			auto handle = std::make_unique<IATHookHandle>(iat, a_funcInfo.address(), a_importName, a_funcInfo.name().data());
			return std::move(handle);
		}

		FATAL("DKU_H: IAT reached the end of table\n\nMethod {} not found", a_importName);
	}
}  // namespace DKUtil::Hook
