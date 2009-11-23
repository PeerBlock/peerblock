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

#pragma once

#include <string>
#include "../platform.hpp"
#include "ip.hpp"

namespace p2p {

/*
	compact_range stores a range without a label.
*/
template<typename Ip>
struct compact_range {
	typedef Ip ip_type;
	typedef typename ip_traits<ip_type>::inaddr_type inaddr_type;
	typedef typename ip_traits<ip_type>::sockaddr_type sockaddr_type;

	ip_type start, end;

	// constructs a range.  the values of start and end are undefined.
	compact_range() {}

	// constructs a range, setting start and end to an ip.
	compact_range(const ip_type &ip) : start(ip),end(ip) {}

	// constructs a range.
	compact_range(const ip_type &start, const ip_type &end) : start(start),end(end) {}

	// constructs a range using socket address types.
	compact_range(const inaddr_type &ip) { end = start = ip_traits<ip_type>::from_native(ip); }
	compact_range(const inaddr_type &start, const inaddr_type &end) {
		this->start = ip_traits<ip_type>::from_native(start);
		this->end = ip_traits<ip_type>::from_native(end);
	}

	// constructs a range using socket address types.
	compact_range(const sockaddr_type &ip) { end = start = ip_traits<ip_type>::from_native(ip); }
	compact_range(const sockaddr_type &start, const sockaddr_type &end) {
		this->start = ip_traits<ip_type>::from_native(start);
		this->end = ip_traits<ip_type>::from_native(end);
	}

	// returns true if start is less than an IP, or a range's start.
	bool operator<(const ip_type &ip) const { return start < ip; }
	bool operator<(const compact_range<ip_type> &r) const { return start < r.start; }

	// returns true if this range fully contains an IP or another range.
	bool contains(const ip_type &ip) const { return start <= ip && ip <= end; }
	bool contains(const compact_range<ip_type> &r) const { return start <= r.start && r.end <= end; }

	// returns true if this range intersects with another range.
	bool intersects(const compact_range<ip_type> &r) const { return contains(r.start) || contains(r.end); }

	// returns true if a is adjacent to b.
	static bool adjacent(const compact_range<ip_type> &a, const compact_range<ip_type> &b) {
		ip_type bstart = b.start; --bstart;
		return a.end == bstart;
	}

	// returns the CIDR bit count for the range.  can be 0...32 for IPv4 or 0...128 for IPv6.
	unsigned int cidr() const {
		for(unsigned int i = 0; i < cidr_bits; ++i) {
			const ip_type &mask = cidr_masks[i];
			const ip_type invmask = ~mask;
			
			if((start & invmask) == (end & invmask) && !(start & mask) && (end & mask) == mask) {
				return i + 1;
			}
		}

		return 0;
	}

	// creates a range from base IP and a CIDR bit count.
	static compact_range<Ip> from_cidr(const ip_type &ip, unsigned int bits) {
		if(bits < 1) bits = 1;
		if(bits > cidr_bits) bits = cidr_bits;

		const ip_type &mask = cidr_masks[bits - 1];

		return compact_range<Ip>(ip & ~mask, ip | mask);
	}

private:
	// specialized for ip4 and ip6 in cidr.cpp
	static const unsigned int cidr_bits;
	static const ip_type cidr_masks[];
};

typedef compact_range<ip4> compact_range4;
typedef compact_range<ip6> compact_range6;

/*
	static_range stores a range with a pooled label.
*/
template<typename Ip>
struct static_range : public compact_range<Ip> {
	const wchar_t *label;
};

typedef static_range<ip4> static_range4;
typedef static_range<ip6> static_range6;

/*
	static_range stores a range with label.
*/
template<typename Ip>
struct range : public compact_range<Ip> {
	std::wstring label;

	// constructs a range.  the values of start and end are undefined.
	range() {}

	// constructs a range based off a compact range.
	range(const compact_range<Ip> &r) : compact_range<Ip>(r) {}

	// constructs a range, setting start and end to an ip.
	range(const ip_type &ip) : compact_range<Ip>(ip) {}
	range(const ip_type &start, const ip_type &end) : compact_range<Ip>(start, end) {}

	// constructs a range using socket address types.
	range(const inaddr_type &ip) : compact_range<Ip>(ip) {}
	range(const inaddr_type &start, const inaddr_type &end) : compact_range<Ip>(start, end) {}

	// constructs a range using socket address types.
	range(const sockaddr_type &ip) : compact_range<Ip>(ip) {}
	range(const sockaddr_type &start, const sockaddr_type &end) : compact_range<Ip>(start, end) {}

	// constructs a range with a label.  the values of start and end are undefined.
	range(const std::wstring &label) : label(label) {}

	// constructs a range with a label, based off a compact range.
	range(const std::wstring &label, const compact_range<Ip> &r) : compact_range<Ip>(r),label(label) {}
	
	// constructs a range with a label, setting start and end to an ip.
	range(const std::wstring &label, const ip_type &ip) : compact_range<Ip>(ip),label(label) {}
	range(const std::wstring &label, const ip_type &start, const ip_type &end) : compact_range<Ip>(start, end),label(label) {}
	
	// constructs a range using a label and socket address types.
	range(const std::wstring &label, const inaddr_type &ip) : compact_range<Ip>(ip),label(label) {}
	range(const std::wstring &label, const inaddr_type &start, const inaddr_type &end) : compact_range<Ip>(start, end),label(label) {}
	
	// constructs a range using a label and socket address types.
	range(const std::wstring &label, const sockaddr_type &ip) : compact_range<Ip>(ip),label(label) {}
	range(const std::wstring &label, const sockaddr_type &start, const sockaddr_type &end) : compact_range<Ip>(start, end),label(label) {}

	range<Ip>& operator=(const compact_range<Ip> &r) {
		*reinterpret_cast<compact_range<Ip>*>(this) = r;
		return *this;
	}
};

typedef range<ip4> range4;
typedef range<ip6> range6;

}
