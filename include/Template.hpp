#ifndef DKUTIL_TEMPLATE
#define DKUTIL_TEMPLATE

#include <memory>
#include <xstring>

// CommonLib
#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

// Json
#include "Json2Settings.h"
namespace J2S = Json2Settings;

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif

#define CTOR_COPY(CLASS)\
	CLASS(const CLASS&) = default;\
	CLASS& operator=(const CLASS&) = default;

#define CTOR_MOVE(CLASS)\
	CLASS(CLASS&&) = default;\
	CLASS& operator=(CLASS&&) = default;

#define CTOR_NO_COPY(CLASS)\
	CLASS(const CLASS&) = delete;\
	CLASS& operator=(const CLASS&) = delete;

#define CTOR_NO_MOVE(CLASS)\
	CLASS(CLASS&&) = delete;\
	CLASS& operator=(CLASS&&) = delete;

#define AssertNotNull(PTR)\
	if (!(PTR)) {\
		return false;\
	}

using namespace std::literals;

namespace logger = SKSE::log;


namespace DKUtil::Template
{
	template <class DERIVED>
	class SDM
	{
	public:
		static DERIVED* GetSingleton()
		{
			static DERIVED singleton;
			return std::addressof(singleton);
		}

		CTOR_NO_COPY(SDM)
		CTOR_NO_MOVE(SDM)
			
	protected:
		SDM() = default;
		virtual ~SDM() = default;
	};
}

template <class DERIVED>
using SDM = DKUtil::Template::SDM<DERIVED>;

#endif
