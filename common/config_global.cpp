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
#include "path.hpp"
#include "win32_error.hpp"
#include "xmllite.hpp"
#include "config.hpp"

pg3_proxy::pg3_proxy() {
	type = 0;
}

pg3_list::pg3_list() {
	block = true;
	lastupdate = 0;
	failedupdate = false;
	failedload = false;
}

pg3_profile::pg3_profile() : presets() {
	crc = 0;
}

bool pg3_profile::operator<(const pg3_profile &p) const {
#if _WIN32_WINNT < 0x0600
#undef LINGUISTIC_IGNORECASE
#define LINGUISTIC_IGNORECASE NORM_IGNORECASE
#endif

	int ret = CompareString(LOCALE_USER_DEFAULT, LINGUISTIC_IGNORECASE | NORM_IGNOREKANATYPE, label.c_str(), (int)label.length(), p.label.c_str(), (int)p.label.length());
	if(!ret) throw win32_error("CompareString");

	return (ret == CSTR_LESS_THAN);
}

global_config::global_config() {
	block.enabled = true;
	block.checkhttp = true;
	
	updates.updatepg = true;
	updates.updatelists = true;
	updates.interval = 2;
	updates.lastupdate = 0;
	updates.timeout = 10;
}

global_config::global_config(const global_config &cfg)
: block(cfg.block),lists(cfg.lists),updates(cfg.updates)
{
}

global_config& global_config::operator=(const global_config &cfg) {
	block = cfg.block;
	lists = cfg.lists;
	updates = cfg.updates;
	return *this;
}

global_config::~global_config() {
}

class global_config_reader {
public:
	global_config_reader(global_config &cfg, const wchar_t *file) : m_cfg(cfg),m_reader(file) {}

	void read() {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"PeerGuardian")) {
					if(!m_reader.move_to_attribute(L"Version")) {
						throw config_error("version attribute not present");
					}

					if(wcscmp(m_reader.value(), L"3.0")) {
						throw config_error("unknown version number");
					}

					m_reader.move_to_element();

					read_peerguardian();
				}
				else {
					throw config_error("unknown element", name);
					//m_reader.skip_element();
				}
			}
		}
	}

private:
	void read_peerguardian() {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"Blocking")) {
					read_blocking();
				}
				else if(!wcscmp(name, L"Lists")) {
					read_lists();
				}
				else if(!wcscmp(name, L"Profiles")) {
					read_profiles();
				}
				else if(!wcscmp(name, L"Updates")) {
					read_updates();
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				break;
			}
		}
	}

	void read_blocking() {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"Enabled")) {
					m_cfg.block.enabled = read_bool();
				}
				else if(!wcscmp(name, L"CheckHttp")) {
					m_cfg.block.checkhttp = read_bool();
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Blocking\\") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				break;
			}
		}
	}

	void read_lists() {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"List")) {
					read_list();
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Lists\\") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				break;
			}
		}
	}

	void read_list() {
		pg3_list list;

		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"Path")) {
					list.path = name;
				}
				else if(!wcscmp(name, L"Type")) {
					std::wstring val = read_string();

					if(val == L"Block") {
						list.block = true;
					}
					else if(val == L"Allow") {
						list.block = false;
					}
					else {
						throw config_error("unknown value", L"PeerGuardian\\Lists\\List\\Type");
					}
				}
				else if(!wcscmp(name, L"Proxy")) {
					read_proxy(list.proxy);
				}
				else if(!wcscmp(name, L"LastUpdate")) {
					list.lastupdate = read_int64();
				}
				else if(!wcscmp(name, L"FailedUpdate")) {
					list.failedupdate = read_bool();
				}
				else if(!wcscmp(name, L"FailedLoad")) {
					list.failedupdate = read_bool();
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Lists\\List\\") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				if(list.path.size()) {
					if(m_lists.insert(lists_type::value_type(list.path, m_cfg.lists.lists.size())).second) {
						m_cfg.lists.lists.push_back(list);
					}
					else {
						throw std::runtime_error("duplicate list");
					}
				}
				else {
					throw std::runtime_error("list is missing required values");
				}
				break;
			}
		}
	}

	void read_profiles() {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"Profile")) {
					read_profile();
				}
				else if(!wcscmp(name, L"CurrentProfile")) {
					profiles_type::iterator iter = m_profiles.find(read_string());
					if(iter != m_profiles.end()) {
						m_cfg.lists.curprofile = iter->second;
					}
					else {
						throw std::runtime_error("bad CurrentProfile value");
					}
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Profiles\\") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				break;
			}
		}
	}

	void read_profile() {
		pg3_profile p;

		p.lists.resize(m_cfg.lists.lists.size());

		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"Label")) {
					p.label = read_string();
				}
				else if(!wcscmp(name, L"CacheHash")) {
					p.crc = read_uint32(16);
				}
				else if(!wcscmp(name, L"Lists")) {
					read_profile_lists(p);
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Profiles\\Profile") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				if(p.label.size()) {
					if(m_profiles.insert(profiles_type::value_type(p.label, m_cfg.lists.profiles.size())).second) {
						m_cfg.lists.profiles.push_back(p);
					}
					else {
						throw std::runtime_error("duplicate profile");
					}
				}
				else {
					throw std::runtime_error("profile is missing required values");
				}
				break;
			}
		}
	}

	void read_profile_lists(pg3_profile &p) {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"List")) {
					lists_type::iterator iter = m_lists.find(read_string());
					if(iter != m_lists.end()) {
						p.lists[iter->second] = true;
					}
					else {
						throw std::runtime_error("bad List value in profile");
					}
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Profiles\\Profile") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				break;
			}
		}
	}

	void read_updates() {
		xml_reader::node_type type;
		while((type = m_reader.read()) != xml_reader::none) {
			if(type == xml_reader::element) {
				const wchar_t *name = m_reader.local_name();
				if(!wcscmp(name, L"UpdatePeerGuardian")) {
					m_cfg.updates.updatepg = read_bool();
				}
				else if(!wcscmp(name, L"UpdateLists")) {
					m_cfg.updates.updatelists = read_bool();
				}
				else if(!wcscmp(name, L"Interval")) {
					m_cfg.updates.interval = read_uint32();
				}
				else if(!wcscmp(name, L"LastUpdate")) {
					m_cfg.updates.lastupdate = read_int64();
				}
				else if(!wcscmp(name, L"Timeout")) {
					m_cfg.updates.timeout = read_uint32();
				}
				else if(!wcscmp(name, L"Proxy")) {
					read_proxy(m_cfg.updates.proxy);
				}
				else {
					throw config_error("unknown element", std::wstring(L"PeerGuardian\\Updates\\") + name);
					//m_reader.skip_element();
				}
			}
			else if(type == xml_reader::endelement) {
				break;
			}
		}
	}

	void read_proxy(pg3_proxy &p) {
		while(m_reader.move_to_next_attribute()) {
			if(!wcscmp(m_reader.local_name(), L"Type")) {
				const wchar_t *val = m_reader.value();

				if(!wcscmp(val, L"http")) {
					p.type = CURLPROXY_HTTP;
				}
				else if(!wcscmp(val, L"socks4")) {
					p.type = CURLPROXY_SOCKS4;
				}
				else if(!wcscmp(val, L"socks5")) {
					p.type = CURLPROXY_SOCKS5;
				}
				else {
					p.type = 0;
				}
			}
		}

		m_reader.move_to_element();

		p.server = read_string();
	}

	bool read_bool() {
		std::wstring s = read_string();

		if(s == L"Yes") return true;
		if(s == L"No") return false;

		throw config_error("invalid boolean value");
	}

	unsigned int read_uint32(int base = 10) {
		unsigned long ret;

		std::wstring s = read_string();

		const wchar_t *str = s.c_str();
		wchar_t *endptr;

		ret = wcstoul(str, &endptr, base);

		if(endptr != (str + s.length())) {
			throw config_error("invalid unsigned value");
		}

		return ret;
	}

	__int64 read_int64() {
		__int64 ret;

		std::wstring s = read_string();

		const wchar_t *str = s.c_str();
		wchar_t *endptr;

		ret = _wcstoui64(str, &endptr, 10);

		if(endptr != (str + s.length())) {
			throw config_error("invalid integer value");
		}

		return ret;
	}

	std::wstring read_string() {
		std::wstring ret;

		if(!m_reader.empty_element()) {
			xml_reader::node_type type;
			while((type = m_reader.read()) != xml_reader::none) {
				if(type == xml_reader::text) {
					UINT len;
					const wchar_t *str = m_reader.value(&len);

					ret.append(str, len);
				}
				else if(type == xml_reader::endelement) {
					break;
				}
			}
		}

		return ret;
	}

	typedef stdext::hash_map<std::wstring, std::vector<pg3_list>::size_type> lists_type;
	typedef stdext::hash_map<std::wstring, std::vector<pg3_profile>::size_type> profiles_type;

	xml_reader m_reader;
	global_config &m_cfg;
	lists_type m_lists;
	profiles_type m_profiles;
};

bool global_config::load() {
	path p = config_path();

	if(path::exists(p)) {
		global_config_reader(*this, p.c_str()).read();
		return true;
	}
	else {
		return false;
	}
}

void global_config::save() const {
	path p = path::temp_file(L"pg3");
	xml_writer writer(p.c_str());

	writer.indent();
	writer.start_document();

	writer.start_element(L"PeerGuardian");
	writer.attribute(L"Version", L"3.0");
	
	writer.start_element(L"Blocking");
	{
		writer.comment(L"Controls if PeerGuardian is enabled.");
		writer.element_string(L"Enabled", block.enabled ? L"Yes" : L"No");

		writer.comment(L"Controls if PeerGuardian checks HTTP connections.");
		writer.element_string(L"CheckHttp", block.checkhttp ? L"Yes" : L"No");
	}
	writer.end_element();

	writer.start_element(L"Lists");
	{
		writer.comment(L"Path: Either a file path or HTTP/FTP URL.");
		writer.comment(L"Type: The type of the list: Block or Allow.");
		writer.comment(L"Proxy: Proxy to connect through.  Type is none/http/socks4/socks5.  If none, connects through the default proxy.");
		writer.comment(L"LastUpdate: Unix timestamp of when the list was last successfully updated.");
		writer.comment(L"FailedUpdate: The last update failed.");
		writer.comment(L"FailedLoad: The list failed to load for cache generation.");

		for(std::vector<pg3_list>::const_iterator iter = lists.lists.begin(), end = lists.lists.end(); iter != end; ++iter) {
			const wchar_t *proxytype;
			wchar_t buf[65];

			switch(iter->proxy.type) {
				case CURLPROXY_HTTP: proxytype = L"http"; break;
				case CURLPROXY_SOCKS4: proxytype = L"socks4"; break;
				case CURLPROXY_SOCKS5: proxytype = L"socks5"; break;
				default: proxytype = L"none"; break;
			}

			writer.start_element(L"List");

			writer.element_string(L"Path", iter->path.c_str());
			writer.element_string(L"Type", iter->block ? L"Block" : L"Allow");
			
			writer.start_element(L"Proxy");
			writer.attribute(L"Type", proxytype);
			if(iter->proxy.server.size()) {
				writer.string(iter->proxy.server.c_str());
			}
			writer.end_element();

			writer.element_string(L"LastUpdate", _i64tow(iter->lastupdate, buf, 10));
			writer.element_string(L"FailedUpdate", iter->failedupdate ? L"Yes" : L"No");
			writer.element_string(L"FailedLoad", iter->failedload ? L"Yes" : L"No");

			writer.end_element();
		}
	}
	writer.end_element();

	writer.start_element(L"Profiles");
	{
		for(std::vector<pg3_profile>::const_iterator iter = lists.profiles.begin(), end = lists.profiles.end(); iter != end; ++iter) {
			wchar_t buf[33];

			writer.start_element(L"Profile");

			writer.element_string(L"Label", iter->label.c_str());
			writer.element_string(L"CacheHash", _ultow(iter->crc, buf, 16));

			writer.start_element(L"Lists");
			{
				if(iter->lists.size() != lists.lists.size()) {
					throw config_error("profile lists do not match lists");
				}

				for(std::vector<bool>::size_type i = 0; i < iter->lists.size(); ++i) {
					if(iter->lists[i]) {
						writer.element_string(L"List", lists.lists[i].path.c_str());
					}
				}
			}
			writer.end_element();

			writer.end_element();
		}

		if(lists.profiles.size()) {
			writer.element_string(L"CurrentProfile", lists.profiles[lists.curprofile].label.c_str());
		}
	}
	writer.end_element();

	writer.start_element(L"Updates");
	{
		const wchar_t *proxytype;
		wchar_t buf[65];

		switch(updates.proxy.type) {
			case CURLPROXY_HTTP: proxytype = L"http"; break;
			case CURLPROXY_SOCKS4: proxytype = L"socks4"; break;
			case CURLPROXY_SOCKS5: proxytype = L"socks5"; break;
			default: proxytype = L"none"; break;
		}

		writer.comment(L"Controls if PeerGuardian will check if an update is available for itself while updating.");
		writer.element_string(L"UpdatePeerGuardian", updates.updatepg ? L"Yes" : L"No");

		writer.comment(L"Controls if PeerGuardian will check for list updates while updating.");
		writer.element_string(L"UpdateLists", updates.updatelists ? L"Yes" : L"No");

		writer.comment(L"Interval in days at which PeerGuardian will auto-update.  0 to disable.");
		writer.element_string(L"Interval", _ultow(updates.interval, buf, 10));

		writer.comment(L"Unix timestamp of when the last update was run.");
		writer.element_string(L"LastUpdate", _i64tow(updates.lastupdate, buf, 10));

		writer.comment(L"Timeout, in seconds, before aborting an update.");
		writer.element_string(L"Timeout", _ultow(updates.timeout, buf, 10));

		writer.comment(L"Default proxy to connect through.  Type is none/http/socks4/socks5.");
		writer.start_element(L"Proxy");
		writer.attribute(L"Type", proxytype);
		if(updates.proxy.server.size()) {
			writer.string(updates.proxy.server.c_str());
		}
		writer.end_element();
	}
	writer.end_element();

	writer.end_element();
	writer.end_document();

	writer.close();
	path::move(p, config_path(), true);
}

path global_config::base_path() {
	return path::base_dir();
	//return path::common_appdata_dir() / L"PeerGuardian 3";
}

path global_config::lists_path() {
	return base_path() / L"Lists";
}

path global_config::cache_path() {
	return base_path() / L"Cache";
}

path global_config::config_path() {
	return base_path() / L"pg3.conf";
}

path global_config::log_path() {
	return base_path() / L"log.db";
}

path global_config::appdb_path() {
	return base_path() / L"apps.db";
}

static path make_path(const std::wstring &str) {
	// generate filename from string.
	std::vector<wchar_t> p;

	// prepend CRC of string so it is kept unique if the replacement algo below causes collisions.
	{
		boost::crc_32_type crc;
		crc.process_bytes(str.c_str(), str.length() * sizeof(std::wstring::value_type));
		
		wchar_t buf[33];
		_ultow(crc.checksum(), buf, 10);

		p.assign(buf, wcschr(buf, L'\0'));
		p.push_back('-');
	}

	for(std::wstring::const_iterator iter = str.begin(), end = str.end(); iter != end; ++iter) {
		switch(*iter) {
			case '<': case L'>': case L':': case L'"': case L'/': case L'\\': case L'|': case L'.':
				p.push_back(L'_');
				break;
			default:
				p.push_back(*iter);
				break;
		}
	}

	p.push_back(L'\0');

	return &p.front();
}

path pg3_list::real_path() const {
	::path ret;

	if(::path::is_url(this->path.c_str())) {
		ret = make_path(this->path.c_str());
	}
	else {
		// normal file.
		ret = this->path;
	}
	
	if(!ret.has_root()) {
		ret = global_config::lists_path() / ret;
	}

	return ret;
}

path pg3_profile::cache_path() const {
	path ret = global_config::cache_path() / make_path(this->label);
	return ret;
}
