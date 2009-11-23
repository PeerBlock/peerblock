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
#include <ostream>
#include "list.hpp"
#include "key.hpp"
#include "../platform.hpp"

namespace p2p {
	// saves a list in P2P format.  only IPv4 addresses are saved.
	COMMON_EXPORT void save_p2p(std::ostream &os, const list &l, bool utf8 = true);

	// saves a list in DAT format.  only IPv4 addresses are saved.
	// if level is < 127, the list is blocked.  if level is >= 127, the list is allowed.
	COMMON_EXPORT void save_dat(std::ostream &os, const list &l, unsigned int level, bool utf8 = true);

	// saves a list in P2Bv1 ISO-9985-1 format.  only IPv4 addresses are saved.
	COMMON_EXPORT void save_p2b1(std::ostream &os, const list &l);
	
	// saves a list in P2Bv2 UTF-8 format.  only IPv4 addresses are saved.
	COMMON_EXPORT void save_p2b2(std::ostream &os, const list &l);
	
	// saves a list in P2Bv3 UTF-8 format.  only IPv4 addresses are saved.
	COMMON_EXPORT void save_p2b3(std::ostream &os, const list &l);
	
	// saves a list in P2Bv4 UTF-8 format.  both IPv4 and IPv6 addresses are saved.
	COMMON_EXPORT void save_p2b4(std::ostream &os, const list &l, key *k = 0);
};
