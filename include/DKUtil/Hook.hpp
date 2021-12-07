#pragma once


/*
 * 1.9.2
 * CMake integration, inter-library integration;
 *
 * 1.9.1
 * Fixed a misrelocation where trampoline ptr may not be pointed to correct address within cave;
 * Fixed an error where branch may not be correctly initiated;
 * Fixed an misorder where flags may be disabled inappropriately;
 * Reordered the cave code layout;
 *
 * 1.9.0
 * Added SMART_ALLOC and CAVE related conditional macro, if enabled:
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
 * Added additional parameter into InjectAt<...>( a_hookFunc, a_prePatch, a_prePatchSize, a_postPatch, a_postPatchSize );
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


#include <cstdint>
#include <xbyak/xbyak.h>


#include "Logger.hpp"

#if ANNIVERSARY_EDITION
#include "external/versionlibdb.h"
#else
#include "external/versiondb.h"
#endif
#include "SKSE/API.h"

#define TRAMPOLINE					SKSE::GetTrampoline()
#define TRAMPOLINE_CAPACITY			(TRAMPOLINE).allocated_size()
#define ALLOCATE(SIZE)				TRAM_PTR						\
	= reinterpret_cast<std::uintptr_t>((TRAMPOLINE).allocate((SIZE)))

#define WRITE(ADDR, DATA)			REL::safe_write(ADDR, DATA)
#define WRITE_BUF(ADDR, DATA, SIZE)	REL::safe_write(ADDR, DATA, (SIZE))


#pragma region GENERAL DEFINE


#define DKUTIL_HOOK_NO_FUNCTION nullptr
#define TRAM_PTR				DKUtil::Hook::Impl::CurrentTrampolinePtr
#define CAVE_PTR				DKUtil::Hook::Impl::CurrentCavePtr

// flag
#define DEFINE_FLAG(FLAG_NAME)	bool hookFlag ## FLAG_NAME = false
#define ENABLE_FLAG(FLAG_NAME)	hookFlag ## FLAG_NAME = true
#define DISABLE_FLAG(FLAG_NAME)	hookFlag ## FLAG_NAME = false
#define FLAG(FLAG_NAME)			hookFlag ## FLAG_NAME

// memory
#define MOVE(PTR, DISP)			(PTR) += (DISP)
#define WRITE_MOVE(PTR, DATA)										\
	WRITE(PTR, DATA);												\
	MOVE(PTR, sizeof(DATA))
#define WRITE_BUF_MOVE(PTR, DATA, SIZE)								\
	WRITE_BUF(PTR, DATA, (SIZE));									\
	MOVE(PTR, (SIZE))
#define ALLOC_WRITE_MOVE(PTR, DATA)									\
	ALLOCATE(sizeof(DATA));											\
	WRITE_MOVE(PTR, DATA)
#define ALLOC_WRITE_BUF_MOVE(PTR, DATA, SIZE)						\
	ALLOCATE((SIZE));												\
	WRITE_BUF_MOVE(PTR, DATA, (SIZE))

// variables
#define VAR_(NAME)				hookLocal ## NAME
#define CAVE_VAR_(NAME)			VAR_(Cave ## NAME)

// cave
#define CAVE_INIT(SRC, SIZE)												\
	CAVE_PTR						= SRC;									\
	std::int32_t CAVE_VAR_(Detour)	= SRC;									\
	std::size_t	CAVE_VAR_(Remain)	= (SIZE) - 5;							\
	ALLOCATE(0);															\
	DEBUG("Cave: Placeholding {}B", (SIZE));								\
	std::uint32_t old{ 0 };													\
	VirtualProtect(reinterpret_cast<void*>(SRC), (SIZE),					\
		PAGE_EXECUTE_READWRITE, (PDWORD)std::addressof(old));				\
	std::fill_n(reinterpret_cast<std::uint8_t*>(SRC), (SIZE), Impl::NOP);	\
	VirtualProtect(reinterpret_cast<void*>(SRC), (SIZE),					\
		old, (PDWORD)std::addressof(old))

#define CAVE_DETOUR()												\
	WRITE_MOVE(CAVE_PTR, Impl::JMP);								\
	CAVE_VAR_(Detour) = TRAM_PTR - CAVE_PTR - sizeof(std::int32_t);	\
	WRITE_MOVE(CAVE_PTR, CAVE_VAR_(Detour));						\
	CAVE_VAR_(Remain) -= (sizeof(Impl::JMP) + sizeof(std::int32_t));\
	DEBUG("Cave: Detour -> trampoline"sv);							\
	DISABLE_FLAG(Detour)

#define CAVE_PREPATCH()										\
	WRITE_BUF_MOVE(CAVE_PTR, a_prePatch, a_prePatchSize);	\
	CAVE_VAR_(Remain) -= a_prePatchSize;					\
	DEBUG("Cave: PrePatch applied"sv);						\
	DISABLE_FLAG(PrePatch)

#define CAVE_POSTPATCH()								\
	WRITE_BUF(CAVE_PTR, a_postPatch, a_postPatchSize);	\
	CAVE_VAR_(Remain) -= a_postPatchSize;				\
	DEBUG("Cave: PostPatch applied"sv);					\
	DISABLE_FLAG(PostPatch)

#define CAVE_PUSH()								\
	WRITE_MOVE(CAVE_PTR, Impl::PUSH_RAX);		\
	CAVE_VAR_(Remain) -= sizeof(Impl::PUSH_RAX);\
	DEBUG("Cave: Rax preserved"sv);				\
	DISABLE_FLAG(Push)

#define CAVE_POP()								\
	WRITE_MOVE(CAVE_PTR, Impl::POP_RAX);		\
	CAVE_VAR_(Remain) -=  sizeof(Impl::POP_RAX);\
	DEBUG("Cave: Rax restored"sv);				\
	DISABLE_FLAG(Pop)


#pragma endregion GENERAL DEFINE


namespace DKUtil::Hook
{
#pragma region GLOBAL

	namespace Impl
	{
		using OpCode = std::uint8_t;

		constexpr OpCode NOP = 0x90;
		constexpr OpCode JMP = 0xE9;
		constexpr OpCode PUSH_RAX = 0x50;
		constexpr OpCode POP_RAX = 0x58;
		constexpr OpCode MOVABS_RAX[2] = { 0x48, 0xB8 };
		constexpr OpCode CALL_RAX[2] = { 0xFF, 0xD0 };


		struct BranchInstruction
		{
			const void* PrePatch;
			const std::size_t PrePatchSize;
			const void* PostPatch;
			const std::size_t PostPatchSize;
			const bool PreserveRax = false;
		};


		// managed pointer of current trampoline
		static std::uintptr_t CurrentTrampolinePtr = 0x0;
		static std::uintptr_t CurrentCavePtr = 0x0;

#pragma endregion GLOBAL


#pragma region CONDITIONAL IMPLEMENTATION

		// internal impl
		template <std::uint64_t CAVE_SIZE>
		void Branch_Internal(const std::uintptr_t a_resolvedAddr, const void* a_hookFunc = DKUTIL_HOOK_NO_FUNCTION,
							 const void* a_prePatch = nullptr, const std::size_t a_prePatchSize = 0,
							 const void* a_postPatch = nullptr, const std::size_t a_postPatchSize = 0,
							 const bool a_preserve = false)
		{
			DEBUG("DKUtil::Hook ready to work"sv);

			// flags
			DEFINE_FLAG(Detour);
			DEFINE_FLAG(Push);
			DEFINE_FLAG(Pop);
			DEFINE_FLAG(Return);
			DEFINE_FLAG(PrePatch);
			DEFINE_FLAG(PostPatch);
			DEFINE_FLAG(LargerPrePatch);
			DEFINE_FLAG(Branch);
			DEFINE_FLAG(Allocate);

			// pre patch exists
			if (a_prePatchSize)
			{
				ENABLE_FLAG(PrePatch);
				DEBUG("PrePatch: {}B", a_prePatchSize);
			}

			// post patch exists
			if (a_postPatchSize)
			{
				ENABLE_FLAG(PostPatch);
				DEBUG("PostPatch: {}B", a_postPatchSize);
			}

			// both patches exist, determine larger one
			if (FLAG(PrePatch) && FLAG(PostPatch) && a_prePatchSize >= a_postPatchSize) { ENABLE_FLAG(LargerPrePatch); }

			// hook function exists
			if (a_hookFunc)
			{
				ENABLE_FLAG(Detour);
				ENABLE_FLAG(Return);
				ENABLE_FLAG(Allocate);
				ENABLE_FLAG(Branch);
				DEBUG("Address: Destination {:x}", a_hookFunc);
			}

			// hook function DNE but patch(es) are too large
			if (!a_hookFunc && CAVE_SIZE < (a_prePatchSize + a_postPatchSize))
			{
				ENABLE_FLAG(Detour);
				ENABLE_FLAG(Return);
				ENABLE_FLAG(Allocate);
				DEBUG("Cave: No priority");
			}

			// .PreserveRax = true
			if (FLAG(Detour) && a_preserve)
			{
				ENABLE_FLAG(Push);
				ENABLE_FLAG(Pop);
				DEBUG("Rax: Preserve register"sv);
			}

			/* possible cave layout:
			 * push rax - 1
			 * pre patch - a_prePatchSize
			 * detour - 5
			 * post patch - a_postPatchSize
			 * pop rax - 1
			 */
			// recalculate cave size and define cave variables
			CAVE_INIT(a_resolvedAddr, CAVE_SIZE);

			// * SMART_ALLOC code
			// both patches exist, size sufficient, write both
			if (FLAG(PrePatch) && FLAG(PostPatch) && CAVE_VAR_(Remain) >= (a_prePatchSize + a_postPatchSize))
			{
				DEBUG("Cave: Full mode"sv);

				if (FLAG(Push) && CAVE_VAR_(Remain) >= (a_prePatchSize + a_postPatchSize + sizeof(PUSH_RAX)))
				{
					CAVE_PUSH();
				}

				if (FLAG(PrePatch)) { CAVE_PREPATCH(); }

				if (FLAG(Detour)) { CAVE_DETOUR(); }

				if (FLAG(PostPatch)) { CAVE_POSTPATCH(); }

				if (FLAG(Pop) && CAVE_VAR_(Remain) >= sizeof(POP_RAX)) { CAVE_POP(); }
			}

			// both patches exist, size insufficient, pick larger one
			if (FLAG(PrePatch) && FLAG(PostPatch) && (CAVE_VAR_(Remain) >= a_prePatchSize && CAVE_VAR_(Remain) >=
				a_postPatchSize && CAVE_VAR_(Remain) < (a_prePatchSize + a_postPatchSize)))
			{
				DEBUG("Cave: Priority mode"sv);

				if (FLAG(LargerPrePatch))
				{
					if (FLAG(Push) && CAVE_VAR_(Remain) >= (a_prePatchSize + sizeof(PUSH_RAX))) { CAVE_PUSH(); }

					CAVE_PREPATCH();
				}

				if (FLAG(Detour)) { CAVE_DETOUR(); }

				if (!FLAG(LargerPrePatch))
				{
					CAVE_POSTPATCH();

					if (FLAG(Pop) && CAVE_VAR_(Remain) >= sizeof(POP_RAX)) { CAVE_POP(); }
				}
			}
			
			// push rax onto stack
			if (FLAG(Push) && CAVE_VAR_(Remain) >= sizeof(PUSH_RAX)) { CAVE_PUSH(); }

			// apply detour code
			if (FLAG(Detour)) { CAVE_DETOUR(); }

			// pop rax cuz it's poppin
			if (FLAG(Pop) && CAVE_VAR_(Remain) >= sizeof(POP_RAX)) { CAVE_POP(); }

			// trampoline
			if (FLAG(Allocate))
			{
				if (FLAG(Push))
				{
					WRITE_MOVE(TRAM_PTR, Impl::PUSH_RAX);
					DEBUG("Rax: Push"sv);
					DISABLE_FLAG(Push);
				}

				if (FLAG(PrePatch))
				{
					ALLOC_WRITE_BUF_MOVE(TRAM_PTR, a_prePatch, a_prePatchSize);
					DEBUG("PrePatch: Applied"sv);
					DISABLE_FLAG(PrePatch);
				}

				if (FLAG(Branch))
				{
					ALLOC_WRITE_BUF_MOVE(TRAM_PTR, Impl::MOVABS_RAX, sizeof(Impl::MOVABS_RAX));
					ALLOC_WRITE_MOVE(TRAM_PTR, reinterpret_cast<std::uintptr_t>(a_hookFunc));
					ALLOC_WRITE_BUF_MOVE(TRAM_PTR, Impl::CALL_RAX, sizeof(Impl::CALL_RAX));
					DEBUG("BranchTarget: {}.{:x}", Version::PROJECT, reinterpret_cast<std::uintptr_t>(a_hookFunc));
				}

				if (FLAG(PostPatch))
				{
					ALLOC_WRITE_BUF_MOVE(TRAM_PTR, a_postPatch, a_postPatchSize);
					DEBUG("PostPatch: Applied"sv);
					DISABLE_FLAG(PostPatch);
				}

				if (FLAG(Pop))
				{
					WRITE_MOVE(TRAM_PTR, Impl::POP_RAX);
					DEBUG("Rax: Pop"sv);
					DISABLE_FLAG(Pop);
				}

				if (FLAG(Return))
				{
					ALLOC_WRITE_MOVE(TRAM_PTR, Impl::JMP);
					CAVE_VAR_(Detour) = CAVE_PTR - TRAM_PTR - sizeof(std::int32_t);
					ALLOC_WRITE_MOVE(TRAM_PTR, CAVE_VAR_(Detour));
					DEBUG("Trampoline: Detour -> Cave"sv);
					DISABLE_FLAG(Return);
				}

				DISABLE_FLAG(Allocate);
			}

			DEBUG("DKUtil::Hook hooked the hook with hooks. Behook"sv);
		}
	}

#pragma endregion CONDITIONAL IMPLEMENTATION


#pragma region GENERAL IMPLEMENTATION


	// ID + Offset
	template <std::uint64_t BASE_ID, std::ptrdiff_t OFFSET_START, std::ptrdiff_t OFFSET_END>
	constexpr void BranchToID(const void* a_hookFunc, const void* a_prePatch = nullptr,
							  const std::size_t a_prePatchSize = 0, const void* a_postPatch = nullptr,
							  const std::size_t a_postPatchSize = 0, const bool a_preserve = false)
	{
		static_assert(OFFSET_END > OFFSET_START, "OFFSET wrong order");
		static_assert((OFFSET_END - OFFSET_START) >= 5, "Insufficient cave size");

		VersionDb db;
		if (!db.Load()) { ERROR("Failed to load version database!"sv); }

		const auto resolvedAddr = reinterpret_cast<std::uintptr_t>(db.FindAddressById(BASE_ID)) + OFFSET_START;
		db.Clear();

		if (!resolvedAddr) { ERROR("Failed to resolve address by id {:x}", BASE_ID); }
		DEBUG("Address: Resolved {:x}", resolvedAddr);

		return Impl::Branch_Internal<OFFSET_END - OFFSET_START>(resolvedAddr, a_hookFunc, a_prePatch, a_prePatchSize,
																a_postPatch, a_postPatchSize, a_preserve);
	}


	template <std::uint64_t BASE_ID, std::ptrdiff_t OFFSET_START, std::ptrdiff_t OFFSET_END>
	constexpr void BranchToID(const void* a_hookFunc, const Impl::BranchInstruction a_instruction)
	{
		return BranchToID<BASE_ID, OFFSET_START, OFFSET_END>(a_hookFunc, a_instruction.PrePatch,
															 a_instruction.PrePatchSize, a_instruction.PostPatch,
															 a_instruction.PostPatchSize, a_instruction.PreserveRax);
	}


	template <std::uint64_t ADDR, std::ptrdiff_t OFFSET_START, std::ptrdiff_t OFFSET_END>
	constexpr void BranchToAddress(const void* a_hookFunc, const void* a_prePatch = nullptr, 
									const std::size_t a_prePatchSize = 0, const void* a_postPatch = nullptr,
									const std::size_t a_postPatchSize = 0, const bool a_preserve = false)
	{
		return Impl::Branch_Internal<OFFSET_END - OFFSET_START>(ADDR, a_hookFunc, a_prePatch, a_prePatchSize,
																a_postPatch, a_postPatchSize, a_preserve);
	}


	template <std::uint64_t ADDR, std::ptrdiff_t OFFSET_START, std::ptrdiff_t OFFSET_END>
	constexpr void BranchToAddress(const void* a_hookFunc, const Impl::BranchInstruction a_instruction)
	{
		return Impl::Branch_Internal<OFFSET_END - OFFSET_START>(ADDR, a_hookFunc, a_instruction.PrePatch,
																a_instruction.PrePatchSize, a_instruction.PostPatch,
																a_instruction.PostPatchSize, a_instruction.PreserveRax);
	}


	using BranchInstruction = Impl::BranchInstruction;

#pragma endregion GENERAL IMPLEMENTATION
}


#pragma region UNDEFINE & ALIAS

#undef TRAMPOLINE
#undef TRAMPOLINE_CAPACITY

#undef DEFINE_FLAG
#undef ENABLE_FLAG
#undef DISABLE_FLAG
#undef FLAG

#undef MOVE
#undef WRITE_BUF_MOVE
#undef WRITE_MOVE
#undef ALLOC_WRITE_BUF_MOVE
#undef ALLOC_WRITE_MOVE

#undef VAR_
#undef SIZE_

#undef ALLOCATE
#undef CODEGEN
#undef CODEGEN_INIT
#undef CODEGEN_RESIZE
#undef CODEGEN_READY

#undef CAVE_VAR_
#undef CAVE_SIZE_

#undef CAVE_INIT
#undef CAVE_DETOUR

#undef CAVE_PREPATCH
#undef CAVE_POSTPATCH
#undef CAVE_PUSH
#undef CAVE_POP

#pragma endregion UNDEFINE

#pragma warning ( default : 4244 )
