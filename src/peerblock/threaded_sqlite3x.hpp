/*
	Copyright (C) 2004-2005 Cory Nelson

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

#include <boost/utility.hpp>
#include <sqlite3x.hpp>
#include "threadx.hpp"

class threaded_sqlite3_connection : public sqlite3x::sqlite3_connection, public
#ifdef _WIN32_WINNT
	mutex
#else
	spinlock
#endif
{};

class sqlite3_lock_base : boost::noncopyable {
private:
	threaded_sqlite3_connection &con;
	bool autotrans, intrans, locked;

protected:
	sqlite3_lock_base(threaded_sqlite3_connection &con, bool autotrans) : con(con),autotrans(autotrans),intrans(false),locked(false) {}

public:
	void begin() {
		con.executenonquery("begin;");
		intrans=true;
	}

	void commit() {
		con.executenonquery("commit;");
		intrans=false;
	}

	void rollback() {
		con.executenonquery("rollback;");
		intrans=false;
	}

	void enter() {
		con.enter();
		locked=true;

		if(autotrans) begin();
	}

	bool tryenter() {
		if((locked=con.tryenter())) {
			if(autotrans) begin();
			return true;
		}
		else return false;
	}

	void leave() {
		if(intrans) rollback();
		con.leave();
	}

	bool is_locked() const { return locked; }

	~sqlite3_lock_base() {
		if(locked) {
			try {
				leave();
			}
			catch(...) {
				return;
			}
		}
	}
};

class sqlite3_lock : public sqlite3_lock_base {
public:
	sqlite3_lock(threaded_sqlite3_connection &con, bool autotrans=false, bool locked=true) : sqlite3_lock_base(con,autotrans) {
		if(locked) enter();
	}
};

class sqlite3_try_lock : public sqlite3_lock_base {
public:
	sqlite3_try_lock(threaded_sqlite3_connection &con, bool autotrans=false, bool locked=true) : sqlite3_lock_base(con,autotrans) {
		if(locked) tryenter();
	}
};
