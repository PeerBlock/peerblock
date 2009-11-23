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
#include "load.hpp"

namespace p2p {

namespace detail {
	bool is_newline(char ch) { return ch == '\r' || ch == '\n'; }
	bool is_whitespace(char ch) { return ch == '\r' || ch == '\n' || ch == '\t' || ch == ' '; }
	bool not_whitespace(char ch) { return ch != '\r' && ch != '\n' && ch != '\t' && ch != ' '; }
}

// classifies a list in memory as one of list_type.
list_type classify(const char *mem, std::size_t len) {
	if(len >= 8 && !memcmp(mem, "\xFF\xFF\xFF\xFFP2B", 7)) {
		return p2b_list;
	}
	else {
		// skip UTF-8 BOM if one is there.
		if(len >= 3 && !memcmp(mem, "\xEF\xBB\xBF", 3)) {
			mem += 3;
			len -= 3;
		}
		
		const char *iter = mem;
		const char *end = iter + len;

		// find first non-empty non-comment line, and its end.
		const char *lend = iter;
		do {
			iter = std::find_if(lend, end, detail::not_whitespace);
			lend = std::find_if(iter, end, detail::is_newline);
		} while(iter < end && iter[0] == '#');

		if(iter < end) {
			typedef std::reverse_iterator<const char*> riter_type;

			unsigned int sa, sb, sc, sd;
			unsigned int ea, eb, ec, ed;

			// test for P2P format.

			const char *rstart = std::find(riter_type(lend), riter_type(iter), ':').base();
			if(rstart != lend) {
				//P2P format.
				
				int ret = _snscanf(rstart, (std::size_t)(lend - rstart), "%u.%u.%u.%u-%u.%u.%u.%u",
					&sa, &sb, &sc, &sd, &ea, &eb, &ec, &ed);
				
				if(ret == 8) return p2p_list;
			}
			else {
				//DAT format.

				unsigned int level;

				int ret = _snscanf(iter, (std::size_t)(lend - iter), "%u.%u.%u.%u , %u.%u.%u.%u , %u",
					&sa, &sb, &sc, &sd, &ea, &eb, &ec, &ed, &level);

				if(ret == 9) return dat_list;
			}
		}
	}

	return unknown_list;
}

// wraps a call to classify and loads a list.
// throws a std::runtime_error if classify returns unknown_list.
void load(list &l, const char *mem, std::size_t len, key *k) {
	list_type t = classify(mem, len);

	if(k && t != p2b_list) {
		throw std::runtime_error("digital signatures unsupported for list type");
	}

	switch(t) {
		case p2b_list: load_p2b(l, mem, len, k); break;
		case p2p_list: load_p2p(l, mem, len); break;
		case dat_list: load_dat(l, mem, len); break;
		case unknown_list:
			throw std::runtime_error("unknown list type");
	}
}

}
