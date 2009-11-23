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
#include "p2p/save.hpp"
#include "mapped_file.hpp"
#include "cache.hpp"

namespace cache {

unsigned int load(p2p::list &block, global_config &config, loading_function &onload) {
	load_state state;

	state.stage = load_state::checking_cache;
	state.progress = 0.0f;
	if(!onload(state)) return cache::cancelled;

	pg3_profile &profile = config.lists.profiles[config.lists.curprofile];

	// get count for progress callback.
	std::vector<bool>::size_type count = 0;
	for(std::vector<bool>::size_type i = 0; i < profile.lists.size(); ++i) {
		if(profile.lists[i]) ++count;
	}

	unsigned int result = cache::ok;

	path cachepath = profile.cache_path();
	
	boost::crc_32_type crc;

	if(path::exists(cachepath)) {
		for(std::vector<bool>::size_type i = 0; i < profile.lists.size(); ++i) {
			if(profile.lists[i]) {
				path p = config.lists.lists[i].real_path();

				if(path::exists(p)) {
					mapped_file mf(p.c_str());

					if(mf.size()) {
						crc.process_bytes(mf.begin(), mf.size());
					}
					else {
						result |= cache::empty_list;
					}
				}
				else {
					result |= path::is_url(config.lists.lists[i].path.c_str()) ? cache::need_update : cache::missing_file;
				}
				
				state.progress = (i + 1) / (float)count;
				if(!onload(state)) return cache::cancelled;
			}
		}

		if(crc.checksum() == profile.crc) {
			state.stage = load_state::loading_cache;
			state.progress = 0.0f;
			onload(state);

			load_list(block, cachepath.c_str());

			state.stage = load_state::loading_cache;
			state.progress = 1.0f;
			if(!onload(state)) return cache::cancelled;

			return result;
		}
	}

	// cache is non-existant or dirty, rebuild.

	state.stage = load_state::loading;
	state.progress = 0.0f;
	if(!onload(state)) return cache::cancelled;

	p2p::list allow;

	crc.reset();

	for(std::vector<bool>::size_type i = 0; i < profile.lists.size(); ++i) {
		if(profile.lists[i]) {
			pg3_list &pgl = config.lists.lists[i];
			path p = pgl.real_path();

			if(path::exists(p)) {
				mapped_file mf(p.c_str());

				if(mf.size()) {
					crc.process_bytes(mf.begin(), mf.size());

					try {
						load_list(pgl.block ? block : allow, mf.begin(), mf.size());
						pgl.failedload = false;
					}
					catch(...) {
						pgl.failedload = true;
						result |= cache::invalid_list;
					}
				}
				else {
					result |= cache::empty_list;
				}
			}
			else {
				result |= path::is_url(pgl.path.c_str()) ? cache::need_update : cache::missing_file;
				pgl.failedload = false;
			}

			state.progress = (i + 1) / (float)count;
			if(!onload(state)) return cache::cancelled;
		}
	}

	state.stage = load_state::optimizing;
	state.progress = 0.0f;
	if(!onload(state)) return cache::cancelled;

	block.optimize();

	state.progress = 0.5f;
	if(!onload(state)) return cache::cancelled;

	block.erase(allow);

	state.progress = 1.0f;
	if(!onload(state)) return cache::cancelled;

	state.stage = load_state::saving_cache;
	state.progress = 0.0f;

	{
		std::ofstream ofs(cachepath.c_str(), std::ios_base::out | std::ios_base::binary);
		p2p::save_p2b4(ofs, block);
	}

	state.progress = 1.0f;
	onload(state);

	profile.crc = crc.checksum();
	
	return result;
}

}
