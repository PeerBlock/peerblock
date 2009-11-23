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

#include <ctime>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "sqlite3x/sqlite3x.hpp"
#include "platform.hpp"

/*
	logdb handles logging actions to a SQLite 3 database.
*/
class logdb {
public:
	enum direction_type { inbound, outbound };

	COMMON_EXPORT logdb();
	COMMON_EXPORT ~logdb();

	COMMON_EXPORT void open();
	COMMON_EXPORT void close();

	// controls the maximum size of the database.  one page is 4KiB.
	COMMON_EXPORT __int64 maxpages();
	COMMON_EXPORT void maxpages(__int64 pages);

	// inserts an action (block/allow) into the database.
	COMMON_EXPORT sqlite3x::sqlite3_result insert(std::time_t time, direction_type dir, const std::wstring &label, const sockaddr *addr, int addrlen, bool blocked);

private:
	sqlite3x::sqlite3_connection m_con;
	sqlite3x::sqlite3_command m_slcmd, m_ilcmd, m_icmd;
};
