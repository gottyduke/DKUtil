#pragma once


/*
 * 2.1.0;
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
#include <cstdint>
#include <xbyak/xbyak.h>

// DKUtil
#include "Logger.hpp"
#include "Utility.hpp"

// Address Library
#if ANNIVERSARY_EDITION
#include "external/versionlibdb.h"
#else
#include "external/versiondb.h"
#endif

// CommonLib
#ifndef TRAMPOLINE
#include "SKSE/API.h"

#define TRAMPOLINE			SKSE::GetTrampoline()
#define TRAM_ALLOC(SIZE)	reinterpret_cast<std::uintptr_t>((TRAMPOLINE).allocate((SIZE)))
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


#define FUNC_INFO(FUNC)		DKUtil::Hook::FuncInfo{reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::Utility::function::GetFuncArgsCount(FUNC), #FUNC }
#define MEM_FUNC_INFO(FUNC)	DKUtil::Hook::FuncInfo{reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::Utility::function::GetMemFuncArgsCount(FUNC), #FUNC }
#define DKU_H_NO_FUNC		DKUtil::Hook::FuncInfo{0, 0, ""sv}


namespace DKUtil::Hook
{
	constexpr auto DKU_H_VERSION = DKU_H_VERSION_MAJOR * 10000 + DKU_H_VERSION_MINOR * 100 + DKU_H_VERSION_REVISION;


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

	namespace Impl
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
	} // namespace Impl


	struct Patch
	{
		const void* Data;
		const std::size_t Size;
	};


	// helper struct to describe function
	struct FuncInfo
	{
		std::uintptr_t Address;
		std::size_t ArgsCount;
		std::string_view Name;
	};


	/* Helpers */

	template <typename data_t>
	concept dku_h_pod_t =
		std::is_integral_v<data_t> ||
		(std::is_standard_layout_v<data_t> &&
			std::is_trivial_v<data_t>);


	inline void WriteData(std::uintptr_t& a_dst, const void* a_data, const std::size_t a_size, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		if (a_requestAlloc) {
			TRAM_ALLOC(a_size);
		}

		DWORD oldProtect;
		auto success = VirtualProtect(reinterpret_cast<void*>(a_dst), a_size, PAGE_EXECUTE_READWRITE, std::addressof(oldProtect));
		if (success != FALSE) {
			std::memcpy(reinterpret_cast<void*>(a_dst), a_data, a_size);
			success = VirtualProtect(reinterpret_cast<void*>(a_dst), a_size, oldProtect, std::addressof(oldProtect));
		}

		assert(success != FALSE);

		if (a_forwardPtr) {
			a_dst += a_size;
		}
	}


	inline void WriteData(const std::uintptr_t&& a_dst, const void* a_data, const std::size_t a_size, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(const_cast<std::uintptr_t&>(a_dst), a_data, a_size, NO_FORWARD, a_requestAlloc);
	}


	inline void WriteImm(std::uintptr_t& a_dst, const dku_h_pod_t auto& a_data, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(a_dst, std::addressof(a_data), sizeof(a_data), a_forwardPtr, a_requestAlloc);
	}


	inline void WriteImm(const std::uintptr_t&& a_dst, const dku_h_pod_t auto& a_data, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(const_cast<std::uintptr_t&>(a_dst), std::addressof(a_data), sizeof(a_data), NO_FORWARD, a_requestAlloc);
	}


	inline void WritePatch(std::uintptr_t& a_dst, const Patch* a_patch, bool a_forwardPtr = FORWARD_PTR, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(a_dst, a_patch->Data, a_patch->Size, a_forwardPtr, a_requestAlloc);
	}


	inline void WritePatch(const std::uintptr_t&& a_dst, const Patch* a_patch, bool a_requestAlloc = REQUEST_ALLOC) noexcept
	{
		return WriteData(const_cast<std::uintptr_t&>(a_dst), a_patch->Data, a_patch->Size, NO_FORWARD, a_requestAlloc);
	}


	inline std::uintptr_t RVA2Abs(std::uint64_t a_id)
	{
		VersionDb db;
		if (!db.Load()) {
			ERROR("Failed to load version database!"sv);
		}

		const auto resolvedAddr = reinterpret_cast<std::uintptr_t>(db.FindAddressById(a_id));
		db.Clear();

		if (!resolvedAddr) {
			ERROR("Failed to resolve address by id {:x}", a_id);
		}
		DEBUG("Address: Resolved {:x}", resolvedAddr);

		return resolvedAddr;
	}


	/* HookHandle */

	class HookHandle
	{
	public:
		virtual void Enable() noexcept = 0;
		virtual void Disable() noexcept = 0;


		const std::uintptr_t Address;
		const std::uintptr_t TramEntry;
		std::uintptr_t TramPtr{ 0x0 };


		virtual ~HookHandle() noexcept = default;
	protected:
		HookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry)
			: Address(a_address), TramEntry(a_tramEntry), TramPtr(a_tramEntry)
		{}
	};


#pragma region CaveHook

	class CaveHookHandle : public HookHandle
	{
	public:
		CaveHookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry, const std::ptrdiff_t a_offsetLow, const std::ptrdiff_t a_offsetHigh) noexcept
			: HookHandle(a_address, a_tramEntry), OffsetLow(a_offsetLow), OffsetHigh(a_offsetHigh), CaveEntry(Address + OffsetLow), CavePtr(Address + OffsetLow)
		{
			DEBUG("DKU_H: Cave capacity: {} bytes\nCave Entry @ {:x} | Tram Entry @ {:x}", OffsetHigh - OffsetLow, CaveEntry, TramEntry);
		}


		void Enable() noexcept override
		{
			WriteData(CavePtr, CaveBuf, OffsetHigh - OffsetLow, FORWARD_PTR, NO_ALLOC);
			DEBUG("DKU_H: Enabled cave hook"sv);
		}


		void Disable() noexcept override
		{
			WriteData(CavePtr, OldBytes, OffsetHigh - OffsetLow, NO_FORWARD, NO_ALLOC);
			DEBUG("DKU_H: Disabled cave hook"sv);
		}


		const std::ptrdiff_t OffsetLow;
		const std::ptrdiff_t OffsetHigh;
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


	// empty a code cave in the body of target function and branch to trampoline
	// accepts a prolog patch before invoking payload and a epilog patch after returning from payload
	template <const std::ptrdiff_t OffsetLow, const std::ptrdiff_t OffsetHigh, const CaveReturnPoint ReturnPoint = CaveReturnPoint::kSkipOP>
		requires ((OffsetHigh - OffsetLow) >= CAVE_MINIMUM_BYTES)
	inline auto AddCaveHook(
		const std::uintptr_t a_src,
		const FuncInfo a_func = DKU_H_NO_FUNC,
		const Patch* a_prolog = nullptr,
		const Patch* a_epilog = nullptr
	) noexcept
	{
		using namespace Impl;

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

		if (a_func.Address) {
			WriteImm(tramPtr, a_func.Address);
			DEBUG("DKU_H: Detour -> {} @ {}.{:x}", a_func.Name.data(), Version::PROJECT.data(), a_func.Address);
		}

		auto handle = std::make_unique<CaveHookHandle>(a_src, tramPtr, OffsetLow, OffsetHigh);

		std::memcpy(handle->OldBytes, reinterpret_cast<void*>(handle->CaveEntry), handle->OffsetHigh - handle->OffsetLow);
		std::fill_n(handle->CaveBuf, handle->OffsetHigh - handle->OffsetLow, NOP);

		asmDetour.Rel32 = static_cast<Imm32>(handle->TramPtr - handle->CavePtr - sizeof(asmDetour));
		std::memcpy(handle->CaveBuf, &asmDetour, sizeof(asmDetour));

		if (a_prolog) {
			WritePatch(handle->TramPtr, a_prolog);
			asmBranch.Disp -= static_cast<Disp32>(a_prolog->Size);
		}

		if (a_func.Address) {
			const auto stackBufSize = a_func.ArgsCount * sizeof(std::uint64_t);
			if (!stackBufSize) {
				ERROR("DKU_H: AddCaveHook() currently does not support function without argument!"sv);
			}

			asmSub.Size = stackBufSize;
			asmAdd.Size = stackBufSize;

			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));
			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmSub));
			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));

			WriteData(handle->TramPtr, &asmSub, sizeof(asmSub));
			WriteData(handle->TramPtr, &asmBranch, sizeof(asmBranch));
			WriteData(handle->TramPtr, &asmAdd, sizeof(asmAdd));
		}

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


	// TODO
#pragma region AsmPatch
#pragma endregion


#pragma region ThunkHook
#pragma endregion


#pragma region VMTHook

	class VMTHookHandle : public HookHandle
	{
	public:

		VMTHookHandle(const std::uintptr_t a_address, const std::uintptr_t a_tramEntry, const std::uint16_t a_index) noexcept
			: HookHandle(a_address + sizeof(std::uintptr_t) * a_index, a_tramEntry), OldAddress(*reinterpret_cast<std::uintptr_t*>(Address))
		{
			DEBUG("DKU_H: VMT @ {:x} [{}]\nOld Entry @ {:x} | New Entry @ {:x}", a_address, a_index, OldAddress, TramEntry);
		}


		void Enable() noexcept override
		{
			WriteImm(std::uintptr_t(Address), TramEntry, NO_ALLOC);
			DEBUG("DKU_H: Enabled vmt hook"sv);
		}


		void Disable() noexcept override
		{
			WriteImm(std::uintptr_t(Address), OldAddress, NO_ALLOC);
			DEBUG("DKU_H: Disabled vmt hook"sv);
		}


		const std::uintptr_t OldAddress;
	};


	// swaps a virtual method table address with target function address
	// accepts a prolog patch before invoking payload
	inline auto AddVMTHook(
		void* a_vtbl,
		const FuncInfo a_func,
		const std::uint16_t a_index = 0,
		const Patch* a_prolog = nullptr
	) noexcept
	{
		using namespace Impl;

		if (!a_func.Address) {
			ERROR("DKU_H: VMTHook must have a valid function pointer"sv);
		}
		DEBUG("DKU_H: Detour -> {} @ {}.{:x}", a_func.Name.data(), Version::PROJECT.data(), a_func.Address);

		if (a_prolog) {
			auto tramPtr = TRAM_ALLOC(0);

			CallRip asmBranch;

			WriteImm(tramPtr, a_func.Address);
			asmBranch.Disp -= static_cast<Disp32>(sizeof(Imm64));

			auto handle = std::make_unique<VMTHookHandle>(*reinterpret_cast<std::uintptr_t*>(a_vtbl), tramPtr, a_index);

			WritePatch(tramPtr, a_prolog);

			asmBranch.Disp -= static_cast<Disp32>(a_prolog->Size);
			asmBranch.Disp -= static_cast<Disp32>(sizeof(asmBranch));

			WriteData(tramPtr, &asmBranch, sizeof(asmBranch));

			return std::move(handle);
		} else {
			auto handle = std::make_unique<VMTHookHandle>(*reinterpret_cast<std::uintptr_t*>(a_vtbl), a_func.Address, a_index);
			return std::move(handle);
		}
	}

#pragma endregion


#pragma region IATHook
#pragma endregion
} // namespace DKUtil::Hook


namespace DKUtil::Alias
{
	using HookHandle = std::unique_ptr<DKUtil::Hook::HookHandle>;
	using Patch = DKUtil::Hook::Patch;
} // namespace DKUtil::Alias


#pragma warning ( default : 4244 )
