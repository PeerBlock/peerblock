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
#include <vector>
#include <stdexcept>

#include <boost/crc.hpp>

#include "p2p/list.hpp"
#include "p2p/key.hpp"
#include "path.hpp"
#include "mutex.hpp"
#include "platform.hpp"

// holds proxy information for updating.
struct pg3_proxy {
	std::wstring server;
	long type;

	COMMON_EXPORT pg3_proxy();
};

// holds list information for our config.
struct pg3_list {
	std::wstring path; // can be a URL or file path.  must be unique.
	bool block;
	
	pg3_proxy proxy;
	
	std::time_t lastupdate;
	bool failedupdate;

	bool failedload;

	COMMON_EXPORT pg3_list();

	// returns path unchanged if it is a file, or generates a unique path if it is a URL.
	COMMON_EXPORT ::path real_path() const;
};

// holds list and cache information for profiles.
struct pg3_profile {
	typedef boost::crc_32_type::value_type crc_type;

	std::wstring label; // must be unique.

	std::vector<bool> lists; // tells which lists are enabled in this profile.

	struct tag_presets {
		bool ads, edu, p2p, spy;
		bool gaming;

		bool bt_level1, bt_level2, bt_level3, bt_bogon, bt_dshield, bt_edu, bt_hijacked,
			bt_multicast, bt_private, bt_reserved, bt_allows, bt_lan, bt_spiders, bt_spyware, bt_trojans;
	} presets; //TODO: read/write config.

	crc_type crc; // crc of lists in cache.

	COMMON_EXPORT pg3_profile();

	COMMON_EXPORT bool operator<(const pg3_profile &p) const;

	COMMON_EXPORT path cache_path() const;
};

// holds window position/size information.
struct pg3_position {
	int x, y, cx, cy;

	COMMON_EXPORT pg3_position();
};

// holds the global config that all users share.
struct global_config : public mutex {
	struct tag_block {
		bool enabled, checkhttp;
	} block;

	struct tag_lists {
		std::vector<pg3_list> lists;
		std::vector<pg3_profile> profiles;
		std::vector<pg3_profile>::size_type curprofile;
	} lists;

	struct tag_updates {
		bool updatepg, updatelists;
		unsigned int interval; // update interval, in days.
		std::time_t lastupdate;
		unsigned int timeout; // timeout in seconds.

		pg3_proxy proxy;
	} updates;

	COMMON_EXPORT global_config();
	COMMON_EXPORT ~global_config();
	
	COMMON_EXPORT global_config(const global_config &cfg);
	COMMON_EXPORT global_config& operator=(const global_config &cfg);

	COMMON_EXPORT bool load();
	COMMON_EXPORT void save() const;

	COMMON_EXPORT static path base_path();
	COMMON_EXPORT static path lists_path();
	COMMON_EXPORT static path cache_path();
	COMMON_EXPORT static path config_path();
	COMMON_EXPORT static path log_path();
	COMMON_EXPORT static path appdb_path();
};

// holds the user-specific config: UI info etc.
struct user_config {
	// column sizes for log in main window
	struct tag_main {
		pg3_position position;

		struct tag_columns {
			unsigned int time, address, label, count;
		} columns;
	} main;

	// column sizes for list manager.
	struct tag_lists {
		struct tag_presets {
			unsigned int publisher, label, type, status; //TODO: load/save publisher.
		} presets;

		struct tag_custom {
			unsigned int label, location, type, status;
		} custom;

		struct tag_quicklist {
			unsigned int label, address, type, expire;
		} quicklist;

		struct tag_apps {
			unsigned int label, application, type;
		} apps;
		
		struct tag_addapp {
			pg3_position position;

			struct tag_columns {
				unsigned int name, path;
			} columns;
		} addapp;
	} lists;

	COMMON_EXPORT user_config();
	COMMON_EXPORT ~user_config();

	COMMON_EXPORT bool load();
	COMMON_EXPORT void save() const;

	COMMON_EXPORT static path base_path();
	COMMON_EXPORT static path config_path();
};

/*
	config_error is thrown if the config file has been corrupted.
*/
class config_error : public std::runtime_error {
public:
	config_error(const char *msg, const wchar_t *elem = L"") : std::runtime_error(msg), m_elem(elem) {}
	config_error(const char *msg, const std::wstring &elem) : std::runtime_error(msg), m_elem(elem) {}

	const wchar_t* element() const { return m_elem.c_str(); }

private:
	std::wstring m_elem;
};

// loads a list from memory.  supports .gz, .zip, and .7z compression.
COMMON_EXPORT void load_list(p2p::list &list, const char *mem, std::size_t len, p2p::key *k = 0);

// loads a list from a file.
COMMON_EXPORT void load_list(p2p::list &list, const wchar_t *file, p2p::key *k = 0);

// loads a list from a pg3_list.
COMMON_EXPORT void load_list(p2p::list &list, const pg3_list &l);
