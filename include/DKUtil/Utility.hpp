#pragma once


#include <algorithm>
#include <bit>
#include <concepts>
#include <string>


#ifndef PROJECT_NAME
#define PROJECT_NAME	Version::PROJECT.data()
#endif


#ifdef DKU_U_NDEBUG
#define DEBUG(...)		void(0)
#endif


namespace DKUtil::Utility
{
	namespace function
	{
		template <typename Func, typename... Args>
		inline consteval std::size_t GetFuncArgsCount(Func(*)(Args...))
		{
			return decltype(std::integral_constant<unsigned, sizeof ...(Args)>{})::value;
		}


		template <typename Func, class Class, typename... Args>
		inline consteval std::size_t GetMemFuncArgsCount(Func(Class::*)(Args...))
		{
			return decltype(std::integral_constant<unsigned, sizeof ...(Args)>{})::value;
		}
	} // namespace function


	namespace model
	{
		template <class derived_t>
		class Singleton
		{
		public:
			static derived_t* GetSingleton()
			{
				static derived_t singleton;
				return std::addressof(singleton);
			}

			Singleton(const Singleton&) = delete;
			Singleton(Singleton&&) = delete;
			Singleton& operator=(const Singleton&) = delete;
			Singleton& operator=(Singleton&&) = delete;

		protected:
			Singleton() = default;
			virtual ~Singleton() = default;
		};
	} // namespace model


	namespace numbers
	{
		inline constexpr std::uint32_t FNV_1A_32(const char* const a_str, const std::uint32_t a_value = 2166136261u) noexcept
		{
			return (a_str[0] == '\0')
				? a_value
				: FNV_1A_32(&a_str[1], (a_value ^ static_cast<std::uint32_t>(a_str[0])) * 16777619u);
		}


		inline constexpr std::uint64_t FNV_1A_64(const char* const a_str, const std::uint64_t a_value = 14695981039346656037u) noexcept
		{
			return (a_str[0] == '\0')
				? a_value
				: FNV_1A_64(&a_str[1], (a_value ^ static_cast<std::uint64_t>(a_str[0])) * 1099511628211u);
		}
	} // namespace numbers


	namespace string
	{
		inline std::wstring to_wstring(const std::string& a_str)
		{
			auto resultSize = MultiByteToWideChar(CP_UTF8, 0, &a_str[0], static_cast<int>(a_str.size()), nullptr, 0);
			std::wstring wStr(resultSize, 0);
			MultiByteToWideChar(CP_UTF8, 0, &a_str[0], static_cast<int>(a_str.size()), &wStr[0], resultSize);
			return std::move(wStr);
		}


		// https://stackoverflow.com/questions/28708497/constexpr-to-concatenate-two-or-more-char-strings
		template<size_t S>
		using size = std::integral_constant<size_t, S>;

		template<class T, size_t N>
		consteval size<N> length(T const(&)[N]) { return {}; }
		template<class T, size_t N>
		consteval size<N> length(std::array<T, N> const&) { return {}; }

		template<class T>
		using length_t = decltype(length(std::declval<T>()));
		consteval size_t sum_string_sizes() { return 0; }
		template<class...Ts>
		consteval size_t sum_string_sizes(size_t i, Ts... ts)
		{
			return (i ? i - 1 : 0) + sum_sizes(ts...);
		}

		template<unsigned N1, unsigned N2, class... Us>
		consteval auto concat(const char(&a1)[N1], const char(&a2)[N2], const Us&... xs) 
			->std::array<char, sum_string_sizes(N1, N2, length_t<Us>::value...) + 1 >
		{
			return concat(a1, concat(a2, xs...));
		}
	} // namespace string


	namespace IPC
	{
		using WndProcFunc = std::add_pointer_t<LRESULT((__stdcall)(HWND, UINT, WPARAM, LPARAM))>;

		struct HostInfo
		{
			enum class HostType
			{
				kInvalid,

				kSelf,
				kRelay
			};

			std::wstring    WndClass;
			std::wstring    WndTitle;
			HostType        Type = HostType::kInvalid;
			HWND            Window = nullptr;
			std::string     Name = PROJECT_NAME;
		};
		using HostType = HostInfo::HostType;


		template <const LRESULT QueryFlag, const LRESULT RelayFlag, typename payload_t>
		inline bool TryInitHost(
			HostInfo& a_host,
			const char* a_desc,
			const WNDPROC a_proc,
			const payload_t a_payload,
			const std::predicate auto a_func) noexcept
		{
			if (!a_proc) {
				return false;
			}

			a_host.Window = FindWindow(a_host.WndClass.c_str(), a_host.WndTitle.c_str());

			if (a_host.Window && a_host.Type == HostType::kSelf) {
				return true;
			} else if (!a_host.Window) {
				DEBUG("DKU_U_IPC: {} hosting...", a_desc);

				WNDCLASSEX windowClass{ };
				windowClass.cbSize = sizeof(windowClass);
				windowClass.lpfnWndProc = DefWindowProc;
				windowClass.lpszClassName = a_host.WndClass.c_str();

				if (!RegisterClassEx(&windowClass)) {
					return false;
				}

				a_host.Window = CreateWindow(windowClass.lpszClassName, a_host.WndTitle.c_str(), WS_CAPTION | WS_DISABLED, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
				if (!a_host.Window) {
					UnregisterClass(windowClass.lpszClassName, GetModuleHandle(nullptr));
					return false;
				}

				SetWindowLongPtr(a_host.Window, GWLP_WNDPROC, (LONG_PTR)(a_proc));

				if (!a_func()) {
					return false;
				}

				a_host.Name = PROJECT_NAME;
				a_host.Type = HostType::kSelf;

				DEBUG("DKU_U_IPC: {} host complete", a_desc);

				return TryInitHost<QueryFlag, RelayFlag>(a_host, a_desc, a_proc, a_payload, a_func);
			} else {
				DEBUG("DKU_U_IPC: {} querying...", a_desc);

				const auto result = SendMessageA(a_host.Window, QueryFlag, std::bit_cast<std::uintptr_t>(std::addressof(a_host.Name)), std::bit_cast<std::uintptr_t>(a_payload));

				if (result == RelayFlag) {
					a_host.Type = HostType::kRelay;
					DEBUG("DKU_U_IPC: {} relay available -> {}", a_desc, a_host.Name);
					return true;
				} else {
					DEBUG("DKU_U_IPC: {} relay unavailable, fallback", a_desc);

					a_host.WndClass = string::to_wstring(PROJECT_NAME);
					a_host.WndTitle = string::to_wstring(PROJECT_NAME);

					return TryInitHost<QueryFlag, RelayFlag>(a_host, a_desc, a_proc, a_payload, a_func);
				}
			}
		}


		inline bool TerminateHost(HostInfo& a_host) noexcept
		{
			if (!a_host.Window) {
				return false;
			}

			UnregisterClass(a_host.WndClass.c_str(), GetModuleHandleA(nullptr));
			DestroyWindow(a_host.Window);

			DEBUG("DKU_U_IPC: {} host terminated", a_host.Name);

			return false;
		}


		inline bool TransferHost() noexcept
		{
			return false;
		}
	} // namespace IPC
}


namespace DKUtil::Alias
{
	using HostInfo = DKUtil::Utility::IPC::HostInfo;
	using HostType = DKUtil::Utility::IPC::HostType;
	using WndProcFunc = DKUtil::Utility::IPC::WndProcFunc;
}