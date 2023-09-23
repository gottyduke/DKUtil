#pragma once

#include "assembly.hpp"
#include "internal.hpp"
#include "shared.hpp"
#include "trampoline.hpp"

namespace DKUtil::Hook
{
	/* @brief Apply assembly patch in the body of execution
	 * @param a_address : Memory address of the BEGINNING of target function
	 * @param a_offset : Offset pairs for <beginning, end> of cave entry from the head of function
	 * @param a_patch : Assembly patch
	 * @param a_forward : Skip the rest of NOPs until next valid opcode
	 * @returns ASMPatchHandle
	 */
	inline auto AddASMPatch(
		const std::uintptr_t        a_address,
		const offset_pair           a_offset,
		const Xbyak::CodeGenerator* a_xbyak,
		const bool                  a_forward = true) noexcept
	{
		return AddASMPatch(a_address, a_offset, std::make_pair(a_xbyak->getCode(), a_xbyak->getSize()), a_forward);
	}

	/* @brief Apply assembly patch in the body of execution
	 * @param a_address : Memory address of the BEGINNING of target function
	 * @param a_offset : Offset pairs for <beginning, end> of cave entry from the head of function
	 * @param a_patch : Assembly patch
	 * @param a_forward : Skip the rest of NOPs until next valid opcode
	 * @returns ASMPatchHandle
	 */
	inline auto AddASMPatch(
		const std::uintptr_t a_address,
		const offset_pair    a_offset,
		const Patch*         a_patch,
		const bool           a_forward = true) noexcept
	{
		return AddASMPatch(a_address, a_offset, std::make_pair(a_patch->Data, a_patch->Size), a_forward);
	}

	/* @brief Branch to hook function in the body of execution from target function.
	 * @param a_offset : Offset pairs for <beginning, end> of cave entry from the head of function
	 * @param a_address : Memory address of the BEGINNING of target function
	 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapper of hook function
	 * @param a_prolog : Prolog patch before detouring to hook function
	 * @param a_epilog : Epilog patch after returning from hook function
	 * @param a_flag : Specifies operation on cave hook
	 * @returns CaveHookHandle
	 */
	inline auto AddCaveHook(
		const std::uintptr_t         a_address,
		const offset_pair            a_offset,
		const FuncInfo               a_funcInfo,
		const Xbyak::CodeGenerator*  a_prolog,
		const Xbyak::CodeGenerator*  a_epilog,
		model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP) noexcept
	{
		return AddCaveHook(
			a_address, a_offset, a_funcInfo,
			a_prolog ? std::make_pair(a_prolog->getCode(), a_prolog->getSize()) : std::make_pair(nullptr, 0),
			a_epilog ? std::make_pair(a_epilog->getCode(), a_epilog->getSize()) : std::make_pair(nullptr, 0),
			a_flag);
	}

	/* @brief Branch to hook function in the body of execution from target function.
	 * @param a_offset : Offset pairs for <beginning, end> of cave entry from the head of function
	 * @param a_address : Memory address of the BEGINNING of target function
	 * @param a_funcInfo : FUNC_INFO or RT_INFO wrapper of hook function
	 * @param a_prolog : Prolog patch before detouring to hook function
	 * @param a_epilog : Epilog patch after returning from hook function
	 * @param a_flag : Specifies operation on cave hook
	 * @returns CaveHookHandle
	 */
	inline auto AddCaveHook(
		const std::uintptr_t         a_address,
		const offset_pair            a_offset,
		const FuncInfo               a_funcInfo,
		const Patch*                 a_prolog,
		const Patch*                 a_epilog,
		model::enumeration<HookFlag> a_flag = HookFlag::kSkipNOP) noexcept
	{
		return AddCaveHook(
			a_address, a_offset, a_funcInfo,
			a_prolog ? std::make_pair(a_prolog->Data, a_prolog->Size) : std::make_pair(nullptr, 0),
			a_epilog ? std::make_pair(a_epilog->Data, a_epilog->Size) : std::make_pair(nullptr, 0),
			a_flag);
	}

	inline auto AddVMTHook(
		const void*                 a_vtbl,
		const std::uint16_t         a_index,
		const FuncInfo              a_funcInfo,
		const Xbyak::CodeGenerator* a_xbyak) noexcept
	{
		return AddVMTHook(a_vtbl, a_index, a_funcInfo, std::make_pair(a_xbyak->getCode(), a_xbyak->getSize()));
	}

	inline auto AddVMTHook(
		const void*         a_vtbl,
		const std::uint16_t a_index,
		const FuncInfo      a_funcInfo,
		const Patch*        a_patch) noexcept
	{
		return AddVMTHook(a_vtbl, a_index, a_funcInfo, std::make_pair(a_patch->Data, a_patch->Size));
	}

	inline auto AddIATHook(
		std::string_view            a_moduleName,
		std::string_view            a_libraryName,
		std::string_view            a_importName,
		const FuncInfo              a_funcInfo,
		const Xbyak::CodeGenerator* a_xbyak) noexcept
	{
		return AddIATHook(a_moduleName, a_libraryName, a_importName, a_funcInfo, std::make_pair(a_xbyak->getCode(), a_xbyak->getSize()));
	}

	inline auto AddIATHook(
		std::string_view a_moduleName,
		std::string_view a_libraryName,
		std::string_view a_importName,
		const FuncInfo   a_funcInfo,
		const Patch*     a_patch) noexcept
	{
		return AddIATHook(a_moduleName, a_libraryName, a_importName, a_funcInfo, std::make_pair(a_patch->Data, a_patch->Size));
	}

	/* @brief Relocate a jmpsite with target hook function
	 * @brief This API exists for compatiblity reason with CLib-style invocations, hook enabled by default
	 * @param <N> : Length of source instruction
	 * @param a_src : Address of jmp instruction
	 * @param a_dst : Destination function
	 * @returns Transitive RelHookHandle that can be converted to F
	 */
	template <std::size_t N, typename F>
	inline auto write_branch(
		std::uintptr_t a_src,
		F              a_dst) noexcept
	{
		auto handle = AddRelHook<N, false>(a_src, unrestricted_cast<std::uintptr_t>(a_dst));
		handle->Enable();
		return std::move(*handle.get());
	}

	/* @brief Relocate a callsite with target hook function
	 * @brief This API exists for compatiblity reason with CLib-style invocations, hook enabled by default
	 * @param <N> : Length of source instruction
	 * @param a_src : Address of call instruction
	 * @param a_dst : Destination function
	 * @returns Transitive RelHookHandle that can be converted to F
	 */
	template <std::size_t N = 5, typename F>
		requires(model::concepts::dku_memory<F>)
	inline auto write_call(
		std::uintptr_t a_src,
		F              a_dst) noexcept
	{
		auto handle = AddRelHook<N, true>(a_src, unrestricted_cast<std::uintptr_t>(a_dst));
		handle->Enable();
		return std::move(*handle.get());
	}

	/* @brief Relocate a callsite with target hook function
	 * @brief This API preserves registers and sse registers across non-volatile call boundaries
	 * @param <N> : Length of source instruction
	 * @param a_src : Address of call instruction
	 * @param a_dst : Destination function
	 * @param a_regs : Regular registers to preserve as non volatile
	 * @param a_simd : SSE registers to preserve as non volatile
	 * @returns Transitive RelHookHandle that can be converted to F
	 */
	template <std::size_t N = 5, typename F>
		requires(model::concepts::dku_memory<F>)
	inline auto write_call_ex(
		std::uintptr_t        a_src,
		F                     a_dst,
		enumeration<Register> a_regs = { Register::NONE },
		enumeration<SIMD>     a_simd = { SIMD::NONE }) noexcept
	{
	}
}  // namespace DKUtil::Hook
