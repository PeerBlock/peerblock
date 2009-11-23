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

#include <cstdlib>
#include <limits>

#include <boost/cstdint.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
typedef sockaddr_in6 SOCKADDR_IN6;
#endif

#include "../platform.hpp"

namespace p2p {

// an IPv4 address is represented by a 32-bit unsigned int.
typedef boost::uint32_t ip4;

// an IPv6 address stores several data types for easy manipulation.
struct ip6 {
	union {
		// network byte order: [127 ... 0]
		boost::uint64_t i64[2];
		boost::uint32_t i32[4];
		boost::uint16_t i16[8];
		boost::uint8_t i8[16];
	};

	// increment the value of the IP.
	ip6& operator++() {
		i64[0] += (++i64[1] == 0);
		return *this;
	}

	// post-increments.
	ip6 operator++(int) {
		ip6 ret = *this;

		++*this;
		return ret;
	}

	// decrement the value of the IP.
	ip6& operator--() {
		i64[0] -= (--i64[1] == std::numeric_limits<boost::uint64_t>::max());
		return *this;
	}

	// post-decrement.
	ip6 operator--(int) {
		ip6 ret = *this;

		--*this;
		return ret;
	}

	// returns a bitwise AND of two IPs.
	ip6 operator&(const ip6 &ip) const {
		ip6 ret = *this;

		ret.i64[0] &= ip.i64[0];
		ret.i64[1] &= ip.i64[1];

		return ret;
	}

	// returns a bitwise OR of two IPs.
	ip6 operator|(const ip6 &ip) const {
		ip6 ret = *this;

		ret.i64[0] |= ip.i64[0];
		ret.i64[1] |= ip.i64[1];

		return ret;
	}

	// returns a bitwise NOT of the IP.
	ip6 operator~() const {
		ip6 ret = *this;

		ret.i64[0] = ~ret.i64[0];
		ret.i64[1] = ~ret.i64[1];

		return ret;
	}

	// performs a logical NOT, testing if the IP is ::0.
	bool operator!() const { return !i64[0] && !i64[1]; }

	// basic logical comparisons.
	bool operator==(const ip6 &b) const { return i64[0] == b.i64[0] && i64[1] == b.i64[1]; }
	bool operator!=(const ip6 &b) const { return i64[0] != b.i64[0] || i64[1] != b.i64[1]; }

	bool operator<(const ip6 &b) const { return compare(*this, b) < 0; }
	bool operator>(const ip6 &b) const { return compare(*this, b) > 0; }

	bool operator<=(const ip6 &b) const { return compare(*this, b) <= 0; }
	bool operator>=(const ip6 &b) const { return compare(*this, b) >= 0; }

	// -1 if a < b, 0 if a == b, 1 if a > b.
	static int compare(const ip6 &a, const ip6 &b) {
#ifdef _M_IX86
		if(a.i32[0] < b.i32[0]) return -1;
		if(a.i32[0] > b.i32[0]) return 1;
		
		if(a.i32[1] < b.i32[1]) return -1;
		if(a.i32[1] > b.i32[1]) return 1;
		
		if(a.i32[2] < b.i32[2]) return -1;
		if(a.i32[2] > b.i32[2]) return 1;
		
		if(a.i32[3] < b.i32[3]) return -1;
		if(a.i32[3] > b.i32[3]) return 1;
#else
		if(a.i64[0] < b.i64[0]) return -1;
		if(a.i64[0] > b.i64[0]) return 1;
		
		if(a.i64[1] < b.i64[1]) return -1;
		if(a.i64[1] > b.i64[1]) return 1;
#endif
		
		return 0;
	}
};

template<typename Ip>
struct ip_traits {
	typedef Ip ip_type;

	// gives in_addr for ip4, and in6_addr for ip6.
	typedef typename boost::mpl::if_<
		boost::is_same<ip_type, ip4>,
		in_addr,
		in6_addr
	>::type inaddr_type;

	// gives sockaddr_in for ip4, and sockaddr_in6 for ip6.
	typedef typename boost::mpl::if_<
		boost::is_same<ip_type, ip4>,
		sockaddr_in,
		sockaddr_in6
	>::type sockaddr_type;

	// returns an ip in host byte order from an address in network byte order.
	static ip_type from_native(const inaddr_type &addr);
	static ip_type from_native(const sockaddr_type &addr);
};

// exports for above from_native functions, for ip4 and ip6.

template<>
COMMON_EXPORT ip4 ip_traits<ip4>::from_native(const inaddr_type &addr);

template<>
COMMON_EXPORT ip4 ip_traits<ip4>::from_native(const sockaddr_type &addr);

template<>
COMMON_EXPORT ip6 ip_traits<ip6>::from_native(const inaddr_type &addr);

template<>
COMMON_EXPORT ip6 ip_traits<ip6>::from_native(const sockaddr_type &addr);

}
