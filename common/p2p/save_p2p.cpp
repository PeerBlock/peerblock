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
#include "save.hpp"

namespace p2p {

// saves a list in P2P format.  only IPv4 addresses are saved.
void save_p2p(std::ostream &os, const list &l, bool utf8) {
	if(utf8) {
		os.write("\xEF\xBB\xBF", 3);
	}

	std::vector<char> label;

	for(list4::const_iterator iter = l.m_list4.begin(), end = l.m_list4.end(); iter != end; ++iter) {
		label.clear();

		if(utf8) {
			unicode::transcode<unicode::wchar_encoding, unicode::utf8>(
				iter->label.begin(), iter->label.end(),
				std::back_inserter(label)
			);
		}
		else {
			// ISO-8895-1
			label.reserve(iter->label.size());

			for(std::wstring::const_iterator liter = iter->label.begin(), lend = iter->label.end(); liter != lend; ++liter) {
				label.push_back((char)*liter);
			}
		}

		label.push_back('\0');

		char range[64];
		sprintf(range, ":%u.%u.%u.%u-%u.%u.%u.%u\r\n",
			iter->start >> 24, iter->start >> 16 & 0xFF, iter->start >> 8 & 0xFF, iter->start & 0xFF,
			iter->end >> 24, iter->end >> 16 & 0xFF, iter->end >> 8 & 0xFF, iter->end & 0xFF);

		os << (&label.front()) << range;
	}
}

}
