/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2010-2014 PeerBlock, LLC

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

#ifndef __P2P_LIST_HPP__
#define __P2P_LIST_HPP__

#include <list>
#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include <utility>
#include <algorithm>
#include <boost/scoped_array.hpp>
#include <p2p/range.hpp>
#include <p2p/compact_list.hpp>

namespace p2p {
class list {
public:
	typedef range range_type;
	typedef std::list<range_type> list_type;
	typedef list_type::size_type size_type;
	typedef list_type::iterator iterator;
	typedef list_type::const_iterator const_iterator;
	typedef std::istream istream_type;
	typedef std::ostream ostream_type;
	typedef std::string path_type;

	enum file_type {
		file_auto,
		file_p2p,
		file_p2b
	};

private:
	list_type _ranges;
    path_type _loadpath;
    path_type _savepath;

	void _load_p2p(istream_type &stream);
	void _save_p2p(ostream_type &stream) const;

	void _load_p2b(istream_type &stream);
	void _save_p2b(ostream_type &stream) const;

public:
	void insert(const range &r);
	void insert(const list &l);
	void erase(const range &r);
	void erase(const class compact_list &l);

	void optimize(bool aggressive=false);

	iterator begin() {
		return this->_ranges.begin();
	}
	iterator end() {
		return this->_ranges.end();
	}

	const_iterator begin() const {
		return this->_ranges.begin();
	}
	const_iterator end() const {
		return this->_ranges.end();
	}

	size_type size() const {
		return this->_ranges.size();
	}
	void clear() {
		this->_ranges.clear();
	}

	void load(istream_type &stream, file_type type=file_auto);
	void load(const path_type &file, file_type type=file_auto);
	void save(ostream_type &stream, file_type type) const;
	void save(const path_type &file, file_type type);

	list() : _loadpath(""), _savepath("") {}
	list(const path_type &file, file_type type=file_auto) : _loadpath(file) {
		this->load(file, type);
	}
	list(istream_type &stream, file_type type=file_auto) {
		this->load(stream, type);
	}
};
}

#endif
