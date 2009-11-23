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

sqlite3_command::sqlite3_command() : m_con(0),m_stmt(0) {}

sqlite3_command::sqlite3_command(sqlite3_connection &con, const char *sql) : m_con(0),m_stmt(0) {
	prepare(con, sql);
}

sqlite3_command::sqlite3_command(sqlite3_connection &con, const wchar_t *sql) : m_con(0),m_stmt(0) {
	prepare(con, sql);
}

sqlite3_command::sqlite3_command(sqlite3_connection &con, const std::string &sql) : m_con(0),m_stmt(0) {
	prepare(con, sql);
}

sqlite3_command::sqlite3_command(sqlite3_connection &con, const std::wstring &sql) : m_con(0),m_stmt(0) {
	prepare(con, sql);
}

sqlite3_command::~sqlite3_command() {
	close();
}

void sqlite3_command::prepare(sqlite3_connection &con, const char *sql) {
	const char *tail = NULL;
	if(sqlite3_prepare_v2(con.m_db, sql, -1, &m_stmt, &tail) != SQLITE_OK)
		throw database_error(con);

	m_con = &con;
	m_argc = sqlite3_column_count(m_stmt);
}

void sqlite3_command::prepare(sqlite3_connection &con, const wchar_t *sql) {
	const wchar_t *tail = NULL;
	if(sqlite3_prepare16_v2(con.m_db, sql, -1, &m_stmt, (const void**)&tail) != SQLITE_OK)
		throw database_error(con);

	m_con = &con;
	m_argc = sqlite3_column_count(m_stmt);
}

void sqlite3_command::prepare(sqlite3_connection &con, const std::string &sql) {
	const char *tail = NULL;
	if(sqlite3_prepare_v2(con.m_db, sql.data(), (int)sql.length(), &m_stmt, &tail) != SQLITE_OK)
		throw database_error(con);

	m_con = &con;
	m_argc = sqlite3_column_count(m_stmt);
}

void sqlite3_command::prepare(sqlite3_connection &con, const std::wstring &sql) {
	const wchar_t *tail = NULL;
	if(sqlite3_prepare16_v2(con.m_db, sql.data(), (int)sql.length()*2, &m_stmt, (const void**)&tail) != SQLITE_OK)
		throw database_error(con);

	m_con = &con;
	m_argc = sqlite3_column_count(m_stmt);
}

void sqlite3_command::close() {
	if(m_stmt) {
		sqlite3_finalize(m_stmt);
		m_stmt = 0;
	}
}

void sqlite3_command::bind_null(int index) {
	if(sqlite3_bind_null(m_stmt, index) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, int data) {
	if(sqlite3_bind_int(m_stmt, index, data) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, long long data) {
	if(sqlite3_bind_int64(m_stmt, index, data) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, double data) {
	if(sqlite3_bind_double(m_stmt, index, data) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, const char *data, int datalen) {
	if(sqlite3_bind_text(m_stmt, index, data, datalen, SQLITE_TRANSIENT) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, const wchar_t *data, int datalen) {
	if(sqlite3_bind_text16(m_stmt, index, data, datalen * sizeof(wchar_t), SQLITE_TRANSIENT) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, const void *data, int datalen) {
	if(sqlite3_bind_blob(m_stmt, index, data, datalen, SQLITE_TRANSIENT) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, const std::string &data) {
	if(sqlite3_bind_text(m_stmt, index, data.data(), (int)data.length(), SQLITE_TRANSIENT) != SQLITE_OK)
		throw database_error(*m_con);
}

void sqlite3_command::bind(int index, const std::wstring &data) {
	if(sqlite3_bind_text16(m_stmt, index, data.data(), (int)data.length() * sizeof(wchar_t), SQLITE_TRANSIENT) != SQLITE_OK)
		throw database_error(*m_con);
}

sqlite3_result sqlite3_command::step() {
	int res = sqlite3_step(m_stmt);

	switch(res) {
		case SQLITE_BUSY:
		case SQLITE_FULL:
		case SQLITE_ROW:
		case SQLITE_DONE:
			return (sqlite3_result)res;
		default:
			throw database_error(*m_con);
	}
}

void sqlite3_command::reset() {
	if(sqlite3_reset(m_stmt) != SQLITE_OK)
		throw database_error(*m_con);
}

int sqlite3_command::getint(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return sqlite3_column_int(m_stmt, index);
}

long long sqlite3_command::getint64(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return sqlite3_column_int64(m_stmt, index);
}

double sqlite3_command::getdouble(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return sqlite3_column_double(m_stmt, index);
}

const char* sqlite3_command::gettext(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return (const char*)sqlite3_column_text(m_stmt, index);
}

const wchar_t* sqlite3_command::gettext16(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return (const wchar_t*)sqlite3_column_text16(m_stmt, index);
}

std::size_t sqlite3_command::gettextlen(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return sqlite3_column_bytes(m_stmt, index);
}

std::size_t sqlite3_command::gettextlen16(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return sqlite3_column_bytes16(m_stmt, index) / 2;
}

std::string sqlite3_command::getstring(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return std::string((const char*)sqlite3_column_text(m_stmt, index), sqlite3_column_bytes(m_stmt, index));
}

std::wstring sqlite3_command::getstring16(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return std::wstring((const wchar_t*)sqlite3_column_text16(m_stmt, index), sqlite3_column_bytes16(m_stmt, index)/2);
}

std::string sqlite3_command::getblob(unsigned int index) {
	if(index >= m_argc) throw std::out_of_range("index out of range");
	return std::string((const char*)sqlite3_column_blob(m_stmt, index), sqlite3_column_bytes(m_stmt, index));
}

sqlite3_result sqlite3_command::executenonquery() {
	sqlite3_reader r(*this);
	return step();
}

int sqlite3_command::executeint() {
	sqlite3_reader r(*this);

	if(step() != row) {
		throw database_error("nothing to read");
	}

	return getint(0);
}

long long sqlite3_command::executeint64() {
	sqlite3_reader r(*this);

	if(step() != row) {
		throw database_error("nothing to read");
	}

	return getint64(0);
}

double sqlite3_command::executedouble() {
	sqlite3_reader r(*this);

	if(step() != row) {
		throw database_error("nothing to read");
	}

	return getdouble(0);
}

std::string sqlite3_command::executestring() {
	sqlite3_reader r(*this);

	if(step() != row) {
		throw database_error("nothing to read");
	}

	return getstring(0);
}

std::wstring sqlite3_command::executestring16() {
	sqlite3_reader r(*this);

	if(step() != row) {
		throw database_error("nothing to read");
	}

	return getstring16(0);
}

std::string sqlite3_command::executeblob() {
	sqlite3_reader r(*this);

	if(step() != row) {
		throw database_error("nothing to read");
	}

	return getblob(0);
}

}
