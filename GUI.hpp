/* DISCLAIMER START */
/**
 * Copyright 2020 DK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
/* DISCLAIMER END */


#ifndef DKUTIL_GUI
#define DKUTIL_GUI


/* VERSION DEFINE START */
#define DKUTIL_GUI_VERSION_MAJOR	1
#define DKUTIL_GUI_VERSION_MINOR	0
#define DKUTIL_GUI_VERSION_PATCH	0
#define DKUTIL_GUI_VERSION_BEAT		0
/* VERSION DEFINE END */


/* PATCH NOTE START */
/**
 * 1.0.0
 * Added simple windows native message box;
 * May abort SkyrimSE process;
 * 
/* PATCH NOTE END */


/* INCLUDE START */
#include <cstdio>

#include <WinUser.h>

/* INCLUDE END */


/* GENERAL DEFINE START */
#define _CRT_SECURE_NO_WARNINGS
/* GENERAL DEFINE END */


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
