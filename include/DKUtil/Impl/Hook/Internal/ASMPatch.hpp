#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{
	class ASMPatchHandle : public HookHandle
	{
	public:
		// execution address, <cave low offset, cave high offset>
		ASMPatchHandle(
			const std::uintptr_t a_address,
			const offset_pair    a_offset) noexcept :
			HookHandle(a_address, a_address + a_offset.first),
			Offset(a_offset), PatchSize(a_offset.second - a_offset.first)
		{
			OldBytes.resize(PatchSize);
			PatchBuf.resize(PatchSize);
			std::memcpy(OldBytes.data(), AsPointer(TramEntry), PatchSize);
			std::ranges::fill(PatchBuf, NOP);

			DEBUG("DKU_H: Patch capacity: {} bytes\nPatch entry @ {:X}", PatchSize, TramEntry);
		}

		void Enable() noexcept override
		{
			WriteData(TramEntry, PatchBuf.data(), PatchSize, false);
			DEBUG("DKU_H: Enabled ASM patch");
		}

		void Disable() noexcept override
		{
			WriteData(TramEntry, OldBytes.data(), PatchSize, false);
			DEBUG("DKU_H: Disabled ASM patch");
		}

		const offset_pair Offset;
		const std::size_t PatchSize;

		std::vector<OpCode> OldBytes{};
		std::vector<OpCode> PatchBuf{};
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
		const offset_pair    a_offset,
		const unpacked_data  a_patch = std::make_pair(nullptr, 0),
		const bool           a_forward = true) noexcept
	{
		dku_assert(a_address && a_patch.first && a_patch.second,
			"DKU_H: Invalid ASM patch");

		auto handle = std::make_unique<ASMPatchHandle>(a_address, a_offset);

		if (a_patch.second > (a_offset.second - a_offset.first)) {
			DEBUG("DKU_H: ASM patch size exceeds the patch capacity, enabled trampoline");
			dku_assert((a_offset.second - a_offset.first) >= sizeof(JmpRel),
				"DKU_H: ASM patch size exceeds the patch capacity & cannot fulfill the minimal trampoline requirement");

			JmpRel asmDetour;  // cave -> tram
			JmpRel asmReturn;  // tram -> cave

			handle->TramPtr = TRAM_ALLOC(0);
			DEBUG("DKU_H: ASM patch tramoline entry -> {:X}", handle->TramPtr);

			std::ptrdiff_t disp = handle->TramPtr - handle->TramEntry - asmDetour.size();
			assert_trampoline_range(disp);

			asmDetour.Disp = static_cast<Disp32>(disp);
			std::memcpy(handle->PatchBuf.data(), asmDetour.data(), asmDetour.size());

			WriteData(handle->TramPtr, a_patch.first, a_patch.second, true);
			handle->TramPtr += a_patch.second;

			if (a_forward) {
				asmReturn.Disp = static_cast<Disp32>(handle->TramEntry + handle->PatchSize - handle->TramPtr - asmReturn.size());
			} else {
				asmReturn.Disp = static_cast<Disp32>(handle->TramEntry + a_patch.second - handle->TramPtr - asmReturn.size());
			}

			WriteData(handle->TramPtr, asmReturn.data(), asmReturn.size(), true);
		} else {
			std::memcpy(handle->PatchBuf.data(), a_patch.first, a_patch.second);

			if (a_forward && handle->PatchSize > (a_patch.second * ASM_MINIMUM_SKIP + sizeof(JmpRel))) {
				JmpRel asmForward;

				asmForward.Disp = static_cast<Disp32>(handle->TramEntry + handle->PatchSize - handle->TramEntry - a_patch.second - asmForward.size());
				std::memcpy(handle->PatchBuf.data() + a_patch.second, asmForward.data(), asmForward.size());

				DEBUG("DKU_H: ASM patch forwarded");
			}
		}

		return std::move(handle);
	}
}  // namespace DKUtil::Hook
