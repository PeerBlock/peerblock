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

#include "range.hpp"
#include "basic_list.hpp"
#include "basic_compact_list.hpp"
#include "compact_list.hpp"
#include "../platform.hpp"

namespace p2p {

class compact_list;

/*
	list wraps two basic_lists, one for IPv4 and one for IPv6.
	list is made to be used for editing a list only, and thus has no search
	capabilities.
*/
class list {
public:
	COMMON_EXPORT list();
	COMMON_EXPORT ~list();

	COMMON_EXPORT void insert(const range4 &r);
	COMMON_EXPORT void insert(const range6 &r);

	COMMON_EXPORT void insert(const list4 &l);
	COMMON_EXPORT void insert(const list6 &l);
	COMMON_EXPORT void insert(const list &l);
	
	COMMON_EXPORT void erase(const compact_range4 &r);
	COMMON_EXPORT void erase(const compact_range6 &r);

	COMMON_EXPORT void erase(const compact_list4 &l);
	COMMON_EXPORT void erase(const compact_list6 &l);
	COMMON_EXPORT void erase(const compact_list &l);

	COMMON_EXPORT void merge(list &l);
	COMMON_EXPORT void splice(list &l);

	COMMON_EXPORT void sort();
	COMMON_EXPORT void optimize();

//private:
	list4 m_list4;
	list6 m_list6;
};

}
