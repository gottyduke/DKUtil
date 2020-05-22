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

#pragma warning ( disable : 4244 )

/* VERSION DEFINE START */
#define DKUTIL_HOOK_VERSION_MAJOR	1
#define DKUTIL_HOOK_VERSION_MINOR	9
#define DKUTIL_HOOK_VERSION_PATCH	0
#define DKUTIL_HOOK_VERSION_BEAT	0
/* VERSION DEFINE END */


/* PATCH NOTE START */
/**
 * 1.9.0
 * Added SMART_ALLOC and CAVE related conditional macro:
 *	Attempt to write patches into code cave to reduce trampoline load if cave size satisfies;
 *	Attempt to skip trampoline allocation if hook function is null and cave size satisfies;
 *	Attempt to write rax-clean instructions into code cave if a_preserve and cave size satisfies;
 *
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


/* CONDITIONAL COMPILATION START */
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
#define FM(FMT, ...)			gLog.FormattedMessage(FMT, __VA_ARGS__);
#else
#define FM(...)
#endif

#define WRITE(ADDR, DATA, SIZE)	SafeWriteBuf(ADDR, const_cast<void*>(DATA), (SIZE))
#define WRITE_8(ADDR, DATA_8)	SafeWrite8(ADDR, DATA_8)
#define WRITE_16(ADDR, DATA_16)	SafeWrite16(ADDR, DATA_16)
#define WRITE_32(ADDR, DATA_32)	SafeWrite32(ADDR, DATA_32)
#define TRAMPOLINE				&g_localTrampoline
/* SKSE64 DEFINE END */


#else


/* COMMONLIB INCLUDE START */
#include "SKSE/CodeGenerator.h"
#include "SKSE/API.h"
/* COMMONLIB INCLUDE END */


/* COMMONLIB DEFINE START */
#ifdef DKUTIL_HOOK_VERBOSE
#define FM(FMT, ...)			SKSE::Impl::MacroLogger::VPrint("DKUTIL_HOOK", 0, SKSE::Logger::Level::kDebugMessage, FMT, __VA_ARGS__);
#else
#define FM(...) 
#endif

#define WRITE(ADDR, DATA, SIZE)	SKSE::SafeWriteBuf(ADDR, DATA, (SIZE))
#define WRITE_8(ADDR, DATA_8)	SKSE::SafeWrite8(ADDR, DATA_8)
#define WRITE_16(ADDR, DATA_16)	SKSE::SafeWrite16(ADDR, DATA_16)
#define WRITE_32(ADDR, DATA_32)	SKSE::SafeWrite32(ADDR, DATA_32)
#define TRAMPOLINE				SKSE::GetTrampoline()
/* COMMONLIB DEFINE END */


#endif
/* CONDITIONAL COMPILATION END */


/* GENERAL DEFINE START */
#define DEFINE_FLAG(NAME)		bool hook_flag ## NAME = false;
#define TOGGLE_FLAG(FLAG)		hook_flag ## FLAG = !hook_flag ## FLAG;
#define ACTIVE_FLAG(FLAG)		hook_flag ## FLAG

#define CURRENT_PTR_MOVE(SIZE)	CurrentPtr += (SIZE);

#define VAR_(NAME)				hook_local ## NAME
#define SIZE_(NAME)				VAR_(NAME ## Size)

#define ALLOC_INIT()\
	std::uint64_t SIZE_(Allocate) = 0;\
	std::uint64_t SIZE_(CodeGen) = 0;
#define ALLOCATE(SIZE)\
	FM("Will allocate %lluB", (SIZE));\
	CurrentPtr = reinterpret_cast<std::uintptr_t>((TRAMPOLINE)->Allocate(SIZE));

#define CODEGEN					Xbyak::CodeGenerator
#define CODEGEN_INIT()			CODEGEN VAR_(CodeGen)(SIZE_(CodeGen), (reinterpret_cast<void*>(CurrentPtr)));
#define CODEGEN_RESIZE(SIZE)	SIZE_(CodeGen) += (SIZE);
#define CODEGEN_READY()\
	ALLOCATE(SIZE_(CodeGen));\
	VAR_(CodeGen).ready();

#define CAVE_VAR_(NAME)			VAR_(Cave ## NAME)
#define CAVE_SIZE_(NAME)		CAVE_VAR_(NAME ## Size)

#define CAVE_INIT()\
	std::uintptr_t CAVE_VAR_(Ptr) = a_resolvedAddr;\
	std::uint64_t CAVE_SIZE_(Remain) = CaveSize;
#define CAVE_PTR_MOVE(SIZE)		CAVE_VAR_(Ptr) += (SIZE);
#define CAVE_SIZE_MOVE(SIZE)	CAVE_SIZE_(Remain) += (SIZE);
#define CAVE_RESIZE(SIZE)\
	CAVE_PTR_MOVE(SIZE);\
	CAVE_SIZE_MOVE(-static_cast<int>((SIZE)));\
	FM("[SMART_ALLOC] Resized cave %llu => %llu | Ptr -> 0x%04x", CaveSize, CAVE_SIZE_(Remain), CAVE_VAR_(Ptr) - a_resolvedAddr);

#define CAVE_BRANCH()\
	if (ACTIVE_FLAG(Detour)) {\
		WRITE_8(CAVE_VAR_(Ptr), Impl::JMP);\
		WRITE_32(CAVE_VAR_(Ptr) + 1, CurrentPtr - a_resolvedAddr - 5);\
		CAVE_PTR_MOVE(5);\
		TOGGLE_FLAG(Detour);\
		FM("Applied detour from code to trampoline");\
	}

#ifdef DKUTIL_HOOK_SMART_ALLOC

/* SMART_ALLOC DEFINE START */
#define CAVE_PREPATCH()\
	WRITE(CAVE_VAR_(Ptr), a_prePatch, a_preSize);\
	CAVE_RESIZE(a_preSize);\
	TOGGLE_FLAG(PrePatch);\
	FM("[SMART_ALLOC] Applied prepatch in cave");
#define CAVE_POSTPATCH()\
	WRITE(CAVE_VAR_(Ptr), a_postPatch, a_postSize);\
	CAVE_RESIZE(a_postSize);\
	TOGGLE_FLAG(PostPatch);\
	FM("[SMART_ALLOC] Applied postpatch in cave");
#define CAVE_PUSH()\
	WRITE_8(CAVE_VAR_(Ptr), Impl::PUSH_RAX);\
	CAVE_RESIZE(1);\
	TOGGLE_FLAG(Push);\
	FM("[SMART_ALLOC] Applied preserve prefix in cave");
#define CAVE_POP()\
	WRITE_8(CAVE_VAR_(Ptr), Impl::POP_RAX);\
	CAVE_RESIZE(1);\
	TOGGLE_FLAG(Pop);\
	FM("[SMART_ALLOC] Applied preserve suffix in cave");
/* SMART_ALLOC DEFINE END */

#endif
/* GENERAL DEFINE END */


namespace DKUtil::Hook
{
	/* GLOBAL DECLARATION START */
	namespace Impl
	{
		constexpr std::uint8_t NOP = 0x90;
		constexpr std::uint8_t JMP = 0xE9;
		constexpr std::uint8_t PUSH_RAX = 0x50;
		constexpr std::uint8_t POP_RAX = 0x58;
		constexpr std::uint64_t BRANCH_SIZE = 12;
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

		PatchCode PrePatch;
		PatchCode PostPatch;
		bool PreserveRax = false;
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

	/// <summary>
	/// Do not call this directly
	/// </summary>
	template <std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
	bool BranchToFunction_Impl(
		const std::uintptr_t a_resolvedAddr,
		const std::uintptr_t a_hookFunc = 0,
		const void* a_prePatch = nullptr,
		const std::uint64_t a_preSize = 0,
		const void* a_postPatch = nullptr,
		const std::uint64_t a_postSize = 0,
		const bool a_preserve = false
	)
	{
#ifdef SKSE64
		FM("===SKSE64 Impl Start===");
#else
		FM("===CommonLib Impl Start===");
#endif
		
		if (!a_resolvedAddr) {
			FM("Resolved address is invalid, will abort");
			return false;
		}
		constexpr auto CaveSize = OFFSET_END - OFFSET_START;
		const auto continueAddr = a_resolvedAddr + CaveSize;

		FM("<START: %p>", a_resolvedAddr + OFFSET_START);
		FM("<END: %p>", a_resolvedAddr + OFFSET_END);

		// flags are initially disabled
		DEFINE_FLAG(Detour);
		DEFINE_FLAG(Push);
		DEFINE_FLAG(Pop);
		DEFINE_FLAG(Return);
		DEFINE_FLAG(PrePatch);
		DEFINE_FLAG(PostPatch);
		DEFINE_FLAG(LargerPrePatch);
		DEFINE_FLAG(Branch);
		DEFINE_FLAG(Allocate);

		/* SETUP FLAG START */
		// pre patch exist
		if (a_prePatch && a_preSize) {
			TOGGLE_FLAG(PrePatch);
		}

		// post patch exist
		if (a_postPatch && a_postSize) {
			TOGGLE_FLAG(PostPatch);
		}

		// both patches exist, determine larger one
		if (ACTIVE_FLAG(PrePatch) && 
			ACTIVE_FLAG(PostPatch) && 
			a_preSize > a_postSize) {
			TOGGLE_FLAG(LargerPrePatch);
		}

		// hook function exists OR
		// hook function DNE but patch(es) are too large
		if (a_hookFunc || 
			!a_hookFunc && 
			CaveSize < a_preSize + a_postSize) {
			TOGGLE_FLAG(Detour);
			TOGGLE_FLAG(Return);
			TOGGLE_FLAG(Allocate);
		}

		// .PreserveRax = true
		if (ACTIVE_FLAG(Detour) && a_preserve) {
			TOGGLE_FLAG(Push);
			TOGGLE_FLAG(Pop);
		}
		/* SETUP FLAG END */

		
		/* CAVE START */
		/* possible cave layout:
		 * pre patch - a_preSize
		 * preserve prefix - 1
		 * detour - 5
		 * preserve suffix - 1
		 * post patch - a_postSize
		 */
		
		FM("Writing %llu NOP into code cave", CaveSize);
		for (auto i = 0; i < CaveSize; ++i) {
			WRITE_8(a_resolvedAddr + i, Impl::NOP);
		}
		
		// recalculate cave size and define cave variables
		CAVE_INIT();
		
		// reserve 5 bytes for detour code
		if (ACTIVE_FLAG(Detour)) {
			CAVE_SIZE_MOVE(-5);
		}

		
		/* SMART_ALLOC START */
#ifdef DKUTIL_HOOK_SMART_ALLOC
		
		// both patches exist, size sufficient, write both
		if (ACTIVE_FLAG(PrePatch) && 
			ACTIVE_FLAG(PostPatch) &&
			CAVE_SIZE_(Remain) >= a_preSize + a_postSize) {
			CAVE_PREPATCH();

			if (ACTIVE_FLAG(Push) && CAVE_SIZE_(Remain) > a_postSize) {
				CAVE_PUSH();
			}
			
			CAVE_BRANCH();

			CAVE_POSTPATCH();

			if (ACTIVE_FLAG(Pop) && CAVE_SIZE_(Remain) >= 1) {
				CAVE_POP();
			}
		}
		
		// both patches exist, size insufficient, pick larger one
		if (ACTIVE_FLAG(PrePatch) && 
			ACTIVE_FLAG(PostPatch) &&
			CAVE_SIZE_(Remain) >= a_preSize &&
			CAVE_SIZE_(Remain) >= a_postSize &&
			CAVE_SIZE_(Remain) < a_preSize + a_postSize) {

			if (ACTIVE_FLAG(LargerPrePatch)) {
				CAVE_PREPATCH();

				if (ACTIVE_FLAG(Push) && CAVE_SIZE_(Remain) >= 1) {
					CAVE_PUSH();
				}
			}
			
			CAVE_BRANCH();

			if (!ACTIVE_FLAG(LargerPrePatch)) {
				CAVE_POSTPATCH();
				
				if (ACTIVE_FLAG(Pop) && CAVE_SIZE_(Remain) >= 1) {
					CAVE_POP();
				}
			}
		}

		// pre patch exists
		if (ACTIVE_FLAG(PrePatch) && CAVE_SIZE_(Remain) >= a_preSize) {
			CAVE_PREPATCH();

			if (ACTIVE_FLAG(Push) && CAVE_SIZE_(Remain) >= 1) {
				CAVE_PUSH();
			}
		}

		// write detour code if not done yet
		CAVE_BRANCH();
		
		// post patch exists
		if (ACTIVE_FLAG(PostPatch) && CAVE_SIZE_(Remain) >= a_postSize) {
			CAVE_POSTPATCH();
			
			if (ACTIVE_FLAG(Pop) && CAVE_SIZE_(Remain) >= 1) {
				CAVE_POP();
			}
		}
		
#endif
		/* SMART_ALLOC END */

		
		// write detour code if not using SMART_ALLOC
		CAVE_BRANCH();
		
		/* CAVE END */

		
		/* ALLOCATE START */
		ALLOC_INIT();

		// prepare allocation
		if (ACTIVE_FLAG(Allocate)) {
			if (ACTIVE_FLAG(PrePatch)) {
				ALLOCATE(a_preSize);
				
				WRITE(CurrentPtr, a_prePatch, a_preSize);
				CURRENT_PTR_MOVE(a_preSize);
				
				TOGGLE_FLAG(PrePatch);
				FM("Applied pre patch");
			}

			if (ACTIVE_FLAG(Branch)) {
				CODEGEN_RESIZE(12);

				if (ACTIVE_FLAG(Push)) {
					CODEGEN_RESIZE(1);
				}

				if (ACTIVE_FLAG(Pop)) {
					CODEGEN_RESIZE(1);
				}
				
				CODEGEN_INIT();
				
				if (ACTIVE_FLAG(Push)) {
					VAR_(CodeGen).push(VAR_(CodeGen).rax);
					TOGGLE_FLAG(Push);
				}

				VAR_(CodeGen).mov(VAR_(CodeGen).rax, a_hookFunc);
				VAR_(CodeGen).call(VAR_(CodeGen).rax);

				if (ACTIVE_FLAG(Pop)) {
					VAR_(CodeGen).pop(VAR_(CodeGen).rax);
					TOGGLE_FLAG(Pop);
				}

				CODEGEN_READY();
				CURRENT_PTR_MOVE(SIZE_(CodeGen));

				TOGGLE_FLAG(Branch);
				FM("Applied branch from trampoline to function");
			}
			
			if (ACTIVE_FLAG(PostPatch)) {
				ALLOCATE(a_postSize);
				
				WRITE(CurrentPtr, a_postPatch, a_postSize);
				CURRENT_PTR_MOVE(a_postSize);

				TOGGLE_FLAG(PostPatch);
				FM("Applied post patch");
			}

			if (ACTIVE_FLAG(Return)) {
				ALLOCATE(5);

				WRITE_8(CurrentPtr, Impl::JMP);
				WRITE_32(CurrentPtr + 1, continueAddr - CurrentPtr - 5);
				CURRENT_PTR_MOVE(5);
				
				TOGGLE_FLAG(Return);
				FM("Applied return from trampoline to code");
			}
		}
		/* ALLOCATE END */

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
	/// <param name="a_prePatch"> to execute before calling branched function</param>
	/// <param name="a_preSize">Size of pre </param>
	/// <param name="a_postPatch"> to execute after returning from branched function</param>
	/// <param name="a_postSize">Size of post </param>
	/// <param name="a_preserve">Preserving rax by using 2 extra bytes</param>
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
		const std::uint64_t a_postSize = 0,
		const bool a_preserve = false
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

		return BranchToFunction_Impl<OFFSET_START, OFFSET_END>(
			resolvedAddr,
			a_hookFunc,
			a_prePatch,
			a_preSize,
			a_postPatch,
			a_postSize,
			a_preserve
		);
	}


	/// <summary>
	/// Packaged invocation to reduce parameter list's length
	/// </summary>
	/// <param name="a_hookdFunc">Hook intercept function address</param>
	/// <param name="a_instruction">Refer to BranchInstruction</param>
	/// <returns>Success indicator</returns>
	/// <remarks>Uses address library ( or REL )</remarks>
	template <std::uint64_t BASE_ID, std::uintptr_t OFFSET_START, std::uintptr_t OFFSET_END>
	bool BranchToFunction(const void* a_hookdFunc, const BranchInstruction a_instruction)
	{
		return BranchToFunction<BASE_ID, OFFSET_START, OFFSET_END>(
			reinterpret_cast<std::uintptr_t>(a_hookdFunc),
			a_instruction.PrePatch.Data,
			a_instruction.PrePatch.Size,
			a_instruction.PostPatch.Data,
			a_instruction.PostPatch.Size,
			a_instruction.PreserveRax
		);
	}


	/// <summary>
	/// Inject at address, apply patches, branch to trampoline and back
	/// </summary>
	/// <param name="a_hookFunc">Function address to branch to, use 0 to not branch any function ( only append patches )</param>
	/// <param name="a_prePatch"> to execute before calling branched function</param>
	/// <param name="a_preSize">Size of pre </param>
	/// <param name="a_postPatch"> to execute after returning from branched function</param>
	/// <param name="a_postSize">Size of post </param>
	/// <param name="a_preserve">Preserve rax by using 2 extra bytes</param>
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
		const std::uint64_t a_postSize = 0,
		const bool a_preserve = false
	)
	{
		return BranchToFunction_Impl<0, ADDRESS_END - ADDRESS_START>(
			ADDRESS_START,
			reinterpret_cast<std::uintptr_t>(a_hookFunc),
			a_prePatch,
			a_preSize,
			a_postPatch,
			a_postSize,
			a_preserve
		);
	}

	
	/// <summary>
	/// Packaged invocation to reduce parameter list's length
	/// </summary>
	/// <param name="a_hookFunc">Hook intercept function</param>
	/// <param name="a_instruction">Refer to BranchInstruction</param>
	/// <returns>Success indicator</returns>
	/// <remarks>Does not use address library ( or REL )</remarks>
	template <std::uintptr_t ADDRESS_START, std::uintptr_t ADDRESS_END>
	bool BranchToFunction(const void* a_hookFunc, const BranchInstruction a_instruction)
	{
		return BranchToFunction_Impl<0, ADDRESS_END - ADDRESS_START>(
			ADDRESS_START,
			reinterpret_cast<std::uintptr_t>(a_hookFunc),
			a_instruction.PrePatch.Data,
			a_instruction.PrePatch.Size,
			a_instruction.PostPatch.Data,
			a_instruction.PostPatch.Size,
			a_instruction.PreserveRax
		);
	}
	
	/* GENERAL IMPLEMENTATION END */
}


/* ALIAS START */
typedef DKUtil::Hook::BranchInstruction BranchInstruction;
/* ALIAS END */

/* UNDEFINE START */
#undef FM
#undef ALLOCATE
#undef CURRENT_PTR_MOVE
#undef CODEGEN
/* UNDEFINE END */

#pragma warning ( default : 4244 )

#endif
