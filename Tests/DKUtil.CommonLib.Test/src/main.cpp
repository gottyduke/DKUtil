#include "version.h"

#include "Hooks.h"

#include "SKSE/API.h"


namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			{
				break;
			}
		default:;
		}
	}
}


extern "C"
{
	bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		SKSE::Logger::OpenRelative(FOLDERID_Documents, L"\\My Games\\Skyrim Special Edition\\SKSE\\DKUtil.CommonLib.Test.log");
		SKSE::Logger::UseLogStamp(true);
#ifdef DEBUG
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kVerboseMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kVerboseMessage);
#else
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kMessage);
		SKSE::Logger::TrackTrampolineStats(true);
#endif

		_MESSAGE("DKUtil.CommonLib.Test v%s", DKUT_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "DKUT";
		a_info->version = DKUT_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			_FATALERROR("Loaded in editor, marking as incompatible!\n");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver <= SKSE::RUNTIME_1_5_39) {
			_FATALERROR("Unsupported runtime version %s!", ver.GetString().c_str());
			return false;
		}

		return true;
	}


	bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		_MESSAGE("DKUtil.CommonLib.Test loaded");

		if (!Init(a_skse)) {
			return false;
		}

		const auto* const messaging = SKSE::GetMessagingInterface();
		if (messaging->RegisterListener("SKSE", MessageHandler)) {
			_MESSAGE("Messaging interface registration successful");
		} else {
			_FATALERROR("Messaging interface registration failed!\n");
			return false;
		}

		if (!SKSE::AllocTrampoline(1 << 6)) {
			_FATALERROR("Failed to allocate trampoline");
			return false;
		}

		if (Hooks::InstallHooks()) {
			_MESSAGE("Hooks installed successfully");
		} else {
			_FATALERROR("Failed to install hooks!\n");
			return false;
		}

		return true;
	}
};
