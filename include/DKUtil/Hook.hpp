#pragma once


/*
 * 2.3.2
 * Minor formatting changes;
 * 
 * 2.3.1
 * CaveHookHandle changed base class;
 * 
 * 2.3.0
 * Added assembly patch;
 * Improved cave hook;
 * 
 * 2.2.0
 * Added import address table hook;
 * 
 * 2.1.0
 * Added virtual method table hook;
 * 
 * 2.0.0
 * Code partial redone; wtf are those macros;
 * Added stack buffer to help with stack unwindability;
 * Replaced old detour method ( rax call ) with ( call rip ) so DKU_H does not dirty register;
 *
 * 1.0.0 ~ 1.9.2
 * CMake integration, inter-library integration;
 * Fixed a misrelocation where trampoline ptr may not be pointed to correct address within cave;
 * Fixed an error where branch may not be correctly initiated;
 * Fixed an misorder where flags may be disabled inappropriately;
 * Reordered the cave code layout;
 * Added SMART_ALLOC and CAVE related conditional macro, if enabled:
 * - Attempt to write patches into code cave to reduce trampoline load if cave size satisfies;
 * - Attempt to skip trampoline allocation if hook function is null and cave size satisfies;
 * - Attempt to write rax-clean instructions into code cave if a_preserve and cave size satisfies;
 * Removed success checks during the writing procedure, due to SKSE64 implementation has no returning value;
 * Restructured two separate implementations into conditional compilation;
 * Resolve address differently based on which implementation is used;
 * Added derived prototype of BranchToFunction<...>(...) without address library usage;
 * Added support for ( a_hookFunc = 0 ) so that patches will be applied without branching to any function;
 * Added default value for patch related parameters, if branching is done without any patches;
 * Fixed various rvalue const cast errors within SKSE64 implementation;
 * Added prototype of GetCurrentPtr();
 * Added prototype of ResetPtr();
 * Renamed some template parameters;
 * Changed some local variables to constexpr;
 * Added SKSE64 implementation;
 * Added VERBOSE conditional to log each step for better debugging;
 * Moved predefined values into Impl namespace;
 * Changed above values from const to constexpr;
 * Renamed InjectAt<...>(...) to BranchToFunction<...>(...);
 * Changed patch type from ( const char* ) to ( const void* );
 * Removed strlen(...) usage;
 * Added additional parameter into InjectAt<...>( a_hookFunc, a_prePatch, a_prePatchSize, a_postPatch, a_postPatchSize );
 * Removed BranchAt<...>(...);
 * Integrated CodeGenerator;
 * Implemented InjectAt<...>(...);
 * Added prototype of bool : BranchAt< a_hookFunc >( a_prePatch, a_postPatch );
 * Added prototype of InjectAt< ID, START, END >( a_hookFunc, a_prePatch, a_postPatch );
 */


#define DKU_H_VERSION_MAJOR     2
#define DKU_H_VERSION_MINOR     1
#define DKU_H_VERSION_REVISION  0


#pragma warning ( disable : 4244 )


 // AdditionalInclude
#include <bit>
#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <xbyak/xbyak.h>

// DKUtil
#include "Logger.hpp"

#ifdef DKU_G_DEBUG
#define DKU_DEBUG
#define DEBUG(...)	INFO(__VA_ARGS__)
#endif

#include "Utility.hpp"

// Address Library
#if ANNIVERSARY_EDITION
#include "external/versionlibdb.h"
#else
#include "external/versiondb.h"
#endif


#define AsAddress(PTR)		std::bit_cast<std::uintptr_t>(PTR)
#define AsPointer(ADDR)		std::bit_cast<void*>(ADDR)


// CommonLib
#ifndef TRAMPOLINE
#include "SKSE/API.h"

#define TRAMPOLINE			SKSE::GetTrampoline()
#define TRAM_ALLOC(SIZE)	AsAddress((TRAMPOLINE).allocate((SIZE)))
#define PAGE_ALLOC(SIZE)	SKSE::AllocTrampoline((SIZE))

#endif


#define REQUEST_ALLOC		true
#define NO_ALLOC			false
#define FORWARD_PTR			true
#define NO_FORWARD			false


#define CAVE_MINIMUM_BYTES	0x5

#ifndef CAVE_MAXIMUM_BYTES
#define CAVE_MAXIMUM_BYTES	0x80
#endif


#define FUNC_INFO(FUNC)		DKUtil::Hook::FuncInfo{reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::function::GetFuncArgsCount(FUNC), #FUNC }
#define MEM_FUNC_INFO(FUNC)	DKUtil::Hook::FuncInfo{reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::function::GetMemFuncArgsCount(FUNC), #FUNC }
#define RT_INFO(FUNC, NAME)	DKUtil::Hook::FuncInfo{FUNC, 0, NAME}


namespace DKUtil
{
	constexpr auto DKU_H_VERSION = DKU_H_VERSION_MAJOR * 10000 + DKU_H_VERSION_MINOR * 100 + DKU_H_VERSION_REVISION;
} // namespace DKUtil


namespace DKUtil::Hook
{
	using REX = std::uint8_t;
	using OpCode = std::uint8_t;
	using ModRM = std::uint8_t;
	using SIndex = std::uint8_t;
	using Disp8 = std::int8_t;
	using Disp16 = std::int16_t;
	using Disp32 = std::int32_t;
	using Imm8 = std::uint8_t;
	using Imm16 = std::uint16_t;
	using Imm32 = std::uint32_t;
	using Imm64 = std::uint64_t;

	namespace detail
	{
		constexpr OpCode NOP = 0x90;
		constexpr OpCode INT3 = 0xCC;
		constexpr OpCode RET = 0xC3;


#pragma pack ( push, 1 )
		struct JmpRel
		{
			OpCode Jmp = 0xE9; // cd
			Imm32 Rel32 = 0x00000000;
		};
		static_assert(sizeof(JmpRel) == 0x5);


		struct JmpRip
		{
			OpCode Jmp = 0xFF; // 4
			ModRM Rip = 0x25; // 1 0 1
			Disp32 Disp = 0x00000000;
		};
		static_assert(sizeof(JmpRip) == 0x6);


		struct CallRip
		{
			OpCode Call = 0xFF; // 2
			ModRM Rip = 0x15; // 1 0 1
			Disp32 Disp = 0x00000000;
		};
		static_assert(sizeof(CallRip) == 0x6);


		struct PushImm64
		{
			OpCode Push = 0x68; // id
			Imm32 Low = 0x00000000; // >> 32
			OpCode Mov = 0xC7; // 0 id
			ModRM Sib = 0x44; // 1 0 0
			SIndex Rsp = 0x24;
			Disp8 Disp = sizeof(Imm32);
			Imm32 High = 0x00000000; // & 0xFFFFFFFFLL
		};
		static_assert(sizeof(PushImm64) == 0xD);


		struct SubRsp
		{
			REX W = 0x48;
			OpCode Sub = 0x83; // 5 ib
			ModRM Rsp = 0xEC; // 1 0 0
			Imm8 Size = 0x00;
		};
		static_assert(sizeof(SubRsp) == 0x4);


		struct AddRsp
		{
			REX W = 0x48;
			OpCode Add = 0x83; // 0 ib
			ModRM Rsp = 0xC4; // 1 0 0
			Imm8 Size = 0x00;
		};
		static_assert(sizeof(AddRsp) == 0x4);
#pragma pack ( pop )
	} // namespace detail


	/* HookHandle */

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
		constexpr derived_t* As() noexcept { return dynamic_cast<derived_t*>(this); }
	protected:
		HookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry)
			: Address(a_address), TramEntry(a_tramEntry), TramPtr(a_tramEntry)
		{}
	};


	struct Patch
	{
		const void* Data;
		const std::size_t Size;
	};


	struct FuncInfo
	{
		std::uintptr_t Address;
		std::size_t ArgsCount;
		std::string_view Name;
	};


	/* Trampoline */
	// https://stackoverflow.com/a/54732489/17295222
	inline void* Allocate2GBRange(const std::uintptr_t a_address, const std::size_t a_size)
	{
		static std::uint32_t dwAllocationGranularity;

		if (!dwAllocationGranularity) {
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			dwAllocationGranularity = si.dwAllocationGranularity;
		}

		std::uintptr_t min, max, addr, add = dwAllocationGranularity - 1, mask = ~add;

		min = a_address >= 0x80000000 ? (a_address - 0x80000000 + add) & mask : 0;
		max = a_address < (UINTPTR_MAX - 0x80000000) ? (a_address + 0x80000000) & mask : UINTPTR_MAX;

		MEMORY_BASIC_INFORMATION mbi;
		do {
			if (!VirtualQuery(AsPointer(min), &mbi, sizeof(mbi))) {
				return nullptr;
			}

			min = AsAddress(mbi.BaseAddress) + mbi.RegionSize;

			if (mbi.State == MEM_FREE) {
				addr = (AsAddress(mbi.BaseAddress) + add) & mask;

				if (addr < min && a_size <= (min - addr)) {
					if (addr = AsAddress(VirtualAlloc(AsPointer(addr), a_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
						return AsPointer(addr);
					}
				}
			}

		} while (min < max);

		return nullptr;
	}


	/* Helpers */

	template <typename data_t>
	concept dku_h_pod_t =
		std::is_integral_v<data_t> ||
		(std::is_standard_layout_v<data_t> &&
			std::is_trivial_v<data_t>);


	inline void WriteData(std::uintptr_t& a_dst, const void* a_data, const std::size_t a_size, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		if (a_requestAlloc) {
			void(TRAM_ALLOC(a_size));
		}

		DWORD oldProtect;

		auto success = VirtualProtect(AsPointer(a_dst), a_size, PAGE_EXECUTE_READWRITE, std::addressof(oldProtect));
		if (success != FALSE) {
			std::memcpy(AsPointer(a_dst), a_data, a_size);
			success = VirtualProtect(AsPointer(a_dst), a_size, oldProtect, std::addressof(oldProtect));
		}

		assert(success != FALSE);

		if (a_forwardPtr) {
			a_dst += a_size;
		}
	}


	inline void WriteData(const std::uintptr_t& a_dst, const void* a_data, const std::size_t a_size, bool a_requestAlloc = NO_ALLOC) noexcept
	{
		return WriteData(const_cast<std::uintptr_t&>(a_dst), a_data, a_size, NO_FORWARD, a_requestAlloc);
	}


	inline void WriteImm(std::uintptr_t& a_dst, const dku_h_pod_t auto& a_data, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(a_dst, std::addressof(a_data), sizeof(a_data), a_forwardPtr, a_requestAlloc);
	}


	inline void WriteImm(const std::uintptr_t& a_dst, const dku_h_pod_t auto& a_data, bool a_requestAlloc = NO_ALLOC) noexcept
	{
		return WriteData(const_cast<std::uintptr_t&>(a_dst), std::addressof(a_data), sizeof(a_data), NO_FORWARD, a_requestAlloc);
	}


	inline void WritePatch(std::uintptr_t& a_dst, const Patch* a_patch, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(a_dst, a_patch->Data, a_patch->Size, a_forwardPtr, a_requestAlloc);
	}


	inline void WritePatch(const std::uintptr_t& a_dst, const Patch* a_patch, bool a_requestAlloc = NO_ALLOC) noexcept
	{
		return WriteData(const_cast<std::uintptr_t&>(a_dst), a_patch->Data, a_patch->Size, NO_FORWARD, a_requestAlloc);
	}


	inline std::uintptr_t IDToAbs(std::uint64_t a_id)
	{
		VersionDb db;
		if (!db.Load()) {
			ERROR("Failed to load version database!"sv);
		}

		const auto resolvedAddr = AsAddress(db.FindAddressById(a_id));
		if (!resolvedAddr) {
			ERROR("Failed to resolve address by id {:X}", a_id);
		}

		const auto base = std::bit_cast<std::uintptr_t>(GetModuleHandleA(db.GetModuleName().c_str()));
		DEBUG("DKU_H: Resolved: {:X} | Base: {:X} | RVA: {:X}", resolvedAddr, base, resolvedAddr - base);

		db.Clear();

		return resolvedAddr;
	}


	inline constexpr std::uintptr_t TblToAbs(const std::uintptr_t a_base, const std::uint16_t a_index, const std::size_t a_size = sizeof(Imm64)) noexcept
	{
		return AsAddress(a_base + a_index * a_size);
	}

#pragma region ASMPatch

	class ASMPatchHandle : public HookHandle
	{
	public:
		// execution address
		ASMPatchHandle(const std::uintptr_t a_address, const std::ptrdiff_t a_offsetLow, const std::ptrdiff_t a_offsetHigh) noexcept
			: HookHandle(a_address, a_address + a_offsetLow), OffsetLow(a_offsetLow), OffsetHigh(a_offsetHigh), PatchSize(OffsetHigh - OffsetLow)
		{
			std::memcpy(OldBytes, AsPointer(TramEntry), PatchSize);
			std::fill_n(PatchBuf, PatchSize, detail::NOP);

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


		const std::ptrdiff_t OffsetLow;
		const std::ptrdiff_t OffsetHigh;
		const std::size_t PatchSize;

		OpCode OldBytes[CAVE_MAXIMUM_BYTES]{};
		OpCode PatchBuf[CAVE_MAXIMUM_BYTES]{};
	};


	/* Apply assembly patch in the body of execution
	 * @param OffsetLow : Beginning offset of the cave
	 * @param OffsetHigh : Ending offset of the cave
	 * @param a_address : Address of the beginning at target body of execution
	 * @param a_patch : Assembly patch
	 * @returns ASMPatchHandle
	 */
	template <const std::ptrdiff_t OffsetLow, const std::ptrdiff_t OffsetHigh>
		requires (OffsetHigh > OffsetLow)
	inline auto AddASMPatch(
		const std::uintptr_t a_address,
		const Patch* a_patch,
		const bool a_forward = true
	) noexcept
	{
		using namespace detail;

		if (!a_address || !a_patch) {
			ERROR("DKU_H: Invalid ASM patch"sv);
		}

		auto handle = std::make_unique<ASMPatchHandle>(a_address, OffsetLow, OffsetHigh);

		if (a_patch->Size > (OffsetHigh - OffsetLow)) {
			DEBUG("DKU_H: ASM patch size exceeds the patch capacity, enabled trampoline"sv);

			JmpRel asmDetour; // cave -> tram
			JmpRel asmReturn; // tram -> cave

			handle->TramPtr = TRAM_ALLOC(0);
			DEBUG("DKU_H: ASM patch tramoline entry -> {:X}", handle->TramPtr);

			asmDetour.Rel32 = static_cast<Imm32>(handle->TramPtr - handle->TramEntry - sizeof(asmDetour));
			std::memcpy(handle->PatchBuf, &asmDetour, sizeof(asmDetour));

			WritePatch(handle->TramPtr, a_patch);

			if (a_forward) {
				asmReturn.Rel32 = static_cast<Imm32>(handle->TramEntry + handle->PatchSize - handle->TramPtr - sizeof(asmReturn));
			} else {
				asmReturn.Rel32 = static_cast<Imm32>(handle->TramEntry + a_patch->Size - handle->TramPtr - sizeof(asmReturn));
			}

			WriteData(handle->TramPtr, &asmReturn, sizeof(asmReturn));
		} else {
			std::memcpy(handle->PatchBuf, a_patch->Data, a_patch->Size);

			if (a_forward && handle->PatchSize > (a_patch->Size * 2 + sizeof(JmpRel))) {
				JmpRel asmForward;

				asmForward.Rel32 = static_cast<Imm32>(handle->TramEntry + handle->PatchSize - handle->TramEntry - a_patch->Size - sizeof(asmForward));
				std::memcpy(handle->PatchBuf + a_patch->Size, &asmForward, sizeof(asmForward));

				DEBUG("DKU_H: ASM patch forwarded"sv);
			}
		}

		return std::move(handle);
	}

#pragma endregion


#pragma region CaveHook

	class CaveHookHandle : public HookHandle
	{
	public:
		// execution address, trampoline address, cave low offset, cave high offset
		CaveHookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramPtr, const std::ptrdiff_t a_offsetLow, const std::ptrdiff_t a_offsetHigh) noexcept
			: HookHandle(a_address, a_tramPtr), OffsetLow(a_offsetLow), OffsetHigh(a_offsetHigh), CaveSize(OffsetHigh - OffsetLow), CaveEntry(Address + OffsetLow), CavePtr(Address + OffsetLow)
		{
			std::memcpy(OldBytes, AsPointer(CaveEntry), CaveSize);
			std::fill_n(CaveBuf, CaveSize, detail::NOP);

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


		const std::ptrdiff_t OffsetLow;
		const std::ptrdiff_t OffsetHigh;
		const std::size_t CaveSize;
		const std::uintptr_t CaveEntry;

		std::uintptr_t CavePtr{ 0x0 };

		OpCode OldBytes[CAVE_MAXIMUM_BYTES]{};
		OpCode CaveBuf[CAVE_MAXIMUM_BYTES]{};
	};


	enum class CaveReturnPoint : std::uint32_t
	{
		kNextOp,	// after JmpRel
		kSkipOP		// skip NOPs
	};


	/* Branch to target function in the body of execution.
	 * If stack manipulation is involved, add stack offset (sizeof(std::uintptr_t) * (number of target function's arguments))
	 * @param OffsetLow : Beginning offset of the cave
	 * @param OffsetHigh : Ending offset of the cave
	 * @param a_address : Address of the beginning at target body of execution
	 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapped function
	 * @param a_prolog : Prolog patch before detouring to target function
	 * @param a_epilog : Epilog patch after returning from target function
	 * @returns CaveHookHandle
	 */
	template <const std::ptrdiff_t OffsetLow, const std::ptrdiff_t OffsetHigh, const CaveReturnPoint ReturnPoint = CaveReturnPoint::kSkipOP>
		requires ((OffsetHigh - OffsetLow) >= CAVE_MINIMUM_BYTES)
	inline auto AddCaveHook(
		const std::uintptr_t a_address,
		const FuncInfo a_funcInfo,
		const Patch* a_prolog = nullptr,
		const Patch* a_epilog = nullptr
	) noexcept
	{
		using namespace detail;

		JmpRel asmDetour; // cave -> tram
		JmpRel asmReturn; // tram -> cave
		SubRsp asmSub;
		AddRsp asmAdd;
		CallRip asmBranch;

		// trampoline layout
		// [qword imm64] <- tram entry
		// [prolog] <- cave detour entry
		// [alloc stack]
		// [call qword ptr [rip + disp]]
		// [dealloc stack]
		// [epilog]
		// [jmp rel32]

		auto tramPtr = TRAM_ALLOC(0);

		WriteImm(tramPtr, a_funcInfo.Address);
		DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.Name.data(), PROJECT_NAME, a_funcInfo.Address);

		auto handle = std::make_unique<CaveHookHandle>(a_address, tramPtr, OffsetLow, OffsetHigh);

		asmDetour.Rel32 = static_cast<Imm32>(handle->TramPtr - handle->CavePtr - sizeof(asmDetour));
		std::memcpy(handle->CaveBuf, &asmDetour, sizeof(asmDetour));

		if (a_prolog) {
			WritePatch(handle->TramPtr, a_prolog);
			asmBranch.Disp -= static_cast<Disp32>(a_prolog->Size);
		}

		const auto stackBufSize = a_funcInfo.ArgsCount * sizeof(std::uint64_t);
		if (!stackBufSize) {
			ERROR("DKU_H: Cave hook currently does not support function without argument!"sv);
		}

		asmSub.Size = stackBufSize;
		asmAdd.Size = stackBufSize;

		asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
		asmBranch.Disp -= static_cast<Disp32>(sizeof(asmSub));
		asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));

		WriteData(handle->TramPtr, &asmSub, sizeof(asmSub));
		WriteData(handle->TramPtr, &asmBranch, sizeof(asmBranch));
		WriteData(handle->TramPtr, &asmAdd, sizeof(asmAdd));

		if (a_epilog) {
			WritePatch(handle->TramPtr, a_epilog);
		}

		if constexpr (ReturnPoint == CaveReturnPoint::kNextOp) {
			asmReturn.Rel32 = static_cast<Imm32>(handle->CavePtr - handle->TramPtr - sizeof(asmReturn));
		} else if constexpr (ReturnPoint == CaveReturnPoint::kSkipOP) {
			asmReturn.Rel32 = static_cast<Imm32>(handle->Address + handle->OffsetHigh - handle->TramPtr - sizeof(asmReturn));
		}

		WriteData(handle->TramPtr, &asmReturn, sizeof(asmReturn));

		return std::move(handle);
	}

#pragma endregion


#pragma region VMTHook

	class VMTHookHandle : public HookHandle
	{
	public:
		// VTBL address, target func address, VTBL func index
		VMTHookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry, const std::uint16_t a_index) noexcept
			: HookHandle(TblToAbs(a_address, a_index), a_tramEntry), OldAddress(*std::bit_cast<std::uintptr_t*>(Address))
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
	 * @param a_prolog : Prolog patch before detouring to target function
	 * @return VMTHookHandle
	 */
	inline auto AddVMTHook(
		const void* a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo a_funcInfo,
		const Patch* a_prolog = nullptr
	) noexcept
	{
		using namespace detail;

		if (!a_funcInfo.Address) {
			ERROR("DKU_H: VMT hook must have a valid function pointer"sv);
		}
		DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.Name.data(), PROJECT_NAME, a_funcInfo.Address);

		if (a_prolog) {
			auto tramPtr = TRAM_ALLOC(0);

			CallRip asmBranch;

			WriteImm(tramPtr, a_funcInfo.Address);
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

			auto handle = std::make_unique<VMTHookHandle>(*std::bit_cast<std::uintptr_t*>(a_vtbl), tramPtr, a_index);

			WritePatch(tramPtr, a_prolog);
			asmBranch.Disp -= static_cast<Disp32>(a_prolog->Size);

			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));
			WriteData(tramPtr, &asmBranch, sizeof(asmBranch));

			return std::move(handle);
		} else {
			auto handle = std::make_unique<VMTHookHandle>(*std::bit_cast<std::uintptr_t*>(a_vtbl), a_funcInfo.Address, a_index);
			return std::move(handle);
		}
	}

#pragma endregion


	// TODO
#pragma region ThunkHook
#pragma endregion


#pragma region IATHook

	class IATHookHandle : public HookHandle
	{
	public:
		// IAT address, target func address, IAT func name, target func name
		IATHookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry, const char* a_methodName, const char* a_funcName) noexcept
			: HookHandle(a_address, a_tramEntry), OldAddress(*std::bit_cast<std::uintptr_t*>(Address))
		{
			DEBUG("DKU_H: IAT @ {:X}\nOld entry {} @ {:X} | New entry {} @ {:X}", a_address, a_methodName, OldAddress, a_funcName, a_tramEntry);
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
	 * @param a_prolog : Prolog patch before detouring to target function
	 * @return VMTHookHandle
	 */
	inline auto AddIATHook(
		const char* a_moduleName,
		const char* a_methodName,
		const FuncInfo a_funcInfo,
		const Patch* a_prolog = nullptr
	) noexcept
	{
		using namespace detail;

		if (!a_funcInfo.Address) {
			ERROR("DKU_H: IAT hook must have a valid function pointer"sv);
		}
		DEBUG("DKU_H: Detour -> {} @ {}.{:X}", a_funcInfo.Name.data(), PROJECT_NAME, a_funcInfo.Address);

		auto* dosHeader = std::bit_cast<IMAGE_DOS_HEADER*>(GetModuleHandleA(a_moduleName));
		if (!dosHeader) {
			ERROR("DKU_H: IAT module name {} invalid", a_moduleName ? a_moduleName : "ProcessHost");
		}
		
		auto* ntHeader = std::bit_cast<IMAGE_NT_HEADERS*>(AsAddress(dosHeader) + dosHeader->e_lfanew);
		if (!ntHeader || ntHeader->Signature != IMAGE_NT_SIGNATURE) {
			ERROR("DKU_H: IAT NT header invalid"sv);
		}

		auto* importDesc = std::bit_cast<IMAGE_IMPORT_DESCRIPTOR*>(dosHeader + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		for (auto index = 0; importDesc[index].Characteristics != 0; ++index) {
			const char* moduleName = std::bit_cast<const char*>(dosHeader + importDesc[index].Name);
			if (_strcmpi(a_moduleName, moduleName) != 0) {
				continue;
			}

			if (!importDesc[index].FirstThunk || !importDesc[index].OriginalFirstThunk) {
				ERROR("DKU_H: IAT read invalid thunk pointer"sv);
			}

			auto* thunk = std::bit_cast<IMAGE_THUNK_DATA*>(dosHeader + importDesc[index].FirstThunk);
			auto* oldThunk = std::bit_cast<IMAGE_THUNK_DATA*>(dosHeader + importDesc[index].OriginalFirstThunk);

			for (void(0); thunk->u1.Function; ++oldThunk, ++thunk) {
				if (oldThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
					continue;
				}

				auto* tbl = std::bit_cast<IMAGE_IMPORT_BY_NAME*>(dosHeader + oldThunk->u1.AddressOfData);

				if (_strcmpi(a_methodName, std::bit_cast<const char*>(std::addressof(tbl->Name[0]))) != 0) {
					continue;
				}

				if (a_prolog) {
					auto tramPtr = TRAM_ALLOC(0);

					CallRip asmBranch;

					WriteImm(tramPtr, a_funcInfo.Address);
					asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

					auto handle = std::make_unique<IATHookHandle>(*std::bit_cast<std::uintptr_t*>(thunk->u1.Function), tramPtr, a_methodName, a_funcInfo.Name.data());

					WritePatch(tramPtr, a_prolog);
					asmBranch.Disp -= static_cast<Disp32>(a_prolog->Size);

					asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));
					WriteData(tramPtr, &asmBranch, sizeof(asmBranch));

					return std::move(handle);
				} else {
					auto handle = std::make_unique<IATHookHandle>(*std::bit_cast<std::uintptr_t*>(thunk->u1.Function), a_funcInfo.Address, a_methodName, a_funcInfo.Name.data());
					return std::move(handle);
				}
			}
		}

		ERROR("DKU_H: IAT reached the end of table\n\nMethod {} not found", a_methodName);
	}
#pragma endregion
} // namespace DKUtil::Hook


namespace DKUtil::Alias
{
	using OpCode = std::uint8_t;
	using Disp8 = std::int8_t;
	using Disp16 = std::int16_t;
	using Disp32 = std::int32_t;
	using Imm8 = std::uint8_t;
	using Imm16 = std::uint16_t;
	using Imm32 = std::uint32_t;
	using Imm64 = std::uint64_t;

	using Patch = DKUtil::Hook::Patch;
	using HookHandle = std::unique_ptr<DKUtil::Hook::HookHandle>;
	using CaveHandle = DKUtil::Hook::CaveHookHandle;
	using VMTHandle = DKUtil::Hook::VMTHookHandle;
	using IATHandle = DKUtil::Hook::IATHookHandle;
} // namespace DKUtil::Alias


#pragma warning ( default : 4244 )
