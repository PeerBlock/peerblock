/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC

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

//#include "../pb/tracelog.h"
//extern TraceLog g_tlog;

namespace p2p {

void list::insert(const range &r) {
	this->_ranges.push_back(r);
}

void list::insert(const list &l) {
	this->_ranges.insert(this->_ranges.end(), l._ranges.begin(), l._ranges.end());
}

template<class T>
inline bool is_between(const T &low, const T &value, const T &high) {
	return (low<=value && value<=high);
}

template<class T>
inline bool is_between(const T &low, const T &value_low, const T &value_high, const T &high) {
	return (low<=value_low && value_high<=high);
}

template<typename ContainerT>
class erase_pred {
private:
	ContainerT &nr;
	const pair<unsigned int,unsigned int> &er;

public:
	erase_pred(ContainerT &newranges, const pair<unsigned int,unsigned int> &r) : er(r),nr(newranges) {}

	bool operator()(list::range_type &r) const {
		if(is_between(er.first, r.start.ipl, r.end.ipl, er.second))
			return true;
		else if(r.start.ipl!=er.first && r.end.ipl!=er.second && is_between(r.start.ipl, er.first, er.second, r.end.ipl)) {
			this->nr.push_back(range(r.name, r.start, er.first-1));
			r.start=er.second+1;
			return false;
		}
		else if(is_between(er.first, r.start.ipl, er.second)) {
			r.start=er.second+1;
			return false;
		}
		else if(is_between(er.first, r.end.ipl, er.second)) {
			r.end=er.first-1;
			return false;
		}
		else return false;
	}
};

void list::erase(const range &r) {
	const pair<unsigned int,unsigned int> p(r.start.ipl, r.end.ipl);

	std::list<range> nr;

	this->_ranges.remove_if(erase_pred<std::list<range> >(nr, p));
	this->_ranges.insert(this->_ranges.end(), nr.begin(), nr.end());
}

template<typename ContainerT>
class masserase_pred {
private:
	const compact_list &allowed;
	ContainerT &nr;

public:
	masserase_pred(const compact_list &a, ContainerT &newranges)
		: allowed(a),nr(newranges) {}

	bool operator()(list::range_type &r) const {
		while(const pair<unsigned int,unsigned int> *a=allowed(r))
			if(erase_pred<ContainerT>(nr, *a)(r)) return true;

		return false;
	}
};

void list::erase(const compact_list &l) {
	std::list<range> nr;
	this->_ranges.remove_if(masserase_pred<std::list<range> >(l, nr));

	//TODO: find a better way.
	while(true) {
		std::list<range> nnr;

		nr.remove_if(masserase_pred<std::list<range> >(l, nnr));

		if(nnr.size()==0) break;
		nr.insert(nr.end(), nnr.begin(), nnr.end());
	}

	this->_ranges.insert(this->_ranges.end(), nr.begin(), nr.end());
}

class merge_pred {
private:
	bool aggressive;

	inline static bool is_adjacent(const range &left, const range &right) {
		return (left.end==right.start-1);
	}

	inline static bool is_semiadjacent(const range &left, const range &right) {
		return (
				   left.end.ipb[0]==255
				   && left.end==right.start-2
			   );
	}

public:
	merge_pred(bool a) : aggressive(a) {}

	bool operator()(range &left, const range &right) const {
		if(
			is_between(left.start, right.start, left.end)
			|| (is_adjacent(left, right) && (this->aggressive || left.name==right.name))
		) {
			left.start=min(left.start, right.start);
			left.end=max(left.end, right.end);
			return true;
		}
		else return false;
	}
};

void list::optimize(bool aggressive) {
	this->_ranges.sort();
	this->_ranges.unique(merge_pred(aggressive));
}

static list::file_type get_file_type(list::istream_type &stream) {
	char buf[6];
	stream.read(buf, 6);

	stream.putback(buf[5]);
	stream.putback(buf[4]);
	stream.putback(buf[3]);
	stream.putback(buf[2]);
	stream.putback(buf[1]);
	stream.putback(buf[0]);

	if(memcmp(buf, "\xFF\xFF\xFF\xFFP2B", 6)==0) return list::file_p2b;
	else return list::file_p2p;
}

void list::load(istream_type &stream, file_type type) {
	switch(type) {
	case file_auto:
		this->load(stream, get_file_type(stream));
		break;
	case file_p2p:
		this->_load_p2p(stream);
		break;
	case file_p2b:
		this->_load_p2b(stream);
		break;
	default:
		throw invalid_argument("invalid type");
	}
}

void list::load(const path_type &file, file_type type) {
	ifstream fs(file.c_str(), ifstream::binary);
	if(!fs.is_open()) throw runtime_error("unable to open file");

	this->load(fs, type);
}



//================================================================================================
//
//  save()
//
//    - Called by GenCacheFuncs Process()
//
/// <summary>
///   Saves our list out to-file..
/// </summary>
//
void list::save(ostream_type &stream, file_type type) const {
//	TRACEI("[list] [save]  > Entering routine.");
	switch(type) {
	case file_p2p:
//			TRACEI("[list] [save]    saving p2p file.");
		this->_save_p2p(stream);
		break;
	case file_p2b:
//			TRACEI("[list] [save]    saving p2b file.");
		this->_save_p2b(stream);
		break;
	default:
//			TRACEE("[list] [save]    ERROR: invalid type of file.");
		throw invalid_argument("invalid type");
	}
//	TRACEI("[list] [save]  < Leaving routine.");

} // End of save()



void list::save(const path_type &file, file_type type) const {
	ofstream fs(file.c_str(), ofstream::binary);
	if(!fs.is_open()) throw p2p_error("unable to open file");

	this->save(fs, type);
}

}
