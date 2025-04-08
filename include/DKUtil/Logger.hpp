#pragma once

/**
 * 1.2.5
 * Changed ERROR, FATAL predefined prompt texts to less verbose;
 * 
 * 1.2.4
 * Fixed some macros;
 * 
 * 1.2.3
 * Preceded ERROR logging macro;
 * Added FATAL logging macro;
 * Various code refactoring;
 * 
 * 1.2.2
 * Adaptation of file structural changes;
 * Init log statement changed to INFO level;
 * 
 * 1.2.1
 * Macro expansion;
 * 
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

#include "Impl/pch.hpp"

#define DKU_L_VERSION_MAJOR 1
#define DKU_L_VERSION_MINOR 2
#define DKU_L_VERSION_REVISION 4

// fixme: decouple PROJECT_NAME requirement
#if !defined(PROJECT_NAME)
#	define PROJECT_NAME Plugin::NAME.data()
#endif

#if defined(DKU_CONSOLE)
#	include <spdlog/sinks/stdout_color_sinks.h>
#else
#	include <spdlog/sinks/basic_file_sink.h>
#endif

#include <spdlog/spdlog.h>

#define __LOG(LEVEL, fmt_s, ...)                                                                \
	{                                                                                           \
		const auto src = DKUtil::Logger::detail::make_current(std::source_location::current()); \
		spdlog::log(src, spdlog::level::LEVEL, fmt_s __VA_OPT__(, ) __VA_ARGS__);               \
	}
#define __REPORT(DO_EXIT, PROMPT, ...)                                         \
	{                                                                          \
		__LOG(critical, __VA_ARGS__);                                          \
		const auto src = std::source_location::current();                      \
		const auto fmt_s = fmt::format(DKUtil::Logger::detail::prompt::PROMPT, \
			DKUtil::Logger::detail::short_file(src.file_name()),               \
			src.line(), src.function_name(), fmt::format(__VA_ARGS__));        \
		DKUtil::Logger::detail::report_error(DO_EXIT, fmt_s);                  \
	}
#define INFO(fmt_s, ...) __LOG(info, fmt_s __VA_OPT__(, ) __VA_ARGS__)
#define DEBUG(fmt_s, ...) __LOG(debug, fmt_s __VA_OPT__(, ) __VA_ARGS__)
#define TRACE(fmt_s, ...) __LOG(trace, fmt_s __VA_OPT__(, ) __VA_ARGS__)
#define WARN(fmt_s, ...) __LOG(warn, fmt_s __VA_OPT__(, ) __VA_ARGS__)
#define ERROR(fmt_s, ...) __REPORT(false, error, fmt_s __VA_OPT__(, ) __VA_ARGS__)
#define FATAL(fmt_s, ...) __REPORT(true, fatal, fmt_s __VA_OPT__(, ) __VA_ARGS__)

#define ENABLE_DEBUG DKUtil::Logger::SetLevel(spdlog::level::debug);
#define DISABLE_DEBUG DKUtil::Logger::SetLevel(spdlog::level::info);

#if !defined(LOG_PATH)

#	if defined(F4SEAPI)
#		define IS_NG REL::Module::IsNG()
#		define IS_F4 REL::Module::IsF4()
#		define IS_VR REL::Module::IsVR()
#		define LOG_PATH "My Games\\Fallout4\\F4SE"sv
#		define LOG_PATH_VR "My Games\\Fallout4VR\\F4SE"sv
#	elif defined(SFSEAPI)
#		define LOG_PATH "My Games\\Starfield\\SFSE\\Logs"sv
#	elif defined(SKSEAPI)
#		define IS_AE REL::Module::IsAE()
#		define IS_SE REL::Module::IsSE()
#		define IS_VR REL::Module::IsVR()
#		define IS_GOG !IS_VR && !std::filesystem::exists("steam_api64.dll")
#		define LOG_PATH "My Games\\Skyrim Special Edition\\SKSE"sv
#		define LOG_PATH_GOG "My Games\\Skyrim Special Edition GOG\\SKSE"sv
#		define LOG_PATH_VR "My Games\\Skyrim VR\\SKSE"sv
#	else
#		define LOG_PATH ""
#		define PLUGIN_MODE
#	endif

#endif

#if defined(NDEBUG)
#	define DKU_L_DISABLE_INTERNAL_DEBUGGING
#endif

#if !defined(DKU_L_DISABLE_INTERNAL_DEBUGGING)

#	define __INFO(...) INFO(__VA_ARGS__)
#	define __DEBUG(...) DEBUG(__VA_ARGS__)
#	define __TRACE(...) TRACE(__VA_ARGS__)
#	define __WARN(...) WARN(__VA_ARGS__)

#else

#	define __INFO(...) void(0)
#	define __DEBUG(...) void(0)
#	define __TRACE(...) void(0)
#	define __WARN(...) void(0)

#endif

namespace DKUtil
{
	constexpr auto DKU_L_VERSION = DKU_L_VERSION_MAJOR * 10000 + DKU_L_VERSION_MINOR * 100 + DKU_L_VERSION_REVISION;
}  // namespace DKUtil

namespace DKUtil::Logger
{
	namespace detail
	{
		// file, line, callstack, error
		namespace prompt
		{
			inline constexpr auto error = FMT_STRING(
				"Error occurred at code:\n[{}:{}] -> {}\n\nDetail:\n{}\n\n"
				"Continuing may result in undesired behavior.\n"
				"Exit process? (yes highly suggested)\n\n");

			inline constexpr auto fatal = FMT_STRING(
				"Error occurred at code:\n[{}:{}] -> {}\n\nDetail:\n{}\n\n"
				"Process cannot continue and will now exit.\n");
		}  // namespace prompt

		// From CommonLibSSE https://github.com/Ryan-rsm-McKenzie/CommonLibSSE
		inline std::filesystem::path docs_directory() noexcept
		{
			wchar_t*                                               buffer{ nullptr };
			const auto                                             result = ::SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
			std::unique_ptr<wchar_t[], decltype(&::CoTaskMemFree)> knownPath{ buffer, ::CoTaskMemFree };

			return (!knownPath || result != S_OK) ? std::filesystem::path{} : std::filesystem::path{ knownPath.get() };
		}

		inline spdlog::source_loc make_current(std::source_location a_loc) noexcept
		{
			return spdlog::source_loc{ a_loc.file_name(), static_cast<int>(a_loc.line()), a_loc.function_name() };
		}

		inline void report_error(bool a_fatal, std::string_view a_fmt)  // noexcept
		{
			if (a_fatal) {
				::MessageBoxA(nullptr, a_fmt.data(), Plugin::NAME.data(), MB_OK | MB_ICONSTOP);
			} else {
				auto result = ::MessageBoxA(nullptr, a_fmt.data(), PROJECT_NAME, MB_YESNO | MB_ICONEXCLAMATION);
				if (result != IDYES) {
					return;
				}
			}

			::TerminateProcess(::GetCurrentProcess(), 'FAIL');
		}

		inline constexpr const char* short_file(const char* path)
		{
			const char* file = path;
			while (*path) {
				if (*path++ == '\\') {
					file = path;
				}
			}
			return file;
		}
	}  // namespace detail

	inline void Init(const std::string_view a_name, const std::string_view a_version) noexcept
	{
		std::filesystem::path path{};
#if defined(F4SEAPI)
		path = detail::docs_directory();
		path /= IS_VR ? LOG_PATH_VR : LOG_PATH;
#elif defined(SKSEAPI)
		path = detail::docs_directory();
		path /= IS_VR ? LOG_PATH_VR : (IS_GOG ? LOG_PATH_GOG : LOG_PATH);
#elif defined(SFSEAPI)
		path = detail::docs_directory();
		path /= LOG_PATH;
#elif defined(PLUGIN_MODE)
		path = std::move(std::filesystem::current_path());
		path /= LOG_PATH;
#endif
		path /= a_name;
		path += ".log"sv;

#if defined(DKU_CONSOLE)
		auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#else
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
#endif

#if !defined(NDEBUG)
		sink->set_pattern("[%i][%l](%s:%#) %v"s);
#else
		sink->set_pattern("[%D %T][%l](%s:%#) %v"s);
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#if !defined(NDEBUG)
		log->set_level(spdlog::level::trace);
#else
		log->set_level(spdlog::level::info);
#endif
		log->flush_on(spdlog::level::trace);

		set_default_logger(std::move(log));

#if defined(F4SEAPI)
#	define MODE IS_VR ? "Fallout 4 VR" : "Fallout 4"
#elif defined(SKSEAPI)
#	define MODE IS_VR ? "Skyrim VR" : (IS_GOG ? "Skyrim Special Edition GOG" : "Skyrim Special Edition")
#elif defined(SFSEAPI)
#	define MODE "Starfield"
#else
#	define MODE "DKUtil"
#endif

#if defined(PLUGIN_MODE)
		INFO("Logger init - {} {}", a_name, a_version);
#elif defined(SKSEAPI)
		INFO("Logger init - {} {}", MODE, a_version);
#else
		INFO("Logger init - {} {}", MODE, a_version);
#endif
	}

	inline void SetLevel(const spdlog::level::level_enum a_level) noexcept
	{
		spdlog::default_logger()->set_level(a_level);
	}

	inline void EnableDebug(bool a_enable = true) noexcept
	{
		SetLevel(a_enable ? spdlog::level::level_enum::trace : spdlog::level::level_enum::info);
	}
}  // namespace DKUtil::Logger
