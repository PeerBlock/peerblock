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
#include "localips.hpp"
#include "win32_error.hpp"

static IP_ADAPTER_INFO* GetAdaptersInfo() {
	ULONG len = 0;

	ULONG err = GetAdaptersInfo(NULL, &len);
	if(err != ERROR_BUFFER_OVERFLOW) throw win32_error("GetAdaptersInfo", err);

	IP_ADAPTER_INFO *ai = NULL;

	do {
		if(ai) free(ai);

		ai = (IP_ADAPTER_INFO*)malloc(len);
		if(!ai) throw std::bad_alloc("unable to allocate memory for adapter info");

		err = GetAdaptersInfo(ai, &len);
	} while(err == ERROR_BUFFER_OVERFLOW);

	if(err != ERROR_SUCCESS) {
		throw win32_error("GetAdaptersInfo", err);
	}

	return ai;
}

static IP_ADAPTER_ADDRESSES* GetAdaptersAddresses(ULONG family, ULONG dwflags) {
	ULONG len = 0;

	ULONG err = GetAdaptersAddresses(family, dwflags, NULL, NULL, &len);
	if(err != ERROR_BUFFER_OVERFLOW) throw win32_error("GetAdaptersAddresses", err);

	IP_ADAPTER_ADDRESSES *aa = NULL;

	do {
		if(aa) free(aa);

		aa = (IP_ADAPTER_ADDRESSES*)malloc(len);
		if(!aa) throw std::bad_alloc("unable to allocate memory for local addresses");

		err = GetAdaptersAddresses(family, dwflags, NULL, aa, &len);
	} while(err == ERROR_BUFFER_OVERFLOW);

	if(err != ERROR_SUCCESS) {
		throw win32_error("GetAdaptersAddresses", err);
	}

	return aa;
}

static p2p::ip4 parseip(const char *str) {
	if(str[0]) {
		unsigned int a, b, c, d;

		if(sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
			if(a > 0 && a <= 255 && b <= 255 && c <= 255 && d <= 255) {
				return (a << 24) | (b << 16) | (c << 8) | d;
			}
		}
	}

	return 0;
}

static void insertallowed(p2p::list &list, sockaddr *addr) {
	if(addr->sa_family == AF_INET) {
		list.insert(p2p::range4(L"local address", *(sockaddr_in*)addr));
	}
	else if(addr->sa_family == AF_INET6) {
		list.insert(p2p::range6(L"local address", *(sockaddr_in6*)addr));
	}
}

void localip::getips(p2p::list &ips, localip_flags flags) {
#if _WIN32_WINNT >= 0x0600
	DWORD dwflags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST;
#else
	// on XP, use GetAdaptersInfo for gateways.

	IP_ADAPTER_INFO *ai = GetAdaptersInfo();
	
	for(IP_ADAPTER_INFO *iter = ai; iter; iter = iter->Next) {
		if(flags & localip::gateway) {
			for(IP_ADDR_STRING *ipiter = &iter->GatewayList; ipiter; ipiter = ipiter->Next) {
				p2p::ip4 ip = parseip(ipiter->IpAddress.String);
				ips.insert(p2p::range4(L"local address", ip));
			}
		}
		
		if(flags & localip::dhcp && iter->DhcpEnabled) {
			for(IP_ADDR_STRING *ipiter = &iter->DhcpServer; ipiter; ipiter = ipiter->Next) {
				p2p::ip4 ip = parseip(ipiter->IpAddress.String);
				ips.insert(p2p::range4(L"local address", ip));
			}
		}
	}

	free(ai);

	DWORD dwflags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST;
#endif

	IP_ADAPTER_ADDRESSES *aa = GetAdaptersAddresses(AF_UNSPEC, dwflags);
	
	for(const IP_ADAPTER_ADDRESSES *iter = aa; iter; iter = iter->Next) {
		if(flags & localip::adapter) {
			for(const IP_ADAPTER_UNICAST_ADDRESS *ipiter = iter->FirstUnicastAddress; ipiter; ipiter = ipiter->Next) {
				insertallowed(ips, ipiter->Address.lpSockaddr);
			}
		}

		if(flags & localip::dns) {
			for(const IP_ADAPTER_DNS_SERVER_ADDRESS *ipiter = iter->FirstDnsServerAddress; ipiter; ipiter = ipiter->Next) {
				insertallowed(ips, ipiter->Address.lpSockaddr);
			}
		}

#if _WIN32_WINNT >= 0x0600
		if(flags & localip::gateway) {
			for(const IP_ADAPTER_GATEWAY_ADDRESS *ipiter = iter->FirstGatewayAddress; ipiter; ipiter = ipiter->Next) {
				insertallowed(ips, ipiter->Address.lpSockaddr);
			}
		}
#endif
	}

	free(aa);
}
