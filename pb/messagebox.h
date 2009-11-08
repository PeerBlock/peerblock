/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC

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

#pragma once

#include <exception>
#include <windows.h>
#include "tstring.h"

int MessageBox(HWND hwnd, const tstring &text, const tstring &caption, UINT type);
int MessageBox(HWND hwnd, const tstring &text, UINT captionid, UINT type);
int MessageBox(HWND hwnd, UINT textid, UINT captionid, UINT type);

void ExceptionBox(HWND hwnd, const std::exception &ex, const char *file, int line);
void PeerBlockExceptionBox(HWND hwnd, const peerblock_error &ex);
void UncaughtExceptionBox(HWND hwnd, const char *file, int line);
void UncaughtExceptionBox(HWND hwnd, const std::exception &ex, const char *file, int line);
