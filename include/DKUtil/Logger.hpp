#pragma once


/*
 * 1.2.0
 * Changed log level controls;
 * 
 * 1.1.0
 * F4SE integration;
 * 
 * 1.0.0
 * Basic spdlog implementation;
 *
 */


#define DKU_L_VERSION_MAJOR     1
#define DKU_L_VERSION_MINOR     2
#define DKU_L_VERSION_REVISION  0


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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


using namespace std::literals;


#define INFO(...)	{std::source_location src = std::source_location::current();	\
	spdlog::log(spdlog::source_loc{ src.file_name(), static_cast<int>(src.line()),	\
	src.function_name() }, spdlog::level::info, __VA_ARGS__);}    
#define DEBUG(...)	{std::source_location src = std::source_location::current();	\
	spdlog::log(spdlog::source_loc{ src.file_name(), static_cast<int>(src.line()),	\
	src.function_name() }, spdlog::level::debug, __VA_ARGS__);}    
#endif

#ifdef DKUTIL_TEST_RUN
#define DEBUG(...) INFO(__VA_ARGS__)
#endif

#define ERROR(...)															\
	{const auto errormsg = "ERROR\n\n"s + fmt::format(__VA_ARGS__);			\
	spdlog::default_logger_raw()->log(spdlog::source_loc{__FILE__, __LINE__,\
		SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__);			\
	MessageBoxA(nullptr, errormsg.c_str(), Plugin::NAME.data(), MB_OK);		\
	ExitProcess(-1);}


#define ENABLE_DEBUG spdlog::default_logger()->set_level(spdlog::level::debug);
#define DISABLE_DEBUG spdlog::default_logger()->set_level(spdlog::level::info);


#ifndef LOG_PATH

#if defined( F4SEAPI )
#define LOG_PATH "My Games\\Fallout4\\F4SE"sv
#else
#define LOG_PATH "My Games\\Skyrim Special Edition\\SKSE"sv
#endif

#endif


namespace DKUtil
{
	constexpr auto DKU_L_VERSION = DKU_L_VERSION_MAJOR * 10000 + DKU_L_VERSION_MINOR * 100 + DKU_L_VERSION_REVISION;
} // namespace DKUtil


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
		sink->set_pattern("[%i][%l](%s:%#) %v"s);
#else
		sink->set_pattern("[%D %T][%l](%s:%#) %v"s);
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
#else
		log->set_level(spdlog::level::info);
#endif
		log->flush_on(spdlog::level::debug);

		set_default_logger(std::move(log));

#if defined( F4SEAPI )
#define MODE	"Fallout 4"
#elif defined ( SKSEAPI )
#define MODE	"Skyrim Special Edition"
#else
#define MODE	"DKUtil"
#endif

		DEBUG("Debug Mode - {} {}", MODE, a_version);
	}


	inline void SetLevel(const spdlog::level::level_enum a_level)
	{
		spdlog::default_logger()->set_level(a_level);
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
