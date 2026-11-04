#pragma once

#if !defined(DKU_H_INTERNAL_IMPORTS)
#	error Incorrect DKUtil::Hook internal import order.
#endif

namespace DKUtil::Hook
{
	class DetourHookHandle : public HookHandle
	{
	public:
		// function address, trampoline entry, hook function address, stolen prologue size
		DetourHookHandle(
			const std::uintptr_t a_address,
			const std::uintptr_t a_tramEntry,
			const std::uintptr_t a_dstAddr,
			const std::size_t    a_stolenSize) noexcept :
			HookHandle(a_address, a_tramEntry),
			StolenSize(a_stolenSize),
			OriginalFunc(a_tramEntry + sizeof(JmpRip)),
			Destination(a_dstAddr)
		{
			OldBytes.resize(StolenSize);
			Detour.resize(StolenSize, NOP);
			std::memcpy(OldBytes.data(), AsPointer(Address), StolenSize);

			__DEBUG(
				"DKU_H: Detour<{}>\n"
				"from : {}.{:X}\n"
				"orig : {:X}\n"
				"to   : {}.{:X}\n"
				"stole: {}",
				StolenSize, GetModuleName(), Address, OriginalFunc, PROJECT_NAME, Destination,
				[this] {
					std::string bytes;
					for (const auto byte : OldBytes) {
						bytes += fmt::format("{}{:02X}", bytes.empty() ? "" : " ", byte);
					}
					return bytes;
				}());
		}

		void Enable() noexcept override
		{
			WriteData(Address, Detour.data(), Detour.size(), false);
			__DEBUG("DKU_H: Enabled detour hook @ {:X}", Address);
		}

		void Disable() noexcept override
		{
			WriteData(Address, OldBytes.data(), OldBytes.size(), false);
			__DEBUG("DKU_H: Disabled detour hook @ {:X}", Address);
		}

		template <typename F>
			requires(model::concepts::dku_memory<F>)
		constexpr operator F() const noexcept
		{
			return std::bit_cast<F>(OriginalFunc);
		}

		const std::size_t   StolenSize;
		const Imm64         OriginalFunc;
		Imm64               Destination;
		std::vector<OpCode> OldBytes{};
		std::vector<OpCode> Detour{};
	};

	/** \brief Detour a function at its entry to target hook function, the stolen prologue is relocated to trampoline
	 * \brief Stolen prologue is copied verbatim, it must end on an instruction boundary and contain no rip-relative or branch instruction
	 * \param <N> : Length of stolen prologue, at least 5
	 * \param a_src : Address of the first instruction of target function
	 * \param a_dst : Destination function
	 * \return DetourHookHandle
	 */
	template <std::size_t N>
	[[nodiscard]] inline auto AddDetourHook(
		std::uintptr_t a_src,
		std::uintptr_t a_dst) noexcept
	{
		static_assert(N >= sizeof(JmpRel), "stolen prologue cannot fit a jmp rel32");

		dku_assert(a_src && a_dst,
			"DKU_H: Detour must have valid target and destination\n"
			"from : {:X}\nto   : {:X}",
			a_src, a_dst);

		// cheap sanity check, the first instruction is the only boundary known without a disassembler
		{
			const auto* head = std::bit_cast<const OpCode*>(a_src);
			const auto  rex = (head[0] & 0xF0) == 0x40;
			const auto  op = head[rex];
			const auto  rm = head[rex + 1];

			const auto relocatable =
				op != 0xE8 && op != 0xE9 && op != 0xEB &&       // call/jmp rel
				!(op == 0xFF && (rm == 0x15 || rm == 0x25)) &&  // call/jmp [rip + disp]
				op != RET && op != 0xC2 && op != INT3;          // not a function body

			dku_assert(relocatable,
				"DKU_H: Detour target head cannot be relocated\n"
				"from : {}.{:X}\nhead : {:02X} {:02X} {:02X}\n"
				"It's either already hooked, a thunk, or not a function entry",
				GetModuleName(), a_src, head[0], head[1], head[2]);
		}

		JmpRip asmRelay;   // tram -> hook
		JmpRel asmReturn;  // tram -> function body
		JmpRel asmDetour;  // function head -> tram

		// trampoline layout
		// [qword imm64] <- tram entry after this
		// [jmp qword ptr [rip + disp]] <- function head detour entry
		// [stolen] <- original function
		// [jmp rel32]
		auto tramPtr = TRAM_ALLOC(0);

		// tram entry
		WriteImm(tramPtr, a_dst, true);
		tramPtr += sizeof(a_dst);

		// handle
		auto handle = std::make_unique<DetourHookHandle>(a_src, tramPtr, a_dst, N);

		// relay
		asmRelay.Disp -= static_cast<Disp32>(sizeof(Imm64));
		asmRelay.Disp -= static_cast<Disp32>(sizeof(asmRelay));
		handle->Write(asmRelay);

		// original function
		handle->Write(handle->OldBytes.data(), N);

		std::ptrdiff_t disp = handle->Address + N - handle->TramPtr - sizeof(asmReturn);
		assert_trampoline_range(disp);

		asmReturn.Disp = static_cast<Disp32>(disp);
		handle->Write(asmReturn);

		// detour
		disp = handle->TramEntry - handle->Address - sizeof(asmDetour);
		assert_trampoline_range(disp);

		asmDetour.Disp = static_cast<Disp32>(disp);
		AsMemCpy(handle->Detour.data(), asmDetour);

		return std::move(handle);
	}
}  // namespace DKUtil::Hook
