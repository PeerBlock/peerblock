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

#ifndef __P2P_IP_HPP__
#define __P2P_IP_HPP__

#include <ostream>

namespace p2p {
	struct ip {
		union {
			unsigned int ipl;
			unsigned char ipb[4];
		};

		ip() : ipl(0) {}
		ip(unsigned int ip) : ipl(ip) {}
		bool operator<(const ip &ip) const { return this->ipl<ip.ipl; }
		bool operator>(const ip &ip) const { return this->ipl>ip.ipl; }
		bool operator<=(const ip &ip) const { return this->ipl<=ip.ipl; }
		bool operator>=(const ip &ip) const { return this->ipl>=ip.ipl; }
		bool operator==(const ip &ip) const { return this->ipl==ip.ipl; }
		bool operator!=(const ip &ip) const { return this->ipl!=ip.ipl; }

		ip operator+(unsigned int i) const { return this->ipl+i; }
		ip operator-(unsigned int i) const { return this->ipl-i; }
	};
}

#endif
