#include "stdafx.h"
#include "ip.hpp"

namespace p2p {

/*
	these all read a socket address and return an ip4 or ip6 in host byte order.
*/

template<>
ip4 ip_traits<ip4>::from_native(const inaddr_type &addr) {
	return _byteswap_ulong(addr.s_addr);
}

template<>
ip4 ip_traits<ip4>::from_native(const sockaddr_type &addr) {
	return from_native(addr.sin_addr);
}

template<>
ip6 ip_traits<ip6>::from_native(const inaddr_type &addr) {
	ip6 ret;

	const boost::uint64_t *i64 = (const boost::uint64_t*)&addr;

	ret.i64[0] = _byteswap_uint64(i64[1]);
	ret.i64[1] = _byteswap_uint64(i64[0]);

	return ret;
}

template<>
ip6 ip_traits<ip6>::from_native(const sockaddr_type &addr) {
	return from_native(addr.sin6_addr);
}

}
