#pragma once


#include "DKUtil/Impl/PCH.hpp"
#include "DKUtil/Logger.hpp"
#include "DKUtil/Utility.hpp"


namespace DKUtil::Config
{
	inline auto GetPath(const std::string_view a_file) noexcept
	{
		std::filesystem::path dir{ CONFIG_ENTRY };
		std::filesystem::path file{ a_file.data() };

		if (!dir.empty()) {
			file = dir / file;
		}

		return std::move(file.string());
	}


	template <bool RECURSIVE = false>
	inline std::vector<std::string> GetAllFiles(std::string_view a_path = {}, std::string_view a_ext = {}, std::string_view a_prefix = {}, std::string_view a_suffix = {}) noexcept
	{
		using dir_iterator = std::conditional_t<RECURSIVE, std::filesystem::recursive_directory_iterator, std::filesystem::directory_iterator>;

		std::vector<std::string> files;
		auto file_iterator = [&](const std::filesystem::directory_entry& a_file) {
			if (a_file.exists() &&
				!a_file.path().empty()) {
				if (!a_ext.empty() && a_file.path().extension() != a_ext) {
					return;
				}

				const auto path = a_file.path().string();

				if (!a_prefix.empty() && path.find(a_prefix) != std::string::npos) {
					files.push_back(path);
				} else if (!a_suffix.empty() && path.rfind(a_suffix) != std::string::npos) {
					files.push_back(path);
				} else if (a_prefix.empty() && a_suffix.empty()) {
					files.push_back(path);
				}
			}
		};

		std::string dir(MAX_PATH + 1, ' ');
		auto res = ::GetModuleFileNameA(nullptr, dir.data(), MAX_PATH + 1);
		if (res == 0) {
			ERROR("DKU_C: Unable to acquire valid path using default null path argument!\nExpected: Current directory\nResolved: NULL");
		}

		auto eol = dir.find_last_of("\\/");
		dir = dir.substr(0, eol);

		auto path = a_path.empty() ? std::filesystem::path{ dir } : std::filesystem::path{ a_path };
		if (!is_directory(path.parent_path())) {
			path = dir / path;
		}

		std::ranges::for_each(dir_iterator(path), file_iterator);
		std::ranges::sort(files);

		return files;
	}


	namespace detail
	{
		class IData;
		using manager = std::unordered_map<std::string_view, detail::IData*>;

		inline static std::uint32_t _Count{ 0 };

		class IParser
		{
		public:
			explicit IParser(std::string_view a_file, const std::uint32_t a_id, manager& a_manager) :
				_fileName(a_file), _filePath(GetPath(a_file)), _id(a_id), _manager(a_manager)
			{}

			constexpr IParser() = delete;
			constexpr IParser(const IParser&) noexcept = default;
			constexpr IParser(IParser&&) noexcept = default;
			constexpr virtual ~IParser() = default;

			// accessor
			[[nodiscard]] constexpr auto* data() noexcept { return _content.data(); }
			[[nodiscard]] constexpr auto& content() const noexcept { return _content; }
			[[nodiscard]] constexpr std::string_view filename() const noexcept { return _fileName; }
			[[nodiscard]] constexpr std::string_view filepath() const noexcept { return _filePath; }


			virtual void Parse(const char* = nullptr) noexcept = 0;
			virtual void Write(const std::string_view) noexcept = 0;

		protected:
			const std::uint32_t _id;
			const std::string _filePath;
			const std::string _fileName;
			std::string _content;
			const manager& _manager;
		};
	}
}
