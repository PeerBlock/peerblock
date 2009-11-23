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
#include "../unicode.hpp"
#include "load.hpp"

namespace p2p {

namespace detail {
	bool is_newline(char ch);
	bool is_whitespace(char ch);
	bool not_whitespace(char ch);

	// returns an ip4, or 0 on error.
	ip4 parse_ip4(const char *iter, const char *end) {
		unsigned int a, b, c, d;

		if(_snscanf(iter, std::size_t(end - iter), "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
			if(a >= 1 && a <= 255 && b <= 255 && c <= 255 && d <= 255) {
				return (a << 24) | (b << 16) | (c << 8) | d;
			}
		}

		return 0;
	}
}

// loads a P2P list as UTF-8 if a UTF-8 BOM is detected, or ISO-8895-1 if not.
void load_p2p(list &l, const char *iter, std::size_t len) {
	const char* const end = iter + len;

	bool utf8;

	if(len >= 3 && !memcmp(iter, "\xEF\xBB\xBF", 3)) {
		utf8 = true;
		iter += 3;
	}
	else {
		utf8 = false;
	}

	while(iter < end) {
		// skip empty lines and comments, set mem to first line and lend to end of it.
		const char *lend = iter;
		do {
			iter = std::find_if(lend, end, detail::not_whitespace);
			lend = std::find_if(iter, end, detail::is_newline);
		} while(iter < end && iter[0] == '#');
		
		if(iter < end) {
			typedef std::reverse_iterator<const char*> riter_type;

			const char *dash = std::find(riter_type(lend), riter_type(iter), '-').base();

			// invalid line, no dash to seperate IPs.
			if(dash-- == iter) {
				iter = lend;
				continue;
			}

			const char *colon = std::find(riter_type(dash), riter_type(iter), ':').base();
			
			// invalid line, no colon to seperate name & IPs.
			if(colon-- == iter) {
				iter = lend;
				continue;
			}
			
			ip4 rstart = detail::parse_ip4(colon + 1, dash);
			ip4 rend = detail::parse_ip4(dash + 1, lend);

			// invalid line, IPs aren't parsing.
			if(!rstart && !rend) {
				iter = lend;
				continue;
			}

			colon = std::find_if(riter_type(colon), riter_type(iter), detail::not_whitespace).base();

			range4 &r = l.m_list4.insert();

			r.start = std::min(rstart, rend);
			r.end = std::max(rstart, rend);
			
			if(utf8) {
				unicode::transcode<unicode::utf8, unicode::wchar_encoding>(iter, colon, std::inserter(r.label, r.label.end()));
			}
			else {
				r.label.assign(iter, colon);
			}

			iter = lend;
		}
	}
}

}
