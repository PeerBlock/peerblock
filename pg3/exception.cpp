/*
	Copyright (C) 2007 Cory Nelson

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "stdafx.h"
#include "helpers.hpp"
#include "win32_error.hpp"
#include "exception.hpp"
#include "resource.h"

const char* commdlg_error::what() const {
	try {
		if(!m_what) {
			int len = WideCharToMultiByte(CP_THREAD_ACP, 0, wcwhat(), -1, NULL, 0, "?", NULL);
			if(len) {
				boost::scoped_array<char> buf(new char[++len]);

				len = WideCharToMultiByte(CP_THREAD_ACP, 0, wcwhat(), -1, buf.get(), len, "?", NULL);
				if(len) {
					m_what.swap(buf);
				}
			}
		}
	}
	catch(...) {}

	return m_what ? m_what.get() : "unknown error - unable to get error message";
}

const wchar_t* commdlg_error::wcwhat() const {
	try {
		if(m_wcwhat.empty()) {
			//TODO: make strings for each possible error?
			m_wcwhat = boost::str(boost::wformat(LoadString(IDS_COMMDLG_ERROR)) % boost::io::group(std::setw(8), std::setfill(L'0'), std::hex, std::uppercase, m_err));
		}

		return m_wcwhat.c_str();
	}
	catch(...) {
		return L"unknown error - unable to get error message";
	}
}

static void DoReport(std::exception &ex, const wchar_t *file, unsigned int line) {
	//TODO: start a thread to report the exception.
}

void ReportException(HWND hwnd, std::exception &ex, const wchar_t *file, unsigned int line) {
	std::wstring info;

	if(win32_error *wex = dynamic_cast<win32_error*>(&ex)) {
		const char *func = wex->func();
		if(!func) func = "[none]";

		std::wstring what = wex->wcwhat();
		boost::trim(what);

		info = boost::str(boost::wformat(LoadString(IDS_REPORTINFOWIN)) % file % line % func % what % boost::io::group(std::setw(8), std::setfill(L'0'), std::hex, std::uppercase, wex->error()));
	}
	else if(commdlg_error *cex = dynamic_cast<commdlg_error*>(&ex)) {
		const char *func = cex->func();
		if(!func) func = "[none]";

		info = boost::str(boost::wformat(LoadString(IDS_REPORTINFOCOMMDLG)) % file % line % func % boost::io::group(std::setw(8), std::setfill(L'0'), std::hex, std::uppercase, cex->error()));
	}
	else {
		std::string what = ex.what();
		boost::trim(what);

		info = boost::str(boost::wformat(LoadString(IDS_REPORTINFO)) % typeid(ex).name() % file % line % what.c_str());
	}

	int button;

#if _WIN32_WINNT >= 0x0600
	TASKDIALOG_BUTTON buttons[2] = {
		{ IDYES, MAKEINTRESOURCE(IDS_REPORTERROR) },
		{ IDCANCEL, MAKEINTRESOURCE(IDS_DONTREPORT) }
	};

	TASKDIALOGCONFIG cfg = {0};
	cfg.cbSize = sizeof(cfg);
	cfg.hwndParent = hwnd;
	cfg.hInstance = GetModuleHandle(NULL);
	cfg.dwFlags = TDF_ENABLE_HYPERLINKS;
	cfg.pszWindowTitle = MAKEINTRESOURCE(IDS_UNCAUGHTTITLE);
	cfg.pszMainIcon = TD_ERROR_ICON;
	cfg.pszMainInstruction = MAKEINTRESOURCE(IDS_UNCAUGHTINSTRUCTION);
	cfg.pszContent = MAKEINTRESOURCE(IDS_UNCAUGHTCONTENT);
	cfg.cButtons = 2;
	cfg.pButtons = buttons;
	cfg.nDefaultButton = IDYES;
	cfg.pszExpandedInformation = info.c_str();
	cfg.pszExpandedControlText = MAKEINTRESOURCE(IDS_HIDEREPORTINFO);
	cfg.pszCollapsedControlText = MAKEINTRESOURCE(IDS_SHOWREPORTINFO);
	cfg.pszFooterIcon = TD_INFORMATION_ICON;
	cfg.pszFooter = MAKEINTRESOURCE(IDS_FINDSOLUTIONS);

	TaskDialogIndirect(&cfg, &button, NULL, NULL);
#else
	const std::wstring &title = LoadString(IDS_UNCAUGHTTITLE);
	const std::wstring &content = LoadString(IDS_UNCAUGHTCONTENT) + L"\r\n\r\n" + info;

	button = MessageBox(hwnd, content.c_str(), title.c_str(), MB_ICONERROR | MB_YESNO);
#endif

	if(button == IDYES) {
		DoReport(ex, file, line);
	}
}
