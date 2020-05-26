#ifndef DKUTIL_HOOK
#define DKUTIL_HOOK

/*!
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
 */

#define DKUTIL_HOOK_VERSION_MAJOR	1
#define DKUTIL_HOOK_VERSION_MINOR	9
#define DKUTIL_HOOK_VERSION_PATCH	1
#define DKUTIL_HOOK_VERSION_BETA	0

/*!
 * 1.9.1
 * Fixed an misrelocation where trampoline ptr may not be pointed to correct address within cave;
 * Fixed an error where branch may not be correctly initiated;
 * Fixed an misorder where flags may be disabled inappropriately;
 * Reordered the cave code layout
 * 
 * 1.9.0
 * Added SMART_ALLOC and CAVE related conditional macro, if enable:
 * - Attempt to write patches into code cave to reduce trampoline load if cave size satisfies;
 * - Attempt to skip trampoline allocation if hook function is null and cave size satisfies;
 * - Attempt to write rax-clean instructions into code cave if a_preserve and cave size satisfies;
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
 */

#pragma warning ( disable : 4244 )

#ifdef SKSE64

#include "skse64_common/SafeWrite.h"
#include "skse64_common/BranchTrampoline.h"

#include "common/IDebugLog.h"

#ifndef XBYAK_NO_OP_NAMES
#define XBYAK_NO_OP_NAMES
#endif

#include "xbyak/xbyak.h"

#include "../External/versiondb.h"


#ifdef DKUTIL_HOOK_VERBOSE
#define FM(FMT, ...)		gLog.FormattedMessage(FMT, __VA_ARGS__)
#else
#define FM(...)
#endif

#define WRITE(ADDR, DATA, SIZE)	SafeWriteBuf(ADDR, const_cast<void*>(DATA), (SIZE))
#define WRITE_8(ADDR, DATA_8)	SafeWrite8(ADDR, DATA_8)
#define WRITE_16(ADDR, DATA_16)	SafeWrite16(ADDR, DATA_16)
#define WRITE_32(ADDR, DATA_32)	SafeWrite32(ADDR, DATA_32)
#define TRAMPOLINE				&g_localTrampoline


#else


#include "SKSE/CodeGenerator.h"
#include "SKSE/API.h"


#ifdef DKUTIL_HOOK_VERBOSE
#define FM(FMT, ...)			SKSE::Impl::MacroLogger::VPrint("DKUTIL_HOOK", 0, SKSE::Logger::Level::kDebugMessage, (FMT), __VA_ARGS__)
#else
#define FM(...) 
#endif

#define WRITE(ADDR, DATA, SIZE)	SKSE::SafeWriteBuf(ADDR, DATA, (SIZE))
#define WRITE_8(ADDR, DATA_8)	SKSE::SafeWrite8(ADDR, DATA_8)
#define WRITE_16(ADDR, DATA_16)	SKSE::SafeWrite16(ADDR, DATA_16)
#define WRITE_32(ADDR, DATA_32)	SKSE::SafeWrite32(ADDR, DATA_32)
#define TRAMPOLINE				SKSE::GetTrampoline()

#endif


#pragma region GENERAL DEFINE


#define DEFINE_FLAG(FLAG_NAME)	bool hook_flag ## FLAG_NAME = false
#define ENABLE_FLAG(FLAG_NAME)	hook_flag ## FLAG_NAME = true
#define DISABLE_FLAG(FLAG_NAME)	hook_flag ## FLAG_NAME = false
#define FLAG(FLAG_NAME)			hook_flag ## FLAG_NAME

#define CURRENT_PTR				Impl::CurrentPtr
#define CURRENT_PTR_VOID		reinterpret_cast<void*>(CURRENT_PTR)
#define CURRENT_PTR_MOVE(SIZE)	(CURRENT_PTR) += (SIZE)

#define VAR_(NAME)		hook_local ## NAME
#define SIZE_(NAME)		VAR_(NAME ## Size)

#define ALLOCATE(SIZE)\
	FM("Will allocate %lluB", (SIZE));\
	(CURRENT_PTR) = reinterpret_cast<std::uintptr_t>((TRAMPOLINE)->Allocate(SIZE));

#define CODEGEN					Xbyak::CodeGenerator
#define CODEGEN_INIT(SIZE)		CODEGEN VAR_(CodeGen)((SIZE), (CURRENT_PTR_VOID))
#define CODEGEN_READY()\
	ALLOCATE(VAR_(CodeGen).getSize());\
	VAR_(CodeGen).ready();\
	CURRENT_PTR_MOVE(VAR_(CodeGen).getSize());\
	FM("Applied branch from trampoline to function");\
	DISABLE_FLAG(Branch);

#define CAVE_VAR_(NAME)		VAR_(Cave ## NAME)
#define CAVE_SIZE_(NAME)	CAVE_VAR_(NAME ## Size)

#define CAVE_INIT()\
	std::uintptr_t	CAVE_VAR_(Ptr) = a_resolvedAddr;\
	std::uintptr_t	CAVE_VAR_(Last) = a_resolvedAddr;\
	std::uint64_t	CAVE_SIZE_(Remain) = CaveSize;\
	(CURRENT_PTR) = reinterpret_cast<std::uintptr_t>((TRAMPOLINE)->Allocate(0));
#define CAVE_PTR_MOVE(SIZE)		CAVE_VAR_(Ptr) += (SIZE);
#define CAVE_SIZE_MOVE(SIZE)	CAVE_SIZE_(Remain) += (SIZE);
#define CAVE_RESIZE(SIZE)\
	CAVE_PTR_MOVE((SIZE));\
	CAVE_SIZE_MOVE(-static_cast<int>((SIZE)));\
	FM("[SMART_ALLOC] Remain %lluB / %lluB | Ptr -> 0x%04x", CAVE_SIZE_(Remain), CaveSize, CAVE_VAR_(Ptr) - a_resolvedAddr);

#define CAVE_DETOUR()\
	WRITE_8(CAVE_VAR_(Ptr), Impl::JMP);\
	WRITE_32(CAVE_VAR_(Ptr) + 1, (CURRENT_PTR)-CAVE_VAR_(Ptr) - 5);\
	CAVE_PTR_MOVE(5);\
	CAVE_VAR_(Last) = CAVE_VAR_(Ptr);\
	FM("Applied detour from code to trampoline");\
	DISABLE_FLAG(Detour);

#ifdef DKUTIL_HOOK_SMART_ALLOC

#pragma region SMART_ALLOC DEFINE

#define CAVE_PREPATCH()\
	WRITE(CAVE_VAR_(Ptr), a_prePatch, a_preSize);\
	CAVE_RESIZE(a_preSize);\
	FM("[SMART_ALLOC] Applied prepatch in cave");\
	DISABLE_FLAG(PrePatch);
#define CAVE_POSTPATCH()\
	WRITE(CAVE_VAR_(Ptr), a_postPatch, a_postSize);\
	CAVE_RESIZE(a_postSize);\
	FM("[SMART_ALLOC] Applied postpatch in cave");\
	DISABLE_FLAG(PostPatch);
#define CAVE_PUSH()\
	WRITE_8(CAVE_VAR_(Ptr), Impl::PUSH_RAX);\
	CAVE_RESIZE(1);\
	FM("[SMART_ALLOC] Applied preserve prefix in cave");\
	DISABLE_FLAG(Push);
#define CAVE_POP()\
	WRITE_8(CAVE_VAR_(Ptr), Impl::POP_RAX);\
	CAVE_RESIZE(1);\
	FM("[SMART_ALLOC] Applied preserve suffix in cave");\
	DISABLE_FLAG(Pop);

#pragma endregion SMART_ALLOC DEFINE

#endif


#pragma endregion GENERAL DEFINE


namespace DKUtil::Hook
{
#pragma region GLOBAL

	namespace Impl
	{
		constexpr std::uint8_t NOP = 0x90;
		constexpr std::uint8_t JMP = 0xE9;
		constexpr std::uint8_t PUSH_RAX = 0x50;
		constexpr std::uint8_t POP_RAX = 0x58;

		/// <summary>
		/// Managed pointer of current utilizing trampoline
		/// </summary>
		static std::uintptr_t CurrentPtr = 0x0;
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
	/// Get current pointer of the managed trampoline
	/// </summary>
	/// <returns>Current pointer of trampoline</returns>
	std::uintptr_t GetCurrentPtr() noexcept { return Impl::CurrentPtr; }


	/// <summary>
	/// Caution ! Resets the pointer of current managed trampoline
	/// </summary>
	/// <remarks>
	/// After resetting pointer,
	/// author must handle trampoline allocation oneself
	/// and keep track of the usage and address of such
	/// </remarks>
	void ResetPtr() noexcept { Impl::CurrentPtr = 0x0; }

#pragma endregion GLOBAL


#pragma region CONDITIONAL IMPLEMENTATION

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

		// * setup flags

		// pre patch exists
		if (a_prePatch && a_preSize) {
			ENABLE_FLAG(PrePatch);
		}

		// post patch exists
		if (a_postPatch && a_postSize) {
			ENABLE_FLAG(PostPatch);
		}

		// both patches exist, determine larger one
		if (FLAG(PrePatch) && FLAG(PostPatch) 
			&& a_preSize >= a_postSize) {
			ENABLE_FLAG(LargerPrePatch);
		}

		// hook function exists
		if (a_hookFunc) {
			ENABLE_FLAG(Detour);
			ENABLE_FLAG(Return);
			ENABLE_FLAG(Allocate);
			ENABLE_FLAG(Branch);
		}

		// hook function DNE but patch(es) are too large
		if (!a_hookFunc && CaveSize < a_preSize + a_postSize) {
			ENABLE_FLAG(Detour);
			ENABLE_FLAG(Return);
			ENABLE_FLAG(Allocate);
		}

		// .PreserveRax = true
		if (FLAG(Detour) && a_preserve) {
			ENABLE_FLAG(Push);
			ENABLE_FLAG(Pop);
		}

		// log entries
		FM("INJECT: %p", a_resolvedAddr);

		if (FLAG(Detour)) {
			FM("HOOK: %p", a_hookFunc);
		}

		if (FLAG(Push) && FLAG(Pop)) {
			FM("RAX: preserve register");
		}

		if (FLAG(PrePatch)) {
			FM("PrePatch: Exists");
		}

		if (FLAG(PostPatch)) {
			FM("PostPatch: Exists");
		}

		// * cave code

		/* possible cave layout:
		 * push rax - 1
		 * pre patch - a_preSize
		 * detour - 5
		 * post patch - a_postSize
		 * pop rax - 1
		 */

		FM("Writing %llu NOP into code cave", CaveSize);
		for (auto i = 0; i < CaveSize; ++i) {
			WRITE_8(a_resolvedAddr + i, Impl::NOP);
		}

		// recalculate cave size and define cave variables
		CAVE_INIT();

		// reserve 5 bytes for detour code
		if (FLAG(Detour)) {
			CAVE_SIZE_(Remain) += (-5);
		}


		// * SMART_ALLOC code

#ifdef DKUTIL_HOOK_SMART_ALLOC

		// both patches exist, size sufficient, write both
		if (FLAG(PrePatch) && FLAG(PostPatch) && 
			CAVE_SIZE_(Remain) >= a_preSize + a_postSize) {
			if (FLAG(Push) && CAVE_SIZE_(Remain) > a_preSize + a_postSize) {
				CAVE_PUSH();
			}

			if (FLAG(PrePatch)) {
				CAVE_PREPATCH();
			}

			if (FLAG(Detour)) {
				CAVE_DETOUR();
			}

			if (FLAG(PostPatch)) {
				CAVE_POSTPATCH();
			}

			if (FLAG(Pop) && CAVE_SIZE_(Remain) >= 1) {
				CAVE_POP();
			}
		}

		// both patches exist, size insufficient, pick larger one
		if (FLAG(PrePatch) && FLAG(PostPatch) && 
			(CAVE_SIZE_(Remain) >= a_preSize && 
			CAVE_SIZE_(Remain) >= a_postSize &&
			CAVE_SIZE_(Remain) < a_preSize + a_postSize)) {
			if (FLAG(LargerPrePatch)) {
				if (FLAG(Push) && CAVE_SIZE_(Remain) > a_preSize) {
					CAVE_PUSH();
				}

				CAVE_PREPATCH();
			}
			
			if (FLAG(Detour)) {
				CAVE_DETOUR();
			}
			
			if (!FLAG(LargerPrePatch)) {
				CAVE_POSTPATCH();
				
				if (FLAG(Pop) && CAVE_SIZE_(Remain) >= 1) {
					CAVE_POP();
				}
			}
		}

		// write preserve prefix if not done yet
		if (FLAG(Push) && (CAVE_SIZE_(Remain) > a_preSize || CAVE_SIZE_(Remain) > a_postSize)) {
			CAVE_PUSH();
		}
		
		// pre patch exists
		if (FLAG(PrePatch) && CAVE_SIZE_(Remain) >= a_preSize) {
			CAVE_PREPATCH();
		}

		// write detour code if not done yet
		if (FLAG(Detour)) {
			CAVE_DETOUR();
		}

		// post patch exists
		if (FLAG(PostPatch) && CAVE_SIZE_(Remain) >= a_postSize) {
			CAVE_POSTPATCH();
		}

		// write preserve suffix if not done yet
		if (FLAG(Pop) && CAVE_SIZE_(Remain) >= 1) {
			CAVE_POP();
		}
		
#endif

		// write detour code if not using SMART_ALLOC
		if (FLAG(Detour)) {
			CAVE_DETOUR();
		}
		
		// * allocation code

		// prepare allocation
		if (FLAG(Allocate)) {
			if (FLAG(Push)) {
				ALLOCATE(1);
				WRITE_8(CURRENT_PTR, Impl::PUSH_RAX);
				CURRENT_PTR_MOVE(1);
				FM("Applied preserve prefix");
				DISABLE_FLAG(Push);
			}
			
			if (FLAG(PrePatch)) {
				ALLOCATE(a_preSize);
				WRITE(CURRENT_PTR, a_prePatch,a_preSize);
				CURRENT_PTR_MOVE(a_preSize);
				FM("Applied pre patch");
				DISABLE_FLAG(PrePatch);
			}
			
			if (FLAG(Branch)) {
				CODEGEN_INIT(12);
				
				VAR_(CodeGen).mov(VAR_(CodeGen).rax, a_hookFunc);
				VAR_(CodeGen).call(VAR_(CodeGen).rax);
				
				CODEGEN_READY();
			}
			
			if (FLAG(PostPatch)) {
				ALLOCATE(a_postSize);
				WRITE(Impl::CurrentPtr, a_postPatch, a_postSize);
				CURRENT_PTR_MOVE(a_postSize);
				FM("Applied post patch");
				DISABLE_FLAG(PostPatch);
			}
			
			if (FLAG(Pop)) {
				ALLOCATE(1);
				WRITE_8(CURRENT_PTR, Impl::POP_RAX);
				CURRENT_PTR_MOVE(1);
				FM("Applied preserve suffix");
				DISABLE_FLAG(Pop);
			}
			
			if (FLAG(Return)) {
				ALLOCATE(5);
				WRITE_8(CURRENT_PTR, Impl::JMP);
				WRITE_32(CURRENT_PTR + 1, CAVE_VAR_(Last) - CURRENT_PTR - 5);
				CURRENT_PTR_MOVE(5);
				FM("Applied return from trampoline to code");
				DISABLE_FLAG(Return);
			}
			
			DISABLE_FLAG(Allocate);
		}


#ifdef SKSE64
		FM("===SKSE64 Impl End===");
#else
		FM("===CommonLib Impl End===");
#endif

		return true;
		}

#pragma endregion CONDITIONAL IMPLEMENTATION


#pragma region GENERAL IMPLEMENTATION

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
		bool BranchToFunction(const std::uintptr_t a_hookdFunc, const BranchInstruction a_instruction)
		{
			return BranchToFunction<BASE_ID, OFFSET_START, OFFSET_END>(
				a_hookdFunc,
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
/// <param name="a_hookFunc">Hook intercept function</param>
/// <param name="a_instruction">Refer to BranchInstruction</param>
/// <returns>Success indicator</returns>
/// <remarks>Does not use address library ( or REL )</remarks>
		template <std::uintptr_t ADDRESS_START, std::uintptr_t ADDRESS_END>
		bool BranchToFunction(const std::uintptr_t a_hookFunc, const BranchInstruction a_instruction)
		{
			return BranchToFunction_Impl<0, ADDRESS_END - ADDRESS_START>(
				ADDRESS_START,
				a_hookFunc,
				a_instruction.PrePatch.Data,
				a_instruction.PrePatch.Size,
				a_instruction.PostPatch.Data,
				a_instruction.PostPatch.Size,
				a_instruction.PreserveRax
			);
		}

#pragma endregion GENERAL IMPLEMENTATION
		}


#pragma region UNDEFINE & ALIAS

#undef FM
		//#undef WRITE
		//#undef WRITE_8
		//#undef WRITE_16
		//#undef WRITE_32
#undef TRAMPOLINE

#undef DEFINE_FLAG
#undef TOGGLE_FLAG
#undef ACTIVE_FLAG
#undef CURRENT_PTR_MOVE

#undef VAR_
#undef SIZE_

#undef ALLOC_INIT
#undef ALLOCATE
#undef CODEGEN
#undef CODEGEN_INIT
#undef CODEGEN_RESIZE
#undef CODEGEN_READY

#undef CAVE_VAR_
#undef CAVE_SIZE_

#undef CAVE_INIT
#undef CAVE_PTR_MOVE
#undef CAVE_SIZE_MOVE
#undef CAVE_RESIZE
#undef CAVE_DETOUR

#undef CAVE_PREPATCH
#undef CAVE_POSTPATCH
#undef CAVE_PUSH
#undef CAVE_POP

		typedef DKUtil::Hook::BranchInstruction BranchInstruction;

#pragma endregion UNDEFINE

#pragma warning ( default : 4244 )

#endif
