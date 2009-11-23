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

#include <vector>
#include <ostream>
#include "../crypto.hpp"
#include "../platform.hpp"

namespace p2p {

class key {
public:
	typedef crypto::ecc_key::key_type key_type;

	COMMON_EXPORT key();
	COMMON_EXPORT ~key();

	COMMON_EXPORT void generate();

	COMMON_EXPORT void load(const char *begin, const char *end, const wchar_t *password = 0);
	COMMON_EXPORT void load(const wchar_t *file, const wchar_t *password = 0);

	COMMON_EXPORT void save_public(std::ostream &os);
	COMMON_EXPORT void save_private(std::ostream &os, const wchar_t *password);

	COMMON_EXPORT void close();
	
	COMMON_EXPORT void sign(std::vector<unsigned char> &outbuf, const unsigned char *hash, unsigned long len);
	COMMON_EXPORT bool verify(const unsigned char *sig, unsigned long siglen, const unsigned char *hash, unsigned long hashlen);

	COMMON_EXPORT key_type type() const;

private:
	// noncopyable
	key(const key&);
	void operator=(const key&);

	static const unsigned long salt_size = 32;

	crypto::ecc_key m_key;
};

}
