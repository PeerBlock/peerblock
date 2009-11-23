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
#include <iostream>
#include "decompress.hpp"

void ungzip(std::vector<char> &buf, const char *mem, std::size_t len) {
	// skip gz header.

	if(len < 10) throw std::runtime_error("invalid gzip file");

	mem += 2; // magic number.
	mem += 1; // method.
	unsigned int flags = *(boost::uint8_t*)mem++;
	mem += 6; // xflags and OS code.

	len -= 10;

	// skip extra field.
	if(flags & 0x04) {
		if(len < 2) throw std::runtime_error("invalid gzip file");

		unsigned int flen = _byteswap_ushort(*(boost::uint16_t*)mem) + 2;

		if(len < flen) throw std::runtime_error("invalid gzip file");

		mem += flen;
		len -= flen;
	}

	// skip original name.
	if(flags & 0x08) {
		const char *cend = std::find(mem, mem + len, '\0');

		len -= (std::size_t)(cend - mem);
		mem = cend + 1;

		if(len-- <= 1) throw std::runtime_error("invalid gzip file");
	}

	// skip comment.
	if(flags & 0x10) {
		const char *cend = std::find(mem, mem + len, '\0');

		len -= (std::size_t)(cend - mem);
		mem = cend + 1;

		if(len-- <= 1) throw std::runtime_error("invalid gzip file");
	}

	// skip crc.
	if(flags & 0x02) {
		if(len < 2) throw std::runtime_error("invalid gzip file");

		mem += 2;
		len -= 2;
	}

	// do the ungzipping.

	z_stream zs = {0};

	zs.next_in = (Bytef*)mem;
	zs.avail_in = (uInt)len;

	len = std::min(std::max(len, (std::size_t)1024), (std::size_t)16384);

	buf.resize(len);

	zs.next_out = (Bytef*)&buf.front();
	zs.avail_out = (uInt)buf.size();

	int ret = inflateInit2(&zs, -MAX_WBITS);
	if(ret != Z_OK) {
		throw std::runtime_error("inflateInit failed");
	}

	do {
		ret = inflate(&zs, Z_NO_FLUSH);
		if(ret == Z_OK) {
			if(!zs.avail_out) {
				buf.resize(buf.size() + len);

				zs.next_out = (Bytef*)&buf[zs.total_out];
				zs.avail_out = (uInt)len;
			}
		}
		else if(ret == Z_BUF_ERROR) {
			buf.resize(buf.size() + len);

			zs.next_out = (Bytef*)&buf[zs.total_out];
			zs.avail_out = (uInt)buf.size() - zs.total_out;
		}
		else {
			inflateEnd(&zs);
			if(ret != Z_STREAM_END) {
				std::cerr << ret << std::endl;
				throw std::runtime_error("inflate failed");
			}
		}
	} while(ret != Z_STREAM_END);

	buf.resize(zs.total_out);
}
