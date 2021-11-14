#pragma once


#include <filesystem>
#include <map>
#include <ShlObj.h>
#include <WinUser.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


using namespace std::literals;


#define INFO(...)	SPDLOG_INFO(__VA_ARGS__)
#ifndef NDEBUG
#define DEBUG(...)	SPDLOG_INFO(__VA_ARGS__)
#else
#define DEBUG(...)
#endif
#define ERROR(...)									\
	const auto errormsg = std::string("ERROR\n") +	\
	fmt::format(__VA_ARGS__);						\
	SPDLOG_CRITICAL(errormsg);						\
	MessageBoxA(nullptr, errormsg.c_str(),			\
		Version::PROJECT.data(), MB_OK);			\
	ExitProcess(114514);


namespace DKUtil::Logger
{
	// From CommonLibSSE https://github.com/Ryan-rsm-McKenzie/CommonLibSSE
	inline std::optional<std::filesystem::path> log_directory()
	{
		wchar_t* buffer{nullptr};
		const auto result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&CoTaskMemFree)> knownPath(buffer, CoTaskMemFree);
		if (!knownPath || result != S_OK) { return std::nullopt; }

		std::filesystem::path path = knownPath.get();
		path /= "My Games/Skyrim Special Edition/SKSE"sv;
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

		DEBUG("Debug mode"sv);
	}


	class Controller
	{
	public:
		using QueueOrder = int;


		struct SinkInfo
		{
			const std::string_view Name;
			const std::string_view Version;
		};


	private:
		std::map<SinkInfo, QueueOrder> sinks;
	};
}
