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

namespace detail {

#pragma pack(push, 1)

	struct p2b3_range {
		boost::uint32_t label;
		boost::uint32_t start;
		boost::uint32_t end;
	};
	
	struct p2b4_range4cidr {
		boost::uint32_t start;
		boost::uint8_t cidr;
		boost::uint32_t label;
	};
	
	struct p2b4_range4 {
		boost::uint32_t start;
		boost::uint32_t end;
		boost::uint32_t label;
	};
	
	struct p2b4_range6cidr {
		boost::uint64_t start[2];
		boost::uint8_t cidr;
		boost::uint32_t label;
	};
	
	struct p2b4_range6 {
		boost::uint64_t start[2];
		boost::uint64_t end[2];
		boost::uint32_t label;
	};

#pragma pack(pop)

	struct string_set {
		boost::uint32_t insert(const std::wstring &str) {
			boost::uint32_t &labelidx = m_strings[str];

			if(!labelidx) {
				labelidx = (boost::uint32_t)m_strings.size();

				unicode::transcode<unicode::wchar_encoding, unicode::utf8>(
					str.begin(), str.end(),
					std::back_inserter(m_buf)
				);

				m_buf.push_back('\0');
			}

			return _byteswap_ulong(labelidx - 1);
		}

		stdext::hash_map<std::wstring, boost::uint32_t> m_strings;
		std::vector<char> m_buf;
	};

	static void save_p2b1(std::ostream &os, const list &l, bool utf8) {
		os.write(utf8 ? "\xFF\xFF\xFF\xFFP2B\x02" : "\xFF\xFF\xFF\xFFP2B\x01", 8);

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

			unsigned int rstart = _byteswap_ulong(iter->start);
			unsigned int rend = _byteswap_ulong(iter->end);
			
			os.write(&label.front(), (std::streamsize)label.size());
			os.write((const char*)&rstart, sizeof(rstart));
			os.write((const char*)&rend, sizeof(rend));
		}
	}

}

// saves a list in P2Bv1 ISO-9985-1 format.  only IPv4 addresses are saved.
void save_p2b1(std::ostream &os, const list &l) {
	detail::save_p2b1(os, l, false);
}

// saves a list in P2Bv2 UTF-8 format.  only IPv4 addresses are saved.
void save_p2b2(std::ostream &os, const list &l) {
	detail::save_p2b1(os, l, true);
}

// saves a list in P2Bv3 format.  only IPv4 addresses are saved.
void save_p2b3(std::ostream &os, const list &l) {
	detail::string_set labels;
	std::vector<detail::p2b3_range> ranges;

	for(list4::const_iterator iter = l.m_list4.begin(), end = l.m_list4.end(); iter != end; ++iter) {
		ranges.push_back(detail::p2b3_range());
		
		detail::p2b3_range &r = ranges.back();

		r.label = labels.insert(iter->label);
		r.start = _byteswap_ulong(iter->start);
		r.end = _byteswap_ulong(iter->end);
	}

	os.write("\xFF\xFF\xFF\xFFP2B\x03", 8);

	boost::uint32_t labelcount = _byteswap_ulong((boost::uint32_t)labels.m_strings.size());
	os.write((const char*)&labelcount, sizeof(labelcount));
	if(labels.m_buf.size()) {
		os.write(&labels.m_buf.front(), (std::streamsize)labels.m_buf.size());
	}

	boost::uint32_t rangecount = _byteswap_ulong((boost::uint32_t)ranges.size());
	os.write((const char*)&rangecount, sizeof(rangecount));
	if(ranges.size()) {
		os.write((const char*)&ranges.front(), (std::streamsize)ranges.size() * sizeof(detail::p2b3_range));
	}
}

// saves a list in P2Bv4 format.  both IPv4 and IPv6 addresses are saved.
void save_p2b4(std::ostream &os, const list &l, key *k) {
	detail::string_set strings;
	std::vector<detail::p2b4_range4cidr> ranges4cidr;
	std::vector<detail::p2b4_range4> ranges4;
	std::vector<detail::p2b4_range6cidr> ranges6cidr;
	std::vector<detail::p2b4_range6> ranges6;
	
	for(list4::const_iterator iter = l.m_list4.begin(), end = l.m_list4.end(); iter != end; ++iter) {
		unsigned int cidr = iter->cidr();
		boost::uint32_t labelid = strings.insert(iter->label);

		if(cidr) {
			ranges4cidr.push_back(detail::p2b4_range4cidr());

			detail::p2b4_range4cidr &r = ranges4cidr.back();

			r.start = _byteswap_ulong(iter->start);
			r.cidr = (boost::uint8_t)cidr;
			r.label = labelid;
		}
		else {
			ranges4.push_back(detail::p2b4_range4());

			detail::p2b4_range4 &r = ranges4.back();

			r.start = _byteswap_ulong(iter->start);
			r.end = _byteswap_ulong(iter->end);
			r.label = labelid;
		}
	}
	
	for(list6::const_iterator iter = l.m_list6.begin(), end = l.m_list6.end(); iter != end; ++iter) {
		unsigned int cidr = iter->cidr();
		boost::uint32_t labelid = strings.insert(iter->label);

		if(cidr) {
			ranges6cidr.push_back(detail::p2b4_range6cidr());

			detail::p2b4_range6cidr &r = ranges6cidr.back();

			r.start[0] = _byteswap_uint64(iter->start.i64[1]);
			r.start[1] = _byteswap_uint64(iter->start.i64[0]);
			r.cidr = (boost::uint8_t)cidr;
			r.label = labelid;
		}
		else {
			ranges6.push_back(detail::p2b4_range6());

			detail::p2b4_range6 &r = ranges6.back();

			r.start[0] = _byteswap_uint64(iter->start.i64[1]);
			r.start[1] = _byteswap_uint64(iter->start.i64[0]);
			r.end[0] = _byteswap_uint64(iter->end.i64[1]);
			r.end[1] = _byteswap_uint64(iter->end.i64[0]);
			r.label = labelid;
		}
	}

	crypto::sha512 sha;
	boost::iostreams::filtering_ostream out;

	out.push(crypto::sha512_filter(sha));
	out.push(os);

	out.write("\xFF\xFF\xFF\xFFP2B\x04", 8);

	// no metadata.
	out.write("\0", 1);

	// range descriptors.
	out.write("\x01label\0", 7);
	out.write("\0", 1);

	// strings.
	if(strings.m_buf.size()) {
		out.write(&strings.m_buf[0], (std::streamsize)strings.m_buf.size());
	}
	out.write("\0", 1);
	
	boost::uint32_t rangecount;

	// IPv4 diff ranges.  none for now.
	rangecount = 0;
	out.write((const char*)&rangecount, sizeof(rangecount));

	// IPv6 diff ranges.  none for now.
	out.write((const char*)&rangecount, sizeof(rangecount));

	// IPv4 CIDR ranges.
	rangecount = _byteswap_ulong((boost::uint32_t)ranges4cidr.size());
	out.write((const char*)&rangecount, sizeof(rangecount));
	if(ranges4cidr.size()) {
		out.write((const char*)&ranges4cidr[0], (std::streamsize)ranges4cidr.size() * sizeof(detail::p2b4_range4cidr));
	}

	// IPv4 ranges.
	rangecount = _byteswap_ulong((boost::uint32_t)ranges4.size());
	out.write((const char*)&rangecount, sizeof(rangecount));
	if(ranges4.size()) {
		out.write((const char*)&ranges4[0], (std::streamsize)ranges4.size() * sizeof(detail::p2b4_range4));
	}

	// IPv6 CIDR ranges.
	rangecount = _byteswap_ulong((boost::uint32_t)ranges6cidr.size());
	out.write((const char*)&rangecount, sizeof(rangecount));
	if(ranges6cidr.size()) {
		out.write((const char*)&ranges6cidr[0], (std::streamsize)ranges6cidr.size() * sizeof(detail::p2b4_range6cidr));
	}

	// IPv6 ranges.
	rangecount = _byteswap_ulong((boost::uint32_t)ranges6.size());
	out.write((const char*)&rangecount, sizeof(rangecount));
	if(ranges6.size()) { 
		out.write((const char*)&ranges6[0], (std::streamsize)ranges6.size() * sizeof(detail::p2b4_range6));
	}
	
	// done using stream filter, sync up.
	out.strict_sync();

	// hash.
	unsigned char hash[64];
	sha.finalize(hash);

	os.write((const char*)hash, 64);

	// signature.
	if(k) {
		std::vector<unsigned char> buf;
		k->sign(buf, hash, 64);

		boost::uint32_t size = _byteswap_ulong((boost::uint32_t)buf.size());

		os.write((const char*)&size, sizeof(size));
		os.write((const char*)&buf[0], (std::streamsize)buf.size());
	}
	else {
		// 0-length signature.
		boost::uint32_t size = 0;
		os.write((const char*)&size, sizeof(size));
	}
}

}
