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

#include "p2p/list.hpp"
#include "config.hpp"

namespace cache {
	enum {
		ok					= 0,
		missing_file	= 1,
		need_update		= 2,
		empty_list		= 4,
		invalid_list	= 8,
		cancelled		= 16
	};

	struct load_state {
		enum { checking_cache, loading_cache, loading, optimizing, saving_cache } stage;
		float progress;
	};

	// return false to immediately cancel loading.  load returns 'cancelled'.
	typedef boost::function<bool(const load_state&)> loading_function;

	// this loads a profile cache.  if it is missing or dirty, it regenerates it.
	// onload is called throughout the operation to notify the caller of status.
	COMMON_EXPORT unsigned int load(p2p::list &block, global_config &config, loading_function &onload);
};
