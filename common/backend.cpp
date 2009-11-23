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
#include "cache.hpp"
#include "localips.hpp"
#include "backend.hpp"
#include "updater.hpp"

backend::backend() {
	m_cfg.load();

	//path p = global_config::base_path();
	//m_cfgwatcher.open(p.c_str(), false, FILE_NOTIFY_CHANGE_LAST_WRITE, boost::bind(&backend::on_cfgwatcher, this, _1));
}

backend::~backend() {}

void backend::setactionfunc(const action_function &func) {
	mutex::scoped_lock lock(m_lock);
	m_onaction = func;
}

void backend::setconfigfunc(const config_function &func) {
	mutex::scoped_lock lock(m_lock);
	m_onconfig = func;
}

void backend::setloadingfunc(const loading_function &func) {
	mutex::scoped_lock lock(m_lock);
	m_onloading = func;
}

void backend::setupdatingfunc(const updating_function &func) {
	mutex::scoped_lock lock(m_lock);
	m_onupdating = func;
}

void backend::on_cfgwatcher(const std::vector<dir_watcher::change> &changes) {
	{
		global_config::scoped_lock lock(m_cfg);
		m_cfg.load();
	}

	{
		mutex::scoped_lock lock(m_lock);
		if(m_onconfig) m_onconfig();
	}
}

dummy_backend::dummy_backend() {
	//TODO: start a timer for the updater.
	//loadlists();
}

dummy_backend::~dummy_backend() {
	m_timer.close();

	if(m_updatethread) {
		m_updating.stop();
		m_updatethread.join();
	}

	if(m_loadthread) {
		m_loading.stop();
		m_loadthread.join();
	}

	m_temp.clear_and_dispose(temp_ip::destroyer);
}


void dummy_backend::setblock(bool block) {
	{
		global_config::scoped_lock lock(m_cfg);
		m_cfg.block.enabled = block;
	}
	
	mutex::scoped_lock lock(m_lock);
	if(m_onconfig) m_onconfig();
}

void dummy_backend::setblockhttp(bool block) {
	{
		global_config::scoped_lock lock(m_cfg);
		m_cfg.block.checkhttp = block;
	}

	mutex::scoped_lock lock(m_lock);
	if(m_onconfig) m_onconfig();
}

void dummy_backend::settempip(p2p::ip4 ip, time_t expire, bool block) {
	std::auto_ptr<temp_ip> tip(new temp_ip);

	tip->ip4 = ip;

	settempip(tip, expire, block);
}

void dummy_backend::settempip(const p2p::ip6 &ip, time_t expire, bool block) {
	std::auto_ptr<temp_ip> tip(new temp_ip);

	tip->ip4 = 0;
	tip->ip6 = ip;

	settempip(tip, expire, block);
}

void dummy_backend::settempip(std::auto_ptr<temp_ip> &tip, time_t expire, bool block) {
	tip->backend = this;
	tip->block = block;

	// expire at least 1 second from now.
	DWORD expirems = (DWORD)std::max((int)(expire - time(NULL)) * 1000, 1000);
	
	{
		mutex::scoped_lock lock(m_lock);

		tip->timer = m_timer.create(on_tempexpire_thunk, (void*)tip.get(), expirems, 0, WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);

		m_temp.push_back(*tip.get());
		tip.release();
	}

	loadlists();
}

void dummy_backend::removetempip(p2p::ip4 ip) {
	bool reload = false;
	
	{
		mutex::scoped_lock lock(m_lock);

		for(temp_list::iterator iter = m_temp.begin(), end = m_temp.end(); iter != end; ++iter) {
			temp_ip *tip = &*iter;

			if(tip->ip4 == ip) {
				reload = m_timer.destroy(tip->timer);
				m_temp.erase(iter);

				tip->timer = NULL;

				if(reload) {
					// we removed it, we get to delete it.
					delete tip;
				}
				break;
			}
		}
	}

	if(reload) {
		loadlists();
	}
}

void dummy_backend::removetempip(const p2p::ip6 &ip) {
	bool reload = false;
	
	{
		mutex::scoped_lock lock(m_lock);

		for(temp_list::iterator iter = m_temp.begin(), end = m_temp.end(); iter != end; ++iter) {
			temp_ip *tip = &*iter;

			if(!tip->ip4 && tip->ip6 == ip) {
				reload = m_timer.destroy(tip->timer);
				m_temp.erase(iter);

				tip->timer = NULL;

				if(reload) {
					// we removed it, we get to delete it.
					delete tip;
				}
				break;
			}
		}
	}

	if(reload) {
		loadlists();
	}
}

void dummy_backend::on_tempexpire(temp_ip *tip) {
	bool reload = false;

	{
		mutex::scoped_lock lock(m_lock);

		if(tip->timer) { // if timer is NULL, it has been deleted elsewhere.
			m_timer.destroy(tip->timer);
			m_temp.erase(m_temp.iterator_to(*tip));
			delete tip;

			reload = true;
		}
	}

	if(reload) {
		loadlists();
	}
}

void CALLBACK dummy_backend::on_tempexpire_thunk(void *arg, BOOLEAN) {
	temp_ip *tip = (temp_ip*)arg;
	tip->backend->on_tempexpire(tip);
}

bool dummy_backend::update(bool force) {
	if(m_updating.start(force)) {
		m_updatethread.destroy();
		m_updatethread.create(boost::bind(&dummy_backend::update_thread, this));
		return true;
	}
	
	return false;
}

void dummy_backend::update_thread() {
	do {
		updater u(m_cfg, boost::bind(&dummy_backend::on_update, this, _1), m_updating);
		u.start();
	} while(m_updating.reset());
}

void dummy_backend::on_update(const updater_state &state) {
	{
		mutex::scoped_lock lock(m_lock);
		if(m_onupdating) m_onupdating(state);
	}
}

bool dummy_backend::loadlists() {
	if(m_loading.start(true)) {
		m_loadthread.destroy();
		m_loadthread.create(boost::bind(&driver_backend::load_thread, this));

		return true;
	}

	return false;
}

void dummy_backend::load_thread() {
	do {
		p2p::list block;

		cache::load(block, m_cfg, cache::loading_function(boost::bind(&driver_backend::on_load, this, _1)));

		if(!m_loading.needs_cancel()) {
			p2p::list allow;

			{
				mutex::scoped_lock lock(m_lock);

				for(temp_list::iterator iter = m_temp.begin(), end = m_temp.end(); iter != end; ++iter) {
					p2p::list &list = iter->block ? block : allow;

					if(iter->ip4) list.insert(p2p::range4(L"temporary range", iter->ip4));
					else list.insert(p2p::range6(L"temporary range", iter->ip6));
				}
			}

			localip::getips(allow, localip::all);

			block.optimize();
			block.erase(allow);

			if(!m_loading.needs_cancel()) {
				load_state s;
				s.stage = load_state::uploading;
				s.progress = 0.0f;

				{
					mutex::scoped_lock lock(m_lock);
					if(m_onloading) m_onloading(s);
				}

				set_ranges(block, true);

				s.progress = 1.0f;

				{
					mutex::scoped_lock lock(m_lock);
					if(m_onloading) {
						m_onloading(s);

						s.stage = load_state::done;
						m_onloading(s);
					}
				}
			}
		}
	} while(m_loading.reset());
}

bool dummy_backend::on_load(const cache::load_state &state) {
	load_state s;
	s.stage = (load_state::stage_type)state.stage;
	s.progress = state.progress;

	{
		mutex::scoped_lock lock(m_lock);
		if(m_onloading) m_onloading(s);
	}

	return !m_loading.needs_cancel();
}

void dummy_backend::set_ranges(p2p::list &list, bool block) {
	// do nothing.
}

driver_backend::driver_backend() {
}

driver_backend::~driver_backend() {
}

void driver_backend::setblock(bool block) {
	m_filter.setblock(block);
}

void driver_backend::setblockhttp(bool block) {
	m_filter.setblockhttp(block);
}

void driver_backend::set_ranges(p2p::list &list, bool block) {
	m_filter.setranges(list, block);
}
