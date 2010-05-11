/*
	Copyright (C) 2004-2005 Cory Nelson

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

inline static unsigned int make_ip(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
	return ((a<<24) | (b<<16) | (c<<8) | d);
}

namespace p2p {
	void list::_load_p2p(istream &stream) {
		string line;
		while(getline(stream, line)) {
			string::size_type i=line.rfind(':');
			if(i==string::npos) continue;

			string name(line.c_str(), i);
			line.erase(0, i+1);
			
			unsigned int sa, sb, sc, sd;
			unsigned int ea, eb, ec, ed;

			if(sscanf(line.c_str(), "%u.%u.%u.%u-%u.%u.%u.%u",
				&sa, &sb, &sc, &sd,
				&ea, &eb, &ec, &ed)!=8 ||
				sa>255 || sb>255 || sc>255 || sd>255 ||
				ea>255 || eb>255 || ec>255 || ed>255) continue;

			boost::trim(name);
			range r;

			// P2P format is expected to be ISO-8859-1 encoded.
			r.name.reserve(name.size());
			copy(name.begin(), name.end(), back_inserter(r.name));

			unsigned int start=make_ip(sa, sb, sc, sd);
			unsigned int end=make_ip(ea, eb, ec, ed);

			r.start=min(start,end);
			r.end=max(start,end);

			this->insert(r);
		}
	}

	void list::_save_p2p(ostream &stream) const {
		for(list::const_iterator iter=this->begin(); iter!=this->end(); iter++) {
			string name;
			name.reserve(iter->name.size());

			for(wstring::size_type i=0; i<iter->name.size(); i++)
				name+=(char)iter->name[i];

			stream
				<< name
				<< ':'
				<< (int)iter->start.ipb[3] << '.'
				<< (int)iter->start.ipb[2] << '.'
				<< (int)iter->start.ipb[1] << '.'
				<< (int)iter->start.ipb[0]
				<< '-'
				<< (int)iter->end.ipb[3] << '.'
				<< (int)iter->end.ipb[2] << '.'
				<< (int)iter->end.ipb[1] << '.'
				<< (int)iter->end.ipb[0]
				<< endl;
		}
	}
}
