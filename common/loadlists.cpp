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
#include "p2p/load.hpp"
#include "mapped_file.hpp"
#include "decompress.hpp"
#include "mutex.hpp"
#include "event.hpp"
#include "config.hpp"

enum file_type { unknown_file, gz_file, zip_file, sz_file };

static file_type classify(const char *mem, std::size_t len) {
	if(len >= 2) {
		if(!memcmp(mem, "\x1F\x8B", 2)) return gz_file;

		if(len >= 4) {
			if(!memcmp(mem, "PK\x03\x04", 4)) return zip_file;

			if(len >= 6 && !memcmp(mem, "7z\xBC\xAF\x27\x1C", 6)) {
				return sz_file;
			}
		}
	}

	return unknown_file;
}

void load_list(p2p::list &list, const char *mem, std::size_t len, p2p::key *k) {
	switch(classify(mem, len)) {
		case gz_file: {
			std::vector<char> buf;
			ungzip(buf, mem, len);

			p2p::load(list, &buf.front(), buf.size(), k);
		} break;
		case zip_file: {
			std::vector<std::vector<char> > bufs;
			unzip(bufs, mem, len);

			for(std::vector<std::vector<char> >::size_type i = 0; i < bufs.size(); ++i) {
				p2p::load(list, &bufs[i].front(), bufs[i].size(), k);
			}
		} break;
		case sz_file: {
			std::vector<std::vector<char> > bufs;
			un7zip(bufs, mem, len);

			for(std::vector<std::vector<char> >::size_type i = 0; i < bufs.size(); ++i) {
				p2p::load(list, &bufs[i].front(), bufs[i].size(), k);
			}
		} break;
		case unknown_file:
			p2p::load(list, mem, len, k);
			break;
	}
}

void load_list(p2p::list &list, const wchar_t *file, p2p::key *k) {
	mapped_file mf(file);

	if(mf.size()) {
		load_list(list, mf.begin(), mf.size(), k);
	}
}

void load_list(p2p::list &list, const pg3_list &l) {
	path p = l.real_path();
	load_list(list, p.c_str());
}
