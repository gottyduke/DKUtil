#ifndef DKUTIL_GUI
#define DKUTIL_GUI

#include <cstdio>

#include <WinUser.h>

#define _CRT_SECURE_NO_WARNINGS


namespace DKUtil::GUI::WinAPI
{
	namespace Impl
	{
		constexpr auto CharBufferSize = 1 << 10;
	}


	/// may abort skyrim main process, use as the last resort
	template <typename... Args>
	void ShowMessageBox(const char* a_title, const char* a_text, Args... a_format)
	{
		char buf[Impl::CharBufferSize];
		std::sprintf(buf, a_text, a_format...);

		MessageBox(nullptr, buf, a_title, MB_OK | MB_SETFOREGROUND);
	}
}


namespace DKUtil::GUI::Wrapper
{
	
}


#endif
