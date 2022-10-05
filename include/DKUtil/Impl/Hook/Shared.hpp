#pragma once


#include "DKUtil/Impl/PCH.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"

#include <xbyak/xbyak.h>


#define AsAddress(PTR) std::bit_cast<std::uintptr_t>(PTR)
#define AsPointer(ADDR) std::bit_cast<void*>(ADDR)


#define REQUEST_ALLOC true
#define NO_ALLOC false
#define FORWARD_PTR true
#define NO_FORWARD false


#define ASM_MINIMUM_SKIP 2

#define CAVE_MINIMUM_BYTES 0x5

#ifndef CAVE_BUF_SIZE 1 << 7
#	define CAVE_BUF_SIZE 1 << 7
#endif


#define FUNC_INFO(FUNC) \
	DKUtil::Hook::FuncInfo { reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::function::GetFuncArgsCount(FUNC), #FUNC }
#define MEM_FUNC_INFO(FUNC) \
	DKUtil::Hook::FuncInfo { reinterpret_cast<std::uintptr_t>(FUNC), DKUtil::function::GetMemFuncArgsCount(FUNC), #FUNC }
#define RT_INFO(FUNC, NAME) \
	DKUtil::Hook::FuncInfo { FUNC, 0, NAME }

namespace DKUtil
{
	namespace Alias
	{
		using OpCode = std::uint8_t;
		using Disp8 = std::int8_t;
		using Disp16 = std::int16_t;
		using Disp32 = std::int32_t;
		using Imm8 = std::uint8_t;
		using Imm16 = std::uint16_t;
		using Imm32 = std::uint32_t;
		using Imm64 = std::uint64_t;
	}  // namesapce Alias


	namespace Hook
	{
		using namespace DKUtil::Alias;

		using REX = std::uint8_t;
		using ModRM = std::uint8_t;
		using SIndex = std::uint8_t;

		using unpacked_data = std::pair<const void*, std::size_t>;
		using offset_pair = std::pair<std::ptrdiff_t, std::ptrdiff_t>;


		template <typename data_t>
		concept dku_h_pod_t =
			std::is_integral_v<data_t> ||
			(std::is_standard_layout_v<data_t> && std::is_trivial_v<data_t>);


		enum class HookFlag : std::uint32_t
		{
			kNoFlag = 0,

			kSkipNOP = 1u << 0,              // skip NOPs
			kRestoreBeforeProlog = 1u << 1,  // apply stolens before prolog
			kRestoreAfterProlog = 1u << 2,   // apply stolens after prolog
			kRestoreBeforeEpilog = 1u << 3,  // apply stolens before epilog
			kRestoreAfterEpilog = 1u << 4,   // apply stolens after epilog
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
	}  // namespace Hook
}  // namespace DKUtil
