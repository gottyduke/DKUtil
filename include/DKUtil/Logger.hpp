#pragma once


#include <filesystem>
#include <map>
#include <ShlObj.h>
#include <WinUser.h>


#ifndef SPDLOG_H // in case other libraries already included <spdlog>

#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#define INFO(...)	SPDLOG_INFO(__VA_ARGS__)
#define DEBUG(...)	SPDLOG_DEBUG(__VA_ARGS__)

#else

#define INFO(...)	SPDLOG_INFO(__VA_ARGS__)
#ifndef NDEBUG
#define DEBUG(...)	SPDLOG_INFO(__VA_ARGS__)
#else
#define DEBUG(...)	void(0)
#endif

#endif


#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace std::literals;


#define ERROR(...)																\
	const auto errormsg = "ERROR\n"s + fmt::format(__VA_ARGS__);				\
	spdlog::default_logger_raw()->log(spdlog::source_loc{__FILE__, __LINE__,	\
		SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__);				\
	MessageBoxA(nullptr, errormsg.c_str(), Version::PROJECT.data(), MB_OK);		\
	ExitProcess(114514);

#ifndef LOG_PATH
#define LOG_PATH "My Games/Skyrim Special Edition/SKSE"sv
#endif


namespace DKUtil::Logger
{
	// From CommonLibSSE https://github.com/Ryan-rsm-McKenzie/CommonLibSSE
	inline std::optional<std::filesystem::path> log_directory()
	{
		wchar_t* buffer{ nullptr };
		const auto result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&CoTaskMemFree)> knownPath(buffer, CoTaskMemFree);
		if (!knownPath || result != S_OK) { return std::nullopt; }

		std::filesystem::path path = knownPath.get();
		path /= LOG_PATH;

		return path;
	}


	inline void Init(const std::string_view a_name, const std::string_view a_version)
	{
		auto path = log_directory();

		*path /= a_name;
		*path += ".log"sv;

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#ifndef NDEBUG
		sink->set_pattern("[%i](%s) %v"s);
#else
		sink->set_pattern("[%T](%s): %v"s);
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
#else
		log->set_level(spdlog::level::info);
#endif
		log->flush_on(spdlog::level::debug);

		set_default_logger(std::move(log));

		DEBUG("Debug Mode {}"sv, ANNIVERSARY_EDITION ? "Anniversary Edition"sv : "Special Edition"sv);
	}


	class Controller
	{
	public:
		using QueueOrder = int;


		struct SinkInfo
		{
			const std::string Name;
			const std::string Version;
		};


	private:
		std::map<SinkInfo, QueueOrder> sinks;
	};
} // namespace DKUtil::Logger
