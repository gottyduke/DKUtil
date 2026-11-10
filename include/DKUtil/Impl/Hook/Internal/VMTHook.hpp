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
			const auto current = *std::bit_cast<std::uintptr_t*>(Address);

			if (!_enabled && current != TramEntry) {
				OldAddress = current;
			}

			WriteImm(Address, TramEntry, false);
			_enabled = true;
			__DEBUG("DKU_H: Enabled VMT hook");
		}

		void Disable() noexcept override
		{
			if (!_enabled) {
				return;
			}

			const auto current = *std::bit_cast<std::uintptr_t*>(Address);

			if (current == OldAddress) {
				_enabled = false;
				return;
			}

			if (current != TramEntry) {
				WARN("DKU_H: VMT slot @ {:X} was swapped after this hook, disable the later hook first", Address);
				return;
			}

			WriteImm(Address, OldAddress, false);
			_enabled = false;
			__DEBUG("DKU_H: Disabled VMT hook");
		}

		template <typename F>
		F GetOldFunction() noexcept
		{
			return std::bit_cast<F>(OldAddress);
		}

		std::uintptr_t OldAddress;

	private:
		bool _enabled{ false };
	};

	/** Swaps a virtual method table function with target function
	 * \param a_vtbl : Pointer to virtual method table (base address of class object)
	 * \param a_index : Index of the virtual function in the virtual method table
	 * \param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
	 * \param a_patch : Prolog patch before detouring to target function
	 * @return VMTHookHandle
	 */
	[[nodiscard]] inline auto AddVMTHook(
		const void*         a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo      a_funcInfo,
		const unpacked_data a_patch = std::make_pair(nullptr, 0)) noexcept
	{
		dku_assert(a_funcInfo.address(), "DKU_H: VMT hook must have a valid function pointer");
		dku_assert(a_vtbl, "DKU_H: VMT hook must have a valid class object");
		__DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.name().data(), PROJECT_NAME, a_funcInfo.address());

		const auto vtbl = *std::bit_cast<std::uintptr_t*>(a_vtbl);
		const auto slot = TblToAbs(vtbl, a_index);
		const auto entry = *std::bit_cast<std::uintptr_t*>(slot);

		const auto query = [](const std::uintptr_t a_address) {
			::MEMORY_BASIC_INFORMATION mbi{};
			::VirtualQuery(AsPointer(a_address), &mbi, sizeof(mbi));
			return mbi;
		};
		constexpr auto executable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

		const auto slotInfo = query(slot);
		dku_assert(slotInfo.Type != MEM_IMAGE || !(slotInfo.Protect & executable) || (query(entry).Protect & executable),
			"DKU_H: VMT slot [{}] @ {:X} is inside code and holds {:X}\n"
			"Pass a pointer to class object instead of the table itself",
			a_index, slot, entry);

		if (a_patch.first && a_patch.second) {
			auto tramPtr = TRAM_ALLOC(0);

			JmpRip asmBranch;

			WriteImm(tramPtr, a_funcInfo.address(), true);
			tramPtr += sizeof(a_funcInfo.address());
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

			auto handle = std::make_unique<VMTHookHandle>(vtbl, tramPtr, a_index);

			handle->Write(a_patch.first, a_patch.second);
			asmBranch.Disp -= static_cast<Disp32>(a_patch.second);

			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));
			handle->Write(asmBranch);

			return std::move(handle);
		} else {
			auto handle = std::make_unique<VMTHookHandle>(vtbl, a_funcInfo.address(), a_index);
			handle->TramPtr = 0;
			return std::move(handle);
		}
	}
}  // namespace DKUtil::Hook
