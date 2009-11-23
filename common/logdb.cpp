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
#include "logdb.hpp"
#include "config.hpp"
using namespace sqlite3x;

logdb::logdb() {
}

logdb::~logdb() {
	close();
}

void logdb::open() {
	path p = global_config::log_path();
	m_con.open(p.c_str());

	m_con.executenonquery("PRAGMA page_size = 4096");

	m_con.executenonquery(
		"CREATE TABLE IF NOT EXISTS t_labels("
		"  id INTEGER NOT NULL AUTOINCREMENT PRIMARY KEY,"
		"  label TEXT NOT NULL UNIQUE)");

	m_con.executenonquery(
		"CREATE TABLE IF NOT EXISTS t_log("
		"  id INTEGER NOT NULL AUTOINCREMENT PRIMARY KEY,"
		"  time INTEGER NOT NULL,"
		"  direction INTEGER NOT NULL,"
		"  labelid INTEGER NOT NULL,"
		"  ip NOT NULL,"		// INTEGER for IPv4, 16-byte BLOB for IPv6.
		"  port INTEGER NOT NULL,"
		"  action INTEGER NOT NULL)");

	m_slcmd.prepare(m_con, "SELECT id FROM t_labels WHERE label=?");
	m_ilcmd.prepare(m_con, "INSERT INTO t_labels(label) VALUES(?)");
	m_icmd.prepare(m_con, "INSERT INTO t_log(time,direction,labelid,ip,port) VALUES(?,?,?,?)");
}

void logdb::close() {
	if(m_con.is_open()) {
		m_slcmd.close();
		m_ilcmd.close();
		m_icmd.close();

		m_con.close();
	}
}

__int64 logdb::maxpages() {
	return m_con.executeint64("PRAGMA max_page_count");
}

void logdb::maxpages(__int64 pages) {
	sqlite3_command cmd(m_con, "PRAGMA max_page_count = ?");

	cmd.bind(1, pages);
	cmd.executenonquery();
}

union in6addr_64 {
	in6_addr addr6;
	unsigned __int64 i64[2];
};

sqlite3_result logdb::insert(std::time_t time, direction_type dir, const std::wstring &label, const sockaddr *addr, int addrlen, bool blocked) {
	//TODO: queue these up and bulk insert.

	sqlite3_transaction trans(m_con);

	__int64 labelid;
	sqlite3_result res;

	if(label.empty()) {
		labelid = 0;
	}
	else {
		sqlite3_reader reader(m_slcmd);

		m_slcmd.bind(1, label);
		
		res = m_slcmd.step();

		if(res == sqlite3x::row) {
			labelid = m_slcmd.getint64(0);
			m_slcmd.reset();
		}
		else if(res == sqlite3x::done) {
			m_slcmd.reset();

			m_ilcmd.bind(1, label);

			res = m_ilcmd.executenonquery();
			if(res != sqlite3x::done) {
				// busy or full.
				return res;
			}

			labelid = m_con.insertid();
		}
		else {
			// busy.
			return res;
		}
	}
	
	m_icmd.bind(1, time);
	m_icmd.bind(2, dir == inbound ? 1 : 0);
	m_icmd.bind(3, labelid);

	if(addr->sa_family == AF_INET) {
		const sockaddr_in &addr4 = *(const sockaddr_in*)addr;

		m_icmd.bind(4, (__int64)_byteswap_ulong(addr4.sin_addr.s_addr));
		m_icmd.bind(5, _byteswap_ushort(addr4.sin_port));
	}
	else if(addr->sa_family == AF_INET6) {
		const sockaddr_in6 &addr6 = *(const sockaddr_in6*)addr;
		const in6addr_64 &ip = *(const in6addr_64*)&addr6.sin6_addr;

		in6addr_64 buf;

		buf.i64[0] = _byteswap_uint64(ip.i64[1]);
		buf.i64[1] = _byteswap_uint64(ip.i64[0]);

		m_icmd.bind(4, (const void*)buf.addr6.u.Byte, 16);
		m_icmd.bind(5, _byteswap_ushort(addr6.sin6_port));
	}
	
	res = m_icmd.executenonquery();
	if(res == sqlite3x::done) {
		trans.commit();
	}

	return res;
}
