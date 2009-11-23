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
#include "list.hpp"

namespace p2p {

/*
	list wraps two basic_lists, one for IPv4 and one for IPv6.
	list is made to be used for editing a list only, and thus has no search
	capabilities.
*/

list::list() {}
list::~list() {}

void list::insert(const range4 &r) { m_list4.insert(r); }
void list::insert(const range6 &r) { m_list6.insert(r); }

void list::insert(const list4 &l) { m_list4.insert(l); }
void list::insert(const list6 &l) { m_list6.insert(l); }
void list::insert(const list &l) { m_list4.insert(l.m_list4); m_list6.insert(l.m_list6); }

void list::erase(const compact_range4 &r) { m_list4.erase(r); }
void list::erase(const compact_range6 &r) { m_list6.erase(r); }

void list::erase(const compact_list4 &l) { m_list4.erase(l); }
void list::erase(const compact_list6 &l) { m_list6.erase(l); }
void list::erase(const compact_list &l) { m_list4.erase(l.m_list4); m_list6.erase(l.m_list6); }

void list::merge(list &l) { m_list4.merge(l.m_list4); m_list6.merge(l.m_list6); }
void list::splice(list &l) { m_list4.splice(l.m_list4); m_list6.splice(l.m_list6); }

void list::sort() { m_list4.sort(); m_list6.sort(); }
void list::optimize() { m_list4.optimize(); m_list6.optimize(); }

}
