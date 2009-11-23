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

#include <list>
#include <algorithm>

#include "ip.hpp"
#include "range.hpp"
#include "basic_compact_list.hpp"

namespace p2p {

template<typename Ip>
class basic_compact_list;

/*
	basic_list stores IP ranges in a list.  It is made to be used for editing
	a list only, and thus has no search capabilities.
*/
template<typename Ip>
class basic_list {
public:
	typedef Ip ip_type;
	typedef range<ip_type> range_type;
	typedef compact_range<ip_type> compact_type;

	typedef typename std::list<range_type>::iterator iterator;
	typedef typename std::list<range_type>::const_iterator const_iterator;
	typedef typename std::list<range_type>::size_type size_type;

	// inserts an empty range and returns a reference to it.
	range_type& insert() { m_ranges.push_back(range_type()); return m_ranges.back(); }

	// inserts a range.
	void insert(const range_type &r) { m_ranges.push_back(r); }

	// inserts another list.  consider using splice() or merge() instead.
	void insert(const basic_list<ip_type> &l) { m_ranges.insert(m_ranges.end(), l.m_ranges.begin(), l.m_ranges.end()); }

	// erases a single range of IPs.
	void erase(const compact_type &r) {
		list_type nr;
		m_ranges.remove_if(erase_pred(nr, r));
		m_ranges.splice(m_ranges.end(), nr);
	}

	// erases a list of IPs.  basic_compact_list is constructable from a basic_list.
	void erase(const basic_compact_list<ip_type> &list) {
		list_type nr;
		m_ranges.remove_if(masserase_pred(nr, list));

		for(;;) {
			list_type nnr;
			nr.remove_if(masserase_pred(nnr, list));

			if(nnr.empty()) break;
			nr.splice(nr.end(), nnr);
		}

		m_ranges.splice(m_ranges.end(), nr);
	}

	// merges two lists.  both must be sorted prior to a merge.
	void merge(basic_list<ip_type> &list) { m_ranges.merge(list.m_ranges); }

	// splices a list onto the end another.
	void splice(basic_list<ip_type> &list) { m_ranges.splice(m_ranges.end(), list.m_ranges); }

	// sorts in ascending order of the starting IP in each range.
	void sort() { m_ranges.sort(); }

	// optimizes ranges.  must be sorted prior to calling.
	// always merges colliding ranges.  merges adjacent ranges if their labels match.
	void optimize() { m_ranges.unique(merge_pred); }

	// iterators.
	iterator begin() { return m_ranges.begin(); }
	const_iterator begin() const { return m_ranges.begin(); }

	iterator end() { return m_ranges.end(); }
	const_iterator end() const { return m_ranges.end(); }

	// O(n) size.
	size_type size() const { return m_ranges.size(); }

	// for completeness.
	bool empty() const { return m_ranges.empty(); }

private:
	typedef std::list<range_type> list_type;

	// determines if a range needs to be erased,
	// and splits ranges if needed.
	class erase_pred {
	public:
		erase_pred(list_type &nr, const compact_type &r) : m_nr(nr),m_r(r) {}

		bool operator()(range_type &r) const {
			if(m_r.contains(r)) {
				return true;
			}

			if(r.contains(m_r)) {
				ip_type newend = m_r.start;

				m_nr.push_back(range_type(r.label, r.start, --newend));
				r.start = m_r.end; ++r.start;
			}
			else if(m_r.contains(r.start)) {
				r.start = m_r.end; ++r.start;
			}
			else if(m_r.contains(r.end)) {
				r.end = m_r.start; --r.end;
			}

			return false;
		}

	private:
		list_type &m_nr;
		const compact_type &m_r;
	};

	// performs a mass-erase using a basic_compact_list.
	class masserase_pred {
	public:
		masserase_pred(list_type &nr, const basic_compact_list<ip_type> &list) : m_nr(nr),m_list(list) {}

		bool operator()(range_type &r) const {
			while(const compact_type *cr = m_list(r)) {
				if(erase_pred(m_nr, *cr)(r)) return true;
			}

			return false;
		}

	private:
		list_type &m_nr;
		const basic_compact_list<ip_type> &m_list;
	};

	// returns true and merges b into a if a intersects b, b intersects a, or a is adjacent to b and the labels match.
	static bool merge_pred(range_type &a, const range_type &b) {
		if(a.contains(b.start) || (range_type::adjacent(a, b) && a.label == b.label)) {
			a.start = std::min(a.start, b.start);
			a.end = std::max(a.end, b.end);
			return true;
		}

		return false;
	}

	list_type m_ranges;
};

typedef basic_list<ip4> list4;
typedef basic_list<ip6> list6;

}
