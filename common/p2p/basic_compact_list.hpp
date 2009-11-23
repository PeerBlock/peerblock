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

#include <vector>
#include <algorithm>

#include "ip.hpp"
#include "range.hpp"
#include "basic_list.hpp"

namespace p2p {

template<typename Ip>
class basic_list;

/*
	basic_compact_list stores IP ranges without a label, in a compact fashion
	made for minimal memory and CPU usage.
*/
template<typename Ip>
class basic_compact_list {
public:
	typedef Ip ip_type;
	typedef compact_range<ip_type> range_type;

private:
	typedef std::vector<range_type> vec_type;

public:
	typedef typename vec_type::const_iterator const_iterator;

	// constructs an empty list.
	basic_compact_list() {}

	// constructs a list using a basic_list.  ranges are optimized.
	basic_compact_list(const basic_list<ip_type> &list) {
		m_ranges.insert(m_ranges.end(), list.begin(), list.end());

		std::sort(m_ranges.begin(), m_ranges.end());

		typename vec_type::iterator iter = std::unique(m_ranges.begin(), m_ranges.end(), merge_pred);
		if(iter != m_ranges.end()) m_ranges.erase(iter, m_ranges.end());
	}

	// returns a range that contains an IP, or NULL.
	const range_type* operator()(const ip_type &ip) const {
		return (*this)(range_type(ip));
	}

	// returns a range that contains a range, or NULL.
	const range_type* operator()(const range_type &r) const {
		const_iterator begin = m_ranges.begin();
		const_iterator end = m_ranges.end();

		const_iterator iter = std::lower_bound(begin, end, r);

		std::size_t num = m_ranges.size();

		if(iter != end) {
			if(iter->start != r.start && iter != begin) --iter;
		}
		else if(iter != begin) {
			--iter;
		}

		return (iter != end && iter->contains(r)) ? &*iter : 0;
	}

	// performs a fast swap between two lists.
	void swap(basic_compact_list<ip_type> &list) { m_ranges.swap(list.m_ranges); }

	// iterators.
	const_iterator begin() const { return m_ranges.begin(); }
	const_iterator end() const { return m_ranges.end(); }

	// constant-time size().
	std::size_t size() const { return m_ranges.size(); }

private:
	// returns true and merges b into a if a intersects b, b intersects a, or a is adjacent to b.
	static bool merge_pred(range_type &a, const range_type &b) {
		if(a.contains(b.start) || range_type::adjacent(a, b)) {
			a.start = std::min(a.start, b.start);
			a.end = std::max(a.end, b.end);
			return true;
		}

		return false;
	}

	vec_type m_ranges;
};

typedef basic_compact_list<ip4> compact_list4;
typedef basic_compact_list<ip6> compact_list6;

}
