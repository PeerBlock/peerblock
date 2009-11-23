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
#include "sqlite3x.hpp"

namespace sqlite3x {

sqlite3_transaction::sqlite3_transaction(sqlite3_connection &con, bool begin) : m_con(con),m_intrans(false) {
	if(begin) {
		this->begin();
	}
}

sqlite3_transaction::~sqlite3_transaction() {
	if(m_intrans) {
		rollback();
	}
}

bool sqlite3_transaction::in_transaction() const {
	return m_intrans;
}

bool sqlite3_transaction::begin() {
	if(m_con.executenonquery("BEGIN") == done) {
		m_intrans = true;
		return true;
	}
	else {
		return false;
	}
}

void sqlite3_transaction::commit() {
	m_intrans = false;
	m_con.executenonquery("COMMIT");
}

void sqlite3_transaction::rollback() {
	m_intrans = false;
	m_con.executenonquery("ROLLBACK");
}

}
