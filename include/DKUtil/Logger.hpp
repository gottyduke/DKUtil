#pragma once

/*
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
#define DKU_L_VERSION_REVISION 2

#ifndef PROJECT_NAME
#	define PROJECT_NAME Plugin::NAME.data()
#endif

#ifndef DKU_DISABLE_LOGGING

#	ifdef DKU_CONSOLE
#		include <spdlog/sinks/stdout_color_sinks.h>
#	else
#		include <spdlog/sinks/basic_file_sink.h>
#	endif

#	include <spdlog/spdlog.h>

#	define __LOG(LEVEL, ...)                                                                       \
		{                                                                                           \
			const auto src = DKUtil::Logger::detail::make_current(std::source_location::current()); \
			spdlog::log(src, spdlog::level::LEVEL, __VA_ARGS__);                                    \
		}

#	define __SHORTF__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#	define __REPORT(DO_EXIT, PROMPT, ...)                                                   \
		{                                                                                    \
			__LOG(critical, __VA_ARGS__);                                                    \
			const auto src = std::source_location::current();                                \
			const auto fmt = fmt::format(DKUtil::Logger::detail::prompt::PROMPT,             \
				src.file_name(), src.line(), src.function_name(), fmt::format(__VA_ARGS__)); \
			DKUtil::Logger::detail::report_error(DO_EXIT, fmt);                              \
		}
#	define INFO(...) __LOG(info, __VA_ARGS__)
#	define DEBUG(...) __LOG(debug, __VA_ARGS__)
#	define TRACE(...) __LOG(trace, __VA_ARGS__)
#	define WARN(...) __LOG(warn, __VA_ARGS__)
#	define ERROR(...) __REPORT(false, error, __VA_ARGS__)
#	define FATAL(...) __REPORT(true, fatal, __VA_ARGS__)

#	define ENABLE_DEBUG DKUtil::Logger::SetLevel(spdlog::level::debug);
#	define DISABLE_DEBUG DKUtil::Logger::SetLevel(spdlog::level::info);

#	ifndef LOG_PATH

#		define PLUGIN_MODE
#		if defined(F4SEAPI)
#			define LOG_PATH "My Games\\Fallout4\\F4SE"sv
#		elif defined(SKSEAPI)
#			define IS_AE REL::Module::IsAE()
#			define IS_SE REL::Module::IsSE()
#			define IS_VR REL::Module::IsVR()
#			define LOG_PATH "My Games\\Skyrim Special Edition\\SKSE"sv
#			define LOG_PATH_VR "My Games\\Skyrim VR\\SKSE"sv
#		else
#			define LOG_PATH ""
#		endif

#	endif

#else

#	define TRACE(...) void(0)
#	define DEBUG(...) void(0)
#	define ERROR(...) void(0)
#	define INFO(...) void(0)

#	define ENABLE_DEBUG void(0)
#	define DISABLE_DEBUG void(0)

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
				"Error occurred at code ->\n[{}:{}]\n\nCallsite ->\n{}\n\nDetail ->\n{}\n\n"
				"Continuing may result in undesired behavior.\n"
				"Exit game? (yes highly suggested)\n\n");

			inline constexpr auto fatal = FMT_STRING(
				"Error occurred at code ->\n[{}:{}]\n\nCallsite ->\n{}\n\nDetail ->\n{}\n\n"
				"Process cannot continue and will now exit.\n");
		}  // namespace prompt

		// From CommonLibSSE https://github.com/Ryan-rsm-McKenzie/CommonLibSSE
		inline std::filesystem::path docs_directory() noexcept
		{
			wchar_t* buffer{ nullptr };
			const auto result = ::SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
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
	}  // namespace detail

	inline void Init(const std::string_view a_name, const std::string_view a_version) noexcept
	{
		std::filesystem::path path;
#ifdef PLUGIN_MODE
		path = std::move(std::filesystem::current_path());
		path /= LOG_PATH;
#else
		path /= IS_VR ? LOG_PATH_VR : LOG_PATH;
#endif
		path /= a_name;
		path += ".log"sv;

#ifdef DKU_CONSOLE
		auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#else
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
#endif

#ifndef NDEBUG
		sink->set_pattern("[%i][%l](%s:%#) %v"s);
#else
		sink->set_pattern("[%D %T][%l](%s:%#) %v"s);
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
		log->set_level(spdlog::level::trace);
#else
		log->set_level(spdlog::level::info);
#endif
		log->flush_on(spdlog::level::trace);

		set_default_logger(std::move(log));

#if defined(F4SEAPI)
#	define MODE "Fallout 4"
#elif defined(SKSEAPI)
#	define MODE "Skyrim Special Edition"
#	define MODE_VR "Skyrim VR"
#else
#	define MODE "DKUtil"
#endif

#ifdef PLUGIN_MODE
		INFO("Logger init - {} {}", a_name, a_version);
#else
		INFO("Logger init - {} {}", IS_VR ? MODE_VR : MODE, a_version);
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
