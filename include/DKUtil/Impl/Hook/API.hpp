#pragma once


#include "Assembly.hpp"
#include "Internal.hpp"
#include "Shared.hpp"
#include "Trampoline.hpp"


namespace DKUtil::Hook
{
	inline auto AddASMPatch(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const Xbyak::CodeGenerator* a_xbyak,
		const bool a_forward = true) noexcept
	{
		return AddASMPatch(a_address, a_offset, { a_xbyak->getCode(), a_xbyak->getSize() }, a_forward);
	}

	inline auto AddASMPatch(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const Patch* a_patch,
		const bool a_forward = true) noexcept
	{
		return AddASMPatch(a_address, a_offset, { a_patch->Data, a_patch->Size }, a_forward);
	}

	inline auto AddCaveHook(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const FuncInfo a_funcInfo,
		const Xbyak::CodeGenerator* a_prolog = nullptr,
		const Xbyak::CodeGenerator* a_epilog = nullptr,
		model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP) noexcept
	{
		return AddCaveHook(
			a_address, a_offset, a_funcInfo,
			a_prolog ? std::make_pair(a_prolog->getCode(), a_prolog->getSize()) : std::make_pair(nullptr, 0),
			a_epilog ? std::make_pair(a_epilog->getCode(), a_epilog->getSize()) : std::make_pair(nullptr, 0),
			a_flag);
	}

	inline auto AddCaveHook(
		const std::uintptr_t a_address,
		const offset_pair a_offset,
		const FuncInfo a_funcInfo,
		const Patch* a_prolog = nullptr,
		const Patch* a_epilog = nullptr,
		model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP) noexcept
	{
		return AddCaveHook(
			a_address, a_offset, a_funcInfo,
			a_prolog ? std::make_pair(a_prolog->Data, a_prolog->Size) : std::make_pair(nullptr, 0),
			a_epilog ? std::make_pair(a_epilog->Data, a_epilog->Size) : std::make_pair(nullptr, 0),
			a_flag);
	}

	inline auto AddVMTHook(
		const void* a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo a_funcInfo,
		const Xbyak::CodeGenerator* a_xbyak = nullptr) noexcept
	{
		return AddVMTHook(a_vtbl, a_index, a_funcInfo,
			a_xbyak ? std::make_pair(a_xbyak->getCode(), a_xbyak->getSize()) : std::make_pair(nullptr, 0));
	}

	inline auto AddVMTHook(
		const void* a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo a_funcInfo,
		const Patch* a_patch = nullptr) noexcept
	{
		return AddVMTHook(a_vtbl, a_index, a_funcInfo,
			a_patch ? std::make_pair(a_patch->Data, a_patch->Size) : std::make_pair(nullptr, 0));
	}

	inline auto AddIATHook(
		const char* a_moduleName,
		const char* a_methodName,
		const FuncInfo a_funcInfo,
		const Xbyak::CodeGenerator* a_xbyak = nullptr) noexcept
	{
		return AddIATHook(a_moduleName, a_methodName, a_funcInfo,
			a_xbyak ? std::make_pair(a_xbyak->getCode(), a_xbyak->getSize()) : std::make_pair(nullptr, 0));
	}

	inline auto AddIATHook(
		const char* a_moduleName,
		const char* a_methodName,
		const FuncInfo a_funcInfo,
		const Patch* a_patch = nullptr) noexcept
	{
		return AddIATHook(a_moduleName, a_methodName, a_funcInfo,
			a_patch ? std::make_pair(a_patch->Data, a_patch->Size) : std::make_pair(nullptr, 0));
	}
}  // namespace DKUtil::Hook
