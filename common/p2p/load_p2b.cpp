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
	// loads a P2Bv1 or P2Bv2 list.
	static void load_p2b1(list &l, const char *iter, const char *end, bool utf8) {
		while(iter < end) {
			range4 &r = l.m_list4.insert();

			const char *labelend = std::find(iter, end, '\0');
			if(labelend == end) throw std::runtime_error("incomplete P2B range");

			if(utf8) {
				unicode::transcode<unicode::utf8, unicode::wchar_encoding>(iter, labelend, std::inserter(r.label, r.label.end()));
			}
			else {
				r.label.assign(iter, labelend);
			}

			iter = labelend + 1;

			if((iter + 8) > end) {
				throw std::runtime_error("incomplete P2B range");
			}

			r.start = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
			r.end = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
		}
	}
	
	// loads a P2Bv3 list.
	static void load_p2b3(list &l, const char *iter, const char *end) {
		if((iter + 4) > end) throw std::runtime_error("expected label count");
		boost::uint32_t labelcount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		std::vector<std::wstring> labels(labelcount);

		for(boost::uint32_t i = 0; i < labelcount; ++i) {
			const char *labelend = std::find(iter, end, '\0');
			if(labelend == end) throw std::runtime_error("incomplete label");

			unicode::transcode<unicode::utf8, unicode::wchar_encoding>(
				iter, labelend,
				std::inserter(labels[i], labels[i].end())
			);

			iter = labelend + 1;
		}
		
		if((iter + 4) > end) throw std::runtime_error("expected range count");
		boost::uint32_t rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 12) > end) throw std::runtime_error("expected range");

			boost::uint32_t label = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
			if(label >= labels.size()) throw std::runtime_error("label out of bounds");

			range4 &r = l.m_list4.insert();

			r.label = labels[label];
			r.start = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
			r.end = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
		}
	}

	// reads P2Bv4 range fields.
	static void read_fields(const char* &iter, const char *end, const std::vector<unsigned int> &fields, const std::vector<std::wstring> &strings, unsigned int labelidx, std::wstring &label) {
		for(unsigned int i = 0; i < fields.size(); ++i) {
			union {
				boost::uint32_t stridx;
				boost::uint32_t i32;
				boost::uint64_t i64;
				boost::uint64_t i128[2];
			};

			switch(fields[i]) {
				case 1:
					if((iter + 4) > end) throw std::runtime_error("expected string index");

					stridx = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
					if(stridx >= strings.size()) {
						throw std::runtime_error("string out of bounds");
					}
					break;
				case 2:
					iter += 4; // i32 not processed right now.
					if(iter > end) throw std::runtime_error("expected uint32");
					break;
				case 3:
					iter += 8; // i64 not processed right now.
					if(iter > end) throw std::runtime_error("expected uint64");
					break;
				case 4:
					iter += 16; // i128 not processed right now.
					if(iter > end) throw std::runtime_error("expected uint128");
					break;
			}

			if(i == labelidx) {
				label = strings[stridx];
			}
		}
	}
	
	// loads a P2Bv4 list.
	static void load_p2b4(list &l, const char *iter, const char *end, key *k) {
		const char *start = iter;

		iter += 8;

		// skip metadata.
		while(iter < end && iter[0]) {
			const char *metaend = std::find(iter, end, '\0');
			if(metaend == end) throw std::runtime_error("incomplete metadata key");

			metaend = std::find(metaend + 1, end, '\0');
			if(metaend == end) throw std::runtime_error("incomplete metadata value");

			iter = metaend + 1;
		}

		// read range descriptors.

		if(++iter >= end) throw std::runtime_error("expected range descriptors");

		unsigned int labelidx = std::numeric_limits<unsigned int>::max();
		std::vector<unsigned int> fields;

		while(iter < end && iter[0]) {
			unsigned int type = *(boost::uint8_t*)iter++;

			const char *descend = std::find(iter, end, '\0');
			if(descend == end) throw std::runtime_error("incomplete descriptor label");

			if(type == 1 && (descend - iter) == 5 && !memcmp(iter, "label", 5)) {
				labelidx = (unsigned int)fields.size();
			}

			fields.push_back(type);
			iter = descend + 1;
		}

		// read strings.
		
		if(++iter >= end) throw std::runtime_error("expected strings");

		std::vector<std::wstring> strings;

		while(iter < end && iter[0]) {
			const char *strend = std::find(iter, end, '\0');
			if(strend == end) throw std::runtime_error("incomplete string");

			strings.push_back(std::wstring());

			std::wstring &str = strings.back();

			unicode::transcode<unicode::utf8, unicode::wchar_encoding>(
				iter, strend,
				std::inserter(str, str.end())
			);

			iter = strend + 1;
		}

		// read IPv4 diff IPs.

		if((++iter + 4) > end) throw std::runtime_error("expected ip count");
		boost::uint32_t rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 4) > end) throw std::runtime_error("expected IPv4 IP");

			boost::uint32_t ip = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

			//TODO: do something with IP.
		}

		// read IPv6 diff IPs.
		
		if((iter + 4) > end) throw std::runtime_error("expected range count");
		rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 16) > end) throw std::runtime_error("expected IPv6 IP");

			ip6 ip;
			ip.i64[1] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;
			ip.i64[0] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;

			//TODO: do something with IP.
		}

		// read IPv4 CIDR ranges.
		
		if((iter + 4) > end) throw std::runtime_error("expected range count");
		rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 5) > end) throw std::runtime_error("expected IPv4 CIDR range");

			boost::uint32_t base = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
			unsigned int cidrbits = *(boost::uint8_t*)iter++;

			range4 &r = l.m_list4.insert();

			r = range4::from_cidr(base, cidrbits);
			read_fields(iter, end, fields, strings, labelidx, r.label);
		}

		// read IPv4 ranges.
		
		if((iter + 4) > end) throw std::runtime_error("expected range count");
		rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 8) > end) throw std::runtime_error("expected IPv4 range");

			range4 &r = l.m_list4.insert();
			
			r.start = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
			r.end = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;
			
			read_fields(iter, end, fields, strings, labelidx, r.label);
		}

		// read IPv6 CIDR ranges.
		
		if((iter + 4) > end) throw std::runtime_error("expected range count");
		rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 17) > end) throw std::runtime_error("expected IPv6 CIDR range");
			
			ip6 base;
			base.i64[1] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;
			base.i64[0] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;

			unsigned int cidrbits = *(boost::uint8_t*)iter++;

			range6 &r = l.m_list6.insert();

			r = range6::from_cidr(base, cidrbits);
			read_fields(iter, end, fields, strings, labelidx, r.label);
		}

		// read IPv6 ranges.
		
		if((iter + 4) > end) throw std::runtime_error("expected range count");
		rangecount = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		for(boost::uint32_t i = 0; i < rangecount; ++i) {
			if((iter + 32) > end) throw std::runtime_error("expected IPv6 range");

			range6 &r = l.m_list6.insert();

			r.start.i64[1] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;
			r.start.i64[0] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;

			r.end.i64[1] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;
			r.end.i64[0] = _byteswap_uint64(*(const boost::uint64_t*)iter); iter += 8;

			read_fields(iter, end, fields, strings, labelidx, r.label);
		}
		
		// verify SHA-512 hash.

		if(iter + 64 > end) throw std::runtime_error("expected hash");

		crypto::sha512 sha;
		sha.process((const unsigned char*)start, (unsigned long)(iter - start));

		unsigned char hash[64];
		sha.finalize(hash);

		if(memcmp(iter, hash, 64)) throw std::runtime_error("hash mismatch, corrupt list");
		iter += 64;

		// check signature, if available.

		if((iter + 4) > end) throw std::runtime_error("expected digital signature size");
		boost::uint32_t siglen = _byteswap_ulong(*(boost::uint32_t*)iter); iter += 4;

		if((iter + siglen) > end || (!siglen && k)) throw std::runtime_error("expected digital signature");

		if(k && !k->verify((const unsigned char*)iter, siglen, hash, 64)) {
			throw std::runtime_error("digital signature mismatch");
		}
	}
}

// loads a P2B list.  Supports versions 1, 2, 3, and 4.
void load_p2b(list &l, const char *mem, std::size_t len, key *k) {
	if(len < 8 || memcmp(mem, "\xFF\xFF\xFF\xFFP2B", 7)) {
		throw std::runtime_error("invalid P2B");
	}

	if(k && mem[7] != 4) {
		throw std::runtime_error("digital signatures unsupported for P2B version");
	}

	switch((int)mem[7]) {
		case 1: detail::load_p2b1(l, mem + 8, mem + len, false); break;
		case 2: detail::load_p2b1(l, mem + 8, mem + len, true); break;
		case 3: detail::load_p2b3(l, mem + 8, mem + len); break;
		case 4: detail::load_p2b4(l, mem, mem + len, k); break;
		default:
			throw std::runtime_error("unknown P2B version");
	}
}

}
