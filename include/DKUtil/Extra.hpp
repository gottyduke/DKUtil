#pragma once


#define DKU_E_VERSION_MAJOR 1
#define DKU_E_VERSION_MINOR 0
#define DKU_E_VERSION_REVISION 0


#if defined(SKSEAPI)


#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>


#include "Impl/pch.hpp"

#include "Impl/Extra/xconsole.hpp"
#include "Impl/Extra/xserialize.hpp"


#define DKU_X_VERSION_MAJOR 1
#define DKU_X_VERSION_MINOR 0
#define DKU_X_VERSION_REVISION 0


namespace DKUtil
{
	constexpr auto DKU_X_VERSION = DKU_X_VERSION_MAJOR * 10000 + DKU_X_VERSION_MINOR * 100 + DKU_X_VERSION_REVISION;
}  // namespace DKUtil


namespace DKUtil
{
	template <typename T, dku::string::static_string H>
	using serializable = DKUtil::serialization::Serializable<T, H>;

	using ResolveType = DKUtil::serialization::ResolveOrder;
}  // DKUtil


#endif