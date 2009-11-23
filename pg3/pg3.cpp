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
#include "procs.hpp"
#include "exception.hpp"
#include "linegraph.hpp"
#include "win32_error.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
	{
		WSADATA data;
		WSAStartup(WINSOCK_VERSION, &data);
	}

	OleInitialize(NULL);

	InitCommonControls();
	RegisterLineGraph();
	RegisterMainClass(hInstance);

	{
		HWND hwnd = CreateMainWindow(hInstance);

		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);
	}

	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
