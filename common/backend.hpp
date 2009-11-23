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
#include <memory>
#include <fstream>
#include <boost/function.hpp>
#include <boost/intrusive/list.hpp>

#include "p2p/ip.hpp"
#include "p2p/list.hpp"
#include "mutex.hpp"
#include "thread.hpp"
#include "config.hpp"
#include "cache.hpp"
#include "path.hpp"
#include "pgfilter.hpp"
#include "timer_queue.hpp"
#include "async_state.hpp"
#include "dir_watcher.hpp"
#include "platform.hpp"

/*
	backend is a mostly virtual class for pluggable backends.
	backends handle uploading lists to the driver, updating, and notifying the
	client through callbacks.
*/
class backend {
public:
	// load_state is given through the loading_function callback.
	struct load_state {
		enum stage_type { checking_cache, loading_cache, loading, optimizing, saving_cache, uploading, done };
		stage_type stage;
		unsigned int ipcount;
		float progress;
	};

	// listupdate_state holds the state for a single list.
	struct listupdate_state {
		enum { not_needed, connecting, not_available, downloading, done, error, network_error } stage;
		std::wstring url;
		float progress;
	};

	// updater_state is given through the updating_function callback.
	struct updater_state {
		enum { starting, working, finished, aborted } stage;
		float progress;

		std::vector<listupdate_state> updates;
		bool pgupdate; // true if an update for PG is available.
	};

	typedef pgfilter::action action;
	typedef pgfilter::action_function action_function;
	typedef boost::function<void()> config_function;
	typedef boost::function<void(const load_state&)> loading_function;
	typedef boost::function<void(const updater_state&)> updating_function;

	virtual ~backend();

	// turns blocking on or off.  this is a blocking (but quick) call.
	virtual void setblock(bool block) = 0;
	
	// turns http blocking on or off.  this is a blocking (but quick) call.
	virtual void setblockhttp(bool block) = 0;

	// starts loading blocklists in the background.
	// returns true if loading was started, false if loading was restarted.
	virtual bool loadlists() = 0;

	// adds a temporary ip, with an expire time.
	virtual void settempip(p2p::ip4 ip, time_t expire, bool block) = 0;
	virtual void settempip(const p2p::ip6 &ip, time_t expire, bool block) = 0;

	// removes a temporary ip.  no error if ip doesn't exist.
	virtual void removetempip(p2p::ip4 ip) = 0;
	virtual void removetempip(const p2p::ip6 &ip) = 0;

	// starts updating in the background.
	// if force is true, it will force a the update to restart from scratch.
	// returns true if updating was started, false if an update is already happening.
	virtual bool update(bool force = false) = 0;

	// sets the callback functions.  these can come in from various threads.
	COMMON_EXPORT void setactionfunc(const action_function &func = action_function());
	COMMON_EXPORT void setconfigfunc(const config_function &func = config_function());
	COMMON_EXPORT void setloadingfunc(const loading_function &func = loading_function());
	COMMON_EXPORT void setupdatingfunc(const updating_function &func = updating_function());
	
	global_config m_cfg;

protected:
	COMMON_EXPORT backend();

	action_function m_onaction;
	config_function m_onconfig;
	loading_function m_onloading;
	updating_function m_onupdating;
	mutex m_lock;

private:
	dir_watcher m_cfgwatcher;
	void on_cfgwatcher(const std::vector<dir_watcher::change> &changes);
};

/*
	dummy_backend does everything but block.
	(for debugging)
*/
class dummy_backend : public backend {
public:
	COMMON_EXPORT dummy_backend();
	~dummy_backend();

	virtual void setblock(bool block);
	virtual void setblockhttp(bool block);

	bool loadlists();
	
	void settempip(p2p::ip4 ip, time_t expire, bool block);
	void settempip(const p2p::ip6 &ip, time_t expire, bool block);

	void removetempip(p2p::ip4 ip);
	void removetempip(const p2p::ip6 &ip);

	bool update(bool force = false);

protected:
	// holds a temporary ip.
	struct temp_ip : public boost::intrusive::list_base_hook<> {
		p2p::ip4 ip4; // must be 0 if ip6 is to be used.
		p2p::ip6 ip6;
		bool block;

		HANDLE timer;
		dummy_backend *backend;

		// destroyer for clearing the intrusive list.
		static void destroyer(temp_ip *tip) { delete tip; }
	};
	typedef boost::intrusive::list<temp_ip> temp_list;

	// adds a temporary IP.
	void settempip(std::auto_ptr<temp_ip> &tip, time_t expire, bool block);

	// called when a temporary IP's timer expires.
	void on_tempexpire(temp_ip *tip);
	static void CALLBACK on_tempexpire_thunk(void *arg, BOOLEAN);

	// thread update() starts up, to update PG/lists.
	void update_thread();

	// callback for updater to recieve status updates.
	void on_update(const updater_state &state);

	// thread loadlists() starts up, to load the lists.
	void load_thread();

	// callback for cache::load to recieve status updates.
	bool on_load(const cache::load_state &state);

	virtual void set_ranges(p2p::list &list, bool block);
	
	timer_queue m_timer;
	temp_list m_temp;
	
	async_state m_loading;
	thread m_loadthread;

	async_state m_updating;
	thread m_updatethread;
};

/*
	driver_backend is a backend that communicates directly to the driver.
*/
class driver_backend : public dummy_backend {
public:
	COMMON_EXPORT driver_backend();
	~driver_backend();

	void setblock(bool block);
	void setblockhttp(bool block);

private:
	void set_ranges(p2p::list &list, bool block);

	pgfilter m_filter;
};
