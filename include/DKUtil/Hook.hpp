#pragma once


/*
 * 2.6.0
 * Added pattern scan modules;
 * Completed trampoline implementation;
 * 
 * 2.5.3
 * Fixed stack alignment, force 0x20 allocation;
 * 
 * 2.5.2
 * Adaptation of file structural changes;
 * 
 * 2.5.1
 * Fixed the issue where CavePtr hasn't been forwarded but calculation is done;
 * Fixed the issue kSkipNOP not actually working;
 * 
 * 2.5.0
 * Added new assembly structs;
 * Removed redundant structs;
 * Removed CAVE_MAXIMUM_BYTES;
 * Added register preserve functions;
 * Changed CaveHookFlag to HookFlag;
 * 
 * 2.4.1
 * Added runtime counterparts for NG;
 * 
 * 2.4.0
 * Minor formatting and refractoring for NG;
 *
 * 2.3.3
 * F4SE integration;
 * CaveHook auto patch the stolen opcodes;
 * 
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
 * Removed success checks during the writing procedure, due to F4SE64 implementation has no returning value;
 * Restructured two separate implementations into conditional compilation;
 * Resolve address differently based on which implementation is used;
 * Added derived prototype of BranchToFunction<...>(...) without address library usage;
 * Added support for ( a_hookFunc = 0 ) so that patches will be applied without branching to any function;
 * Added default value for patch related parameters, if branching is done without any patches;
 * Fixed various rvalue const cast errors within F4SE64 implementation;
 * Added prototype of GetCurrentPtr();
 * Added prototype of ResetPtr();
 * Renamed some template parameters;
 * Changed some local variables to constexpr;
 * Added F4SE64 implementation;
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


#define DKU_H_VERSION_MAJOR 2
#define DKU_H_VERSION_MINOR 5
#define DKU_H_VERSION_REVISION 2


#pragma warning(push)
#pragma warning(disable: 4244)


#include "Impl/pch.hpp"


namespace DKUtil
{
	constexpr auto DKU_H_VERSION = DKU_H_VERSION_MAJOR * 10000 + DKU_H_VERSION_MINOR * 100 + DKU_H_VERSION_REVISION;
}  // namespace DKUtil


#include "Impl/Hook/shared.hpp"

#include "Impl/Hook/api.hpp"


namespace DKUtil::Alias
{
	using Patch = DKUtil::Hook::Patch;
	using HookHandle = std::unique_ptr<DKUtil::Hook::HookHandle>;
	using CaveHandle = DKUtil::Hook::CaveHookHandle;
	using VMTHandle = DKUtil::Hook::VMTHookHandle;
	using IATHandle = DKUtil::Hook::IATHookHandle;

	using Reg = DKUtil::Hook::Assembly::Register;
	template <class... Rules>
	using Pattern = DKUtil::Hook::Assembly::Pattern::PatternMatcher<Rules...>;

	using HookFlag = DKUtil::Hook::HookFlag;
}  // namespace DKUtil::Alias


#pragma warning(pop)
