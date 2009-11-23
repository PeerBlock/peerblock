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
#include <string>
#include <vector>
#include <hash_map>
#include <boost/scoped_ptr.hpp>

namespace p2p {

/*
	string_pool pools many strings in a single buffer for low memory usage.
*/
class string_pool {
public:
	// constructs an empty string pool.
	string_pool() : m_map(new map_type) {}

	// inserts a string into the pool and returns its offset.
	// add the offset to base() to get the pooled string.
	std::size_t insert(const std::wstring &str) {
		std::size_t &idx = (*m_map)[str];

		if(!idx) {
			idx = m_buf.size();
			m_buf.insert(m_buf.end(), str.begin(), str.end());
			m_buf.push_back(L'\0');
		}

		return idx;
	}
	
	// freezes the string_pool.  after this is called, no other strings can
	// be added to the pool.
	void freeze() { m_map.reset(); }

	// returns the base address of the memory buffer.  the returned value is
	// volatile until freeze() is called.
	const wchar_t* base() const { return &m_buf.front(); }

	// performs a fast swap of two string pools.
	void swap(string_pool &pool) {
		m_buf.swap(pool.m_buf);
		m_map.swap(pool.m_map);
	}

private:
	typedef stdext::hash_map<std::wstring,std::size_t> map_type;
	typedef std::vector<wchar_t> vec_type;

	vec_type m_buf;
	boost::scoped_ptr<map_type> m_map;
};

}
