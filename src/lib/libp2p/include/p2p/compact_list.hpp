/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2010 PeerBlock, LLC

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

#ifndef __P2P_COMPACT_LIST_HPP__
#define __P2P_COMPACT_LIST_HPP__

#include <utility>
#include <boost/scoped_array.hpp>
#include <p2p/list.hpp>
#include <p2p/range.hpp>

namespace p2p {
class compact_list {
public:
	typedef std::pair<unsigned int,unsigned int> range_type;

private:
	boost::scoped_array<range_type> _ranges;
	int _rangecount;

public:
	compact_list(const class list &l);

	int size() const {
		return _rangecount;
	}
	unsigned int ip_count() const;

	const range_type& operator[](int index) const {
		return _ranges[index];
	}

	const range_type *operator()(unsigned int ip) const;
	const range_type *operator()(const range_type &r) const;
	const range_type *operator()(const struct range &r) const;
};
}

#endif
