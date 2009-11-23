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
#include "compact_list.hpp"

namespace p2p {

/*
	compact_list wraps two basic_compact_lists, one for IPv4 and one for IPv6.
	compact_list stores IP ranges without a label, in a compact fashion made for
	minimal memory and CPU usage.
*/

compact_list::compact_list() {}
compact_list::compact_list(const list &list) : m_list4(list.m_list4),m_list6(list.m_list6) {}
compact_list::~compact_list() {}

const compact_range4* compact_list::operator()(const ip4 &ip) const { return m_list4(ip); }
const compact_range6* compact_list::operator()(const ip6 &ip) const { return m_list6(ip); }

const compact_range4* compact_list::operator()(const compact_range4 &r) const { return m_list4(r); }
const compact_range6* compact_list::operator()(const compact_range6 &r) const { return m_list6(r); }

void compact_list::swap(compact_list &list) {
	m_list4.swap(list.m_list4);
	m_list6.swap(list.m_list6);
}

}
