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

#include <cstddef>
#include "list.hpp"
#include "key.hpp"
#include "../platform.hpp"

namespace p2p {
	enum list_type { unknown_list, p2p_list, p2b_list, dat_list };

	// classifies a list in memory as one of list_type above.
	COMMON_EXPORT list_type classify(const char *mem, std::size_t len);

	// loads a P2P list as UTF-8 if a UTF-8 BOM is detected, or ISO-8895-1 if not.
	COMMON_EXPORT void load_p2p(list &l, const char *mem, std::size_t len);

	// loads a DAT list as UTF-8 if a UTF-8 BOM is detected, or ISO-8895-1 if not.
	COMMON_EXPORT void load_dat(list &l, const char *mem, std::size_t len);
	
	// loads a P2B list.  Supports versions 1, 2, 3, and 4.
	COMMON_EXPORT void load_p2b(list &l, const char *mem, std::size_t len, key *k = 0);

	// wraps a call to classify and loads a list.
	// throws a std::runtime_error if classify returns unknown_list.
	COMMON_EXPORT void load(list &l, const char *mem, std::size_t len, key *k = 0);
};
