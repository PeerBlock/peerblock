/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2014 PeerBlock, LLC

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

namespace p2p {

//============================================================================================
//
//  _load_p2b()
//
//    - Called by list::load()
//
/// <summary>
///   Reads our list in from a binary "p2b" file.
/// </summary>
//
void list::_load_p2b(istream &stream) {
	char buf[7];
	unsigned char version;
	if(
		!stream.read(buf, sizeof(buf)) ||
		memcmp(buf, "\xFF\xFF\xFF\xFFP2B", 7) ||
		!stream.read((char*)&version, sizeof(version))
	) {
        std::stringstream ss;
        ss << "invalid p2b stream (no version) while parsing stream for loading";
        if (!_loadpath.empty()) {
            ss << " file:[" + _loadpath + "]";
        }
        throw p2p_error(ss.str().c_str());
    }

	if(version==1 || version==2) {
		unsigned int start, end;

		string name;
		while(getline(stream, name, '\0')) {
			if(
				!stream.read((char*)&start, sizeof(start)) ||
				!stream.read((char*)&end, sizeof(end))
			) {
                std::stringstream ss;
                ss << "invalid p2b stream (name expected) while parsing stream for loading";
                if (!_loadpath.empty()) {
                    ss << " file:[" + _loadpath + "]";
                }
                throw p2p_error(ss.str().c_str());
            }

			range r;

			r.name.reserve(name.length());
			if(version==1) { // P2B v1 is expected to be ISO-8859-1 encoded.
				copy(name.begin(), name.end(), back_inserter(r.name));
			}
			else if(version==2) utf8_wchar(name, r.name);

			boost::trim(r.name);

			start=ntohl(start);
			end=ntohl(end);

			r.start=min(start,end);
			r.end=max(start,end);

			this->insert(r);
		}
	}
	else if(version==3) {
		unsigned int namecount;
		if(!stream.read((char*)&namecount, sizeof(namecount))) {
            std::stringstream ss;
            ss << "invalid p2b stream (name count expected) while parsing V3 stream for loading";
            if (!_loadpath.empty()) {
                ss << " file:[" + _loadpath + "]";
            }
            throw p2p_error(ss.str().c_str());
        }
		namecount=ntohl(namecount);

		boost::scoped_array<wstring> names(new wstring[namecount]);

		for(unsigned int i=0; i<namecount; i++) {
			string name;
			if(!getline(stream, name, '\0')) {
                std::stringstream ss;
                ss << "invalid p2b stream (name expected for name "<< i << " of " << namecount << 
                    ") while parsing V3 stream for loading";
                if (!_loadpath.empty()) {
                    ss << " file:[" + _loadpath + "]";
                }
                throw p2p_error(ss.str().c_str());
            }

			utf8_wchar(name, names[i]);
		}

		unsigned int rangecount;
		if(!stream.read((char*)&rangecount, sizeof(rangecount))) {
            std::stringstream ss;
            ss << "invalid p2b stream (range count expected) while parsing V3 stream for loading";
            if (!_loadpath.empty()) {
                ss << " file:[" + _loadpath + "]";
            }
            throw p2p_error(ss.str().c_str());
        }
		rangecount=ntohl(rangecount);

		unsigned int name, start, end;

		for(unsigned int i=0; i<rangecount; i++) {
			if(
				!stream.read((char*)&name, sizeof(name)) ||
				!stream.read((char*)&start, sizeof(start)) ||
				!stream.read((char*)&end, sizeof(end))
			) {
                std::stringstream ss;
                ss << "invalid p2b stream (range expected for range "<< i << " of " << rangecount << 
                    ") while parsing V3 stream for loading";
                if (!_loadpath.empty()) {
                    ss << " file:[" + _loadpath + "]";
                }
                throw p2p_error(ss.str().c_str());
            }

			name=ntohl(name);
			start=ntohl(start);
			end=ntohl(end);

			range r(names[name], min(start,end), max(start,end));
			boost::trim(r.name);

			this->insert(r);
		}
	}
	else {
        std::stringstream ss;
        ss << "invalid p2b stream (unknown p2b version:[" << version << "]) while parsing V3 stream for loading";
        if (!_loadpath.empty()) {
            ss << " file:[" + _loadpath + "]";
        }
        throw p2p_error(ss.str().c_str());
    }

} // End of _load_p2b()



//============================================================================================
//
//  _save_p2b()
//
//    - Called by list::save()
//
/// <summary>
///   Writes our list out to binary "p2b" file.
/// </summary>
//
void list::_save_p2b(ostream &stream) const {
	stream.write("\xFF\xFF\xFF\xFFP2B\x03", 8);

	stdext::hash_map<wstring,unsigned int> names;
	unsigned int i=0;

	{
		vector<wstring> namevec;

		// PERF:  this loop takes AWHILE to run (~20 sec in debug-mode)
		for(list::const_iterator iter=this->begin(); iter!=this->end(); ++iter)
			if(names.find(iter->name)==names.end()) {
				names[iter->name]=i++;
				namevec.push_back(iter->name);
			}

		i=htonl(i);
		stream.write((const char*)&i, sizeof(i));

		// PERF:  this loop takes a few seconds to run (~5 in debug-mode)
		for(vector<wstring>::size_type j=0; j<namevec.size(); j++) {
			string name;
			wchar_utf8(namevec[j], name);
			stream.write(name.c_str(), (streamsize)name.size()+1);
		}
	}

	i=htonl((unsigned int)this->size());
	stream.write((const char*)&i, sizeof(i));

	// PERF:  this loop doesn't take much time (one or two seconds in debug-mode)
	for(list::const_iterator iter=this->begin(); iter!=this->end(); ++iter) {
		unsigned int name=htonl(names[iter->name]);
		unsigned int start=htonl(iter->start.ipl);
		unsigned int end=htonl(iter->end.ipl);

		stream.write((const char*)&name, sizeof(name));
		stream.write((const char*)&start, sizeof(start));
		stream.write((const char*)&end, sizeof(end));
	}

} // End of _save_p2b()

} // End of namespace p2p
