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

#pragma once

#include <windows.h>
#include "config.hpp"

struct configs {
	global_config &cfg;
	user_config &ucfg;

	configs(global_config &g, user_config &u) : cfg(g),ucfg(u) {}
};

void RegisterMainClass(HINSTANCE hInstance);

HWND CreateMainWindow(HINSTANCE hInstance);
void ShowListManagerDialog(HWND parent, configs &cfgs);
void ShowAddListDialog(HWND parent);
void ShowAddTempDialog(HWND parent);
void ShowAddAppDialog(HWND parent);
bool ShowAddProfileDialog(HWND parent, const global_config &cfg, pg3_profile &p);
void ShowEditProfilesDialog(HWND parent, global_config &cfg);
void ShowAboutDialog(HWND parent);

INT_PTR CALLBACK Lists_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK QuickList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AppList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
