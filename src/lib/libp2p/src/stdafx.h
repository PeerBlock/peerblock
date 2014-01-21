/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2014 PeerBlock, LLC

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

#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <hash_map>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <boost/scoped_array.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <climits>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#ifndef htonl
#define htonl(A)  ((((unsigned int)(A) & 0xff000000) >> 24) | \
                   (((unsigned int)(A) & 0x00ff0000) >> 8)  | \
                   (((unsigned int)(A) & 0x0000ff00) << 8)  | \
                   (((unsigned int)(A) & 0x000000ff) << 24))
#endif
#ifndef ntohl
#define ntohl     htonl
#endif

#include <p2p/ip.hpp>
#include <p2p/range.hpp>
#include <p2p/list.hpp>
#include <p2p/compact_list.hpp>
#include <p2p/exception.hpp>

#include "utf8.h"

#endif
