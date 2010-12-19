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

#include "stdafx.h"
using namespace std;

typedef p2p::compact_list::range_type range_type;

struct add_pred {
	unsigned int res;

	add_pred() : res(0) {}
	void operator()(const range_type &r) {
		res+=r.second-r.first+1;
	}
};

inline static bool operator<(const range_type &left, const range_type &right) {
	return
		(left.first<right.first) |
		(left.first==right.first && left.second<right.second);
}

inline static bool mergepred(range_type &left, const range_type &right) {
	if(left.first<=right.first && right.first<=left.second) {
		left.first=min(left.first, right.first);
		left.second=max(left.second, right.second);
		return true;
	}
	else return false;
}

namespace p2p {
compact_list::compact_list(const list &l) : _ranges(new range_type[l.size()]),_rangecount((int)l.size()) {
	size_t i=0;
	for(list::const_iterator iter=l.begin(); iter!=l.end(); ++iter) {
		range_type &r=_ranges[i++];
		r.first=iter->start.ipl;
		r.second=iter->end.ipl;
	}

	sort(_ranges.get(), _ranges.get()+_rangecount);
	range_type *r=unique(_ranges.get(), _ranges.get()+_rangecount, mergepred);
	_rangecount=(int)distance(_ranges.get(), r);
}

unsigned int compact_list::ip_count() const {
	add_pred p;
	for_each<range_type*,add_pred&>(_ranges.get(), _ranges.get()+_rangecount, p);
	return p.res;
}

const compact_list::range_type *compact_list::operator()(const range &r) const {
	int high=_rangecount, low=-1, probe;

	while(high-low > 1) {
		probe=(high+low)/2;
		if(_ranges[probe].first>r.end.ipl)
			high=probe;
		else low=probe;
	}

	if(low==-1 || _ranges[low].first>r.end.ipl || r.start.ipl>_ranges[low].second) return NULL;
	else return &_ranges[low];
}

const compact_list::range_type *compact_list::operator()(const range_type &r) const {
	int high=_rangecount, low=-1, probe;

	while(high-low > 1) {
		probe=(high+low)/2;
		if(_ranges[probe].first>r.second)
			high=probe;
		else low=probe;
	}

	if(low==-1 || _ranges[low].first>r.second || r.first>_ranges[low].second) return NULL;
	else return &_ranges[low];
}

const compact_list::range_type *compact_list::operator()(unsigned int ip) const {
	int high=_rangecount, low=-1, probe;

	while(high-low > 1) {
		probe=(high+low)/2;
		if(_ranges[probe].first>ip)
			high=probe;
		else low=probe;
	}

	if(low==-1 || _ranges[low].first>ip || ip>_ranges[low].second) return NULL;
	else return &_ranges[low];
}
}
