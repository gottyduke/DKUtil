/* DISCLAIMER START */
/**
 * Copyright 2020 DK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
/* DISCLAIMER END */


#ifndef DKUTIL_HOOK
#define DKUTIL_HOOK


/* VERSION DEFINE START */
#define DKUTIL_HOOK_VERSION_MAJOR	1
#define DKUTIL_HOOK_VERSION_MINOR	8
#define DKUTIL_HOOK_VERSION_PATCH	1
#define DKUTIL_HOOK_VERSION_BEAT	0
/* VERSION DEFINE END */


/* PATCH NOTE START */
/**
 * 1.8.1
 * Removed success checks during the writing procedure, due to SKSE64 implementation has no returning value;
 *
 * 1.8.0
 * Restructured two separate implementations into conditional compilation;
 * Resolve address differently based on which implementation is used;
 * Added derived prototype of BranchToFunction<...>(...) without address library usage;
 *
 * 1.7.0
 * Added support for ( a_hookFunc = 0 ) so that patches will be applied without branching to any function;
 * Added default value for patch related parameters, if branching is done without any patches;
 *
 * 1.6.0
 * Fixed various rvalue const cast errors within SKSE64 implementation;
 * Added prototype of GetCurrentPtr();
 * Added prototype of ResetPtr();
 * Renamed some template parameters;
 * Changed some local variables to constexpr;
 *
 * 1.5.0
 * Added SKSE64 implementation;
 * Added VERBOSE conditional to log each step for better debugging;
 *
 * 1.4.0
 * Moved predefined values into Impl namespace;
 * Changed above values from const to constexpr;
 * Renamed InjectAt<...>(...) to BranchToFunction<...>(...);
 * 
 * 1.3.0
 * Changed patch type from ( const char* ) to ( const void* );
 * Removed strlen(...) usage;
 * Added additional parameter into InjectAt<...>( a_hookFunc, a_prePatch, a_preSize, a_postPatch, a_postSize );
 *
 * 1.2.0
 * Removed BranchAt<...>(...);
 * Integrated CodeGenerator;
 *
 * 1.1.0
 * Implemented InjectAt<...>(...);
 * Added prototype of bool : BranchAt< a_hookFunc >( a_prePatch, a_postPatch );
 *
 * 1.0.0
 * Added prototype of InjectAt< ID, START, END >( a_hookFunc, a_prePatch, a_postPatch );
 *
/* PATCH NOTE END */

#ifdef SKSE64


/* SKSE64 INCLUDE START */
#include "skse64_common/SafeWrite.h"
#include "skse64_common/BranchTrampoline.h"

#include "common/IDebugLog.h"

#ifndef XBYAK_NO_OP_NAMES
#define XBYAK_NO_OP_NAMES
#endif

#include "xbyak/xbyak.h"

#include "External/versiondb.h"
/* SKSE64 INCLUDE END */


/* SKSE64 DEFINE START */
#ifdef DKUTIL_HOOK_VERBOSE
#define FM(FMT, ...)				gLog.FormattedMessage(FMT, __VA_ARGS__);
#else
#define FM(...)
#endif

#define WRITE(ADDR, DATA, LENGTH)	SafeWriteBuf(ADDR, const_cast<void*>(DATA), LENGTH)
#define WRITE_8(ADDR, DATA_8)		SafeWrite8(ADDR, DATA_8)
#define WRITE_16(ADDR, DATA_16)		SafeWrite16(ADDR, DATA_16)
#define WRITE_32(ADDR, DATA_32)		SafeWrite32(ADDR, DATA_32)

#define ALLOCATE(LENGTH)			CurrentPtr = reinterpret_cast<std::uintptr_t>(g_localTrampoline.Allocate(LENGTH))
#define FORWARD_PTR(LENGTH)			CurrentPtr += (LENGTH);

#define CODEGEN						Xbyak::CodeGenerator
/* SKSE64 DEFINE END */


#else


/* COMMONLIB INCLUDE START */
#include "SKSE/CodeGenerator.h"
#include "SKSE/API.h"
/* COMMONLIB INCLUDE END */


/* COMMONLIB DEFINE START */
#ifdef DKUTIL_HOOK_VERBOSE
#define FM(FMT, ...)				SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kMessage, FMT, __VA_ARGS__);
#else
#define FM(...) 
#endif

#define WRITE(ADDR, DATA, LENGTH)	SKSE::SafeWriteBuf(ADDR, DATA, LENGTH)
#define WRITE_8(ADDR, DATA_8)		SKSE::SafeWrite8(ADDR, DATA_8)
#define WRITE_16(ADDR, DATA_16)		SKSE::SafeWrite16(ADDR, DATA_16)
#define WRITE_32(ADDR, DATA_32)		SKSE::SafeWrite32(ADDR, DATA_32)

#define ALLOCATE(LENGTH)			CurrentPtr = reinterpret_cast<std::uintptr_t>(SKSE::GetTrampoline()->Allocate(LENGTH))
#define FORWARD_PTR(LENGTH)			CurrentPtr += (LENGTH);

#define CODEGEN						SKSE::CodeGenerator
/* COMMONLIB DEFINE END */


#endif


namespace DKUtil::Hook
{
	/* GLOBAL DECLARATION START */
	namespace Impl
	{
		constexpr std::uint8_t NOP = 0x90;
		constexpr std::uint8_t JMP = 0xE9;
		constexpr std::uint64_t BRANCH_SIZE = 12;
		constexpr std::uint8_t BRANCH_PREFIX[] = { 0x48, 0xB8 };
		constexpr std::uint8_t BRANCH_SUFFIX[] = { 0xFF, 0xD0 };
	}

	
	/// <summary>
	/// Wrapped type to invocate with ease.
	/// Should initialize as constexpr
	/// </summary>
	struct BranchInstruction
	{
		struct PatchCode
		{
			const void* Data = nullptr;
			std::uint64_t Size = 0;
		};

		std::uintptr_t BranchTarget = 0x0;
		PatchCode PrePatch;
		PatchCode PostPatch;
	};
	
	
	/// <summary>
	/// Managed pointer of current utilizing trampoline
	/// </summary>
	std::uintptr_t CurrentPtr = 0x0;

	/// <summary>
	/// Get current pointer of the managed trampoline
	/// </summary>
	/// <returns>Current pointer of trampoline</returns>
	std::uintptr_t GetCurrentPtr() noexcept { return CurrentPtr; }

	/// <summary>
	/// Caution ! Resets the pointer of current managed trampoline
	/// </summary>
	/// <remarks>
	/// After resetting pointer,
	/// author must handle trampoline allocation oneself
	/// and keep track of the usage and address of such
	/// </remarks>
	void ResetPtr() noexcept { CurrentPtr = 0x0; }
	/* GLOBAL DECLARATION END */

	
	/* CONDITIONAL IMPLEMENTATION START */
	template <std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
	bool BranchToFunction_Impl(
		const std::uintptr_t a_resolvedAddr,
		const std::uintptr_t a_hookFunc = 0,
		const void* a_prePatch = nullptr,
		const std::uint64_t a_preSize = 0,
		const void* a_postPatch = nullptr,
		const std::uint64_t a_postSize = 0
	)
	{
#ifdef SKSE64
		FM("===SKSE64 Impl Start===");
#else
		FM("===CommonLib Impl Start===");
#endif
				
		constexpr auto CaveSize = OFFSET_END - OFFSET_START;
		const auto continueAddr = a_resolvedAddr + CaveSize;

		if (CaveSize < 5) {
			return false;
		}

		FM("<START: %p>", a_resolvedAddr + OFFSET_START);
		FM("<END: %p>", a_resolvedAddr + OFFSET_END);

		const auto allocationSize = (a_hookFunc ? Impl::BRANCH_SIZE : 0) + a_preSize + a_postSize + 5;

		FM("Will allocate %lluB", allocationSize);
		
		ALLOCATE(allocationSize);

		FM("Writing %llu NOP into code cave", CaveSize);
		
		for (auto i = 0; i < CaveSize; ++i) {
			WRITE_8(a_resolvedAddr + i, Impl::NOP);
		}

		FM("Writing first jmp from code to trampoline");
		
		// rel jmp to trampoline
		WRITE_8(a_resolvedAddr, Impl::JMP);
		WRITE_32(a_resolvedAddr + 1, CurrentPtr - a_resolvedAddr - 5);
		
		// write pre patch
		if (a_prePatch) {
			FM("Applying pre patch code")
			
			WRITE(CurrentPtr, a_prePatch, a_preSize);
			FORWARD_PTR(a_preSize);
		}

		// branch to function
		if (a_hookFunc) {
			FM("Writing branch code from trampoline to function");

#ifdef SKSE64
			CODEGEN codeGen(Impl::BRANCH_SIZE, reinterpret_cast<void*>(CurrentPtr));
#else
			CODEGEN codeGen(Impl::BRANCH_SIZE);
#endif
			codeGen.mov(codeGen.rax, a_hookFunc);
			codeGen.call(codeGen.rax);
			codeGen.ready();

#ifndef SKSE64
			WRITE(CurrentPtr, codeGen.getCode(), Impl::BRANCH_SIZE);
#endif
			
			FORWARD_PTR(Impl::BRANCH_SIZE);
		} else {
			FM("Does not branch to any function");
		}
		
		// append post patch
		if (a_postPatch) {
			FM("Appending post patch code");
			
			WRITE(CurrentPtr, a_postPatch, a_postSize);
			FORWARD_PTR(a_postSize);
		}

		// rel jmp to orginal
		FM("Writing second jmp from trampoline to code");
		
		WRITE_8(CurrentPtr, Impl::JMP);
		WRITE_32(CurrentPtr + 1, continueAddr - CurrentPtr - 5);
		FORWARD_PTR(5);

#ifdef SKSE64
		FM("===SKSE64 Impl End===");
#else
		FM("===CommonLib Impl End===");
#endif		
		return true;
	}
	/* CONDITIONAL IMPLEMENTATION END */

	/* GENERAL IMPLEMENTATION START */
	/// <summary>
	/// Inject at address, apply patches, branch to trampoline and back
	/// </summary>
	/// <param name="a_hookFunc">Function address to branch to, use 0 to not branch any function ( only append patches )</param>
	/// <param name="a_prePatch">Patch code to execute before calling branched function</param>
	/// <param name="a_preSize">Size of pre patch code</param>
	/// <param name="a_postPatch">Patch code to execute after returning from branched function</param>
	/// <param name="a_postSize">Size of post patch code</param>
	/// <typeparam name="BASE_ID">Base ID of address to apply branch on. Get from current address library bin</typeparam>
	/// <typeparam name="OFFSET_START">Offset of code cave starts from base address</typeparam>
	/// <typeparam name="OFFSET_END">Offset of code cave ends from base address</typeparam>
	/// <returns>Success indicator</returns>
	/// <remarks>Uses address library ( or REL )</remarks>
	template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
	bool BranchToFunction(
		const std::uintptr_t a_hookFunc,
		const void* a_prePatch = nullptr,
		const std::uint64_t a_preSize = 0,
		const void* a_postPatch = nullptr,
		const std::uint64_t a_postSize = 0
	)
	{
#ifdef SKSE64
		VersionDb db;
		if (!db.Load()) {
			FM("Failed to load version database!\n");
			return false;
		}

		const auto resolvedAddr = reinterpret_cast<std::uintptr_t>(db.FindAddressById(BASE_ID)) + OFFSET_START;
		if (!resolvedAddr) {
			FM("Failed to resolve address by id %llu", BASE_ID);
			return false;
		}
#else
		const auto resolvedAddr = REL::ID(BASE_ID).GetAddress() + OFFSET_START;
#endif

		return BranchToFunction_Impl
			<OFFSET_START, OFFSET_END>
			(resolvedAddr, 
			 a_hookFunc, 
			 a_prePatch, 
			 a_preSize, 
			 a_postPatch, 
			 a_postSize);
	}


	/// <summary>
	/// Packaged invocation to reduce parameter list's length
	/// </summary>
	/// <param name="a_instruction">Refer to BranchInstruction</param>
	/// <returns>Success indicator</returns>
	/// <remarks>Uses address library ( or REL )</remarks>
	template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
	bool BranchToFunction(const BranchInstruction a_instruction)
	{
		return BranchToFunction
			<BASE_ID, OFFSET_START, OFFSET_END>
			(a_instruction.BranchTarget,
			 a_instruction.PrePatch.Data,
			 a_instruction.PrePatch.Size,
			 a_instruction.PostPatch.Data,
			 a_instruction.PostPatch.Size);
	}


	/// <summary>
	/// Inject at address, apply patches, branch to trampoline and back
	/// </summary>
	/// <param name="a_hookFunc">Function address to branch to, use 0 to not branch any function ( only append patches )</param>
	/// <param name="a_prePatch">Patch code to execute before calling branched function</param>
	/// <param name="a_preSize">Size of pre patch code</param>
	/// <param name="a_postPatch">Patch code to execute after returning from branched function</param>
	/// <param name="a_postSize">Size of post patch code</param>
	/// <typeparam name="ADDRESS_START">Base address to apply branch on</typeparam>
	/// <typeparam name="ADDRESS_END">Offset of code cave ends from base address</typeparam>
	/// <returns>Success indicator</returns>
	/// <remarks>Does not use address library ( or REL )</remarks>
	template <std::uintptr_t ADDRESS_START, std::uintptr_t ADDRESS_END>
	bool BranchToFunction(
		const std::uintptr_t a_hookFunc,
		const void* a_prePatch = nullptr,
		const std::uint64_t a_preSize = 0,
		const void* a_postPatch = nullptr,
		const std::uint64_t a_postSize = 0
	)
	{
		return BranchToFunction_Impl
			<0, ADDRESS_END - ADDRESS_START>
			(ADDRESS_START, 
			 a_hookFunc,
			 a_prePatch,
			 a_preSize,
			 a_postPatch,
			 a_postSize);
	}

	
	/// <summary>
	/// Packaged invocation to reduce parameter list's length
	/// </summary>
	/// <param name="a_instruction">Refer to BranchInstruction</param>
	/// <returns>Success indicator</returns>
	/// <remarks>Does not use address library ( or REL )</remarks>
	template <std::uintptr_t ADDRESS_START, std::uintptr_t ADDRESS_END>
	bool BranchToFunction(const BranchInstruction a_instruction)
	{
		return BranchToFunction_Impl
			<0, ADDRESS_END - ADDRESS_START>
			(ADDRESS_START,
			 a_instruction.BranchTarget,
			 a_instruction.PrePatch.Data,
			 a_instruction.PrePatch.Size,
			 a_instruction.PostPatch.Data,
			 a_instruction.PostPatch.Size);

	}
	
	/* GENERAL IMPLEMENTATION END */
}


/* ALIAS START */
typedef DKUtil::Hook::BranchInstruction BranchInstruction;
/* ALIAS END */

/* UNDEFINE START */
#undef FM
#undef ALLOCATE
#undef FORWARD_PTR
#undef CODEGEN
/* UNDEFINE END */


#endif
