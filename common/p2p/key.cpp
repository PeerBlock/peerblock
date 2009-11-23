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
#include "../mapped_file.hpp"
#include "key.hpp"

namespace p2p {

namespace detail {

	static const char* memstr(const char *buf, std::size_t buflen, const char *str, std::size_t len) {
		while(buflen >= len) {
			if(!memcmp(buf, str, len)) return buf;

			++buf;
			--buflen;
		}

		return NULL;
	}

}


key::key() {}
key::~key() {}

void key::close() {
	m_key.close();
}

void key::generate() {
	m_key.generate(crypto::ecc_key::key_521);
}

void key::load(const char *begin, const char *end, const wchar_t *password) {
	const char *kbegin;
	bool ispublic;

	kbegin = detail::memstr(begin, end - begin, "===== BEGIN PeerGuardian 3 Public Key =====", 43);
	if(kbegin) {
		kbegin += 43;

		end = detail::memstr(kbegin, end - begin, "===== END PeerGuardian 3 Public Key =====", 41);
		if(!end) throw std::runtime_error("expected end of key");

		ispublic = true;
	}
	else {
		kbegin = detail::memstr(begin, end - begin, "===== BEGIN PeerGuardian 3 Private Key =====", 44);
		if(!kbegin) throw std::runtime_error("invalid key");

		kbegin += 44;
		
		end = detail::memstr(kbegin, end - begin, "===== END PeerGuardian 3 Private Key =====", 42);
		if(!end) throw std::runtime_error("expected end of key");

		ispublic = false;
	}

	std::vector<unsigned char> buf;
	crypto::base64_decode(buf, (const unsigned char*)kbegin, (unsigned long)(end - kbegin));

	if(!ispublic) {
		if(!password) password = L"";

		std::vector<unsigned char> upass;
		unicode::transcode<unicode::wchar_encoding, unicode::utf8>(std::wstring(password), upass);
		upass.push_back(0);

		unsigned char hash[48];
		unsigned long hashlen = 48;

		if(buf.size() < salt_size) {
			throw std::runtime_error("unexpected end of key");
		}

		crypto::hash_password(&upass[0], (unsigned long)upass.size(), &buf[0], salt_size, 100, hash, hashlen);

		crypto::aes256_stream aes;

		aes.start(hash, hash + 16);

		std::vector<unsigned char> decoded(buf.size() - salt_size);
		aes.decrypt(&buf[salt_size], &decoded[0], (unsigned long)decoded.size());

		buf.swap(decoded);
	}
	
	m_key.read(&buf[0], (unsigned long)buf.size());
}

void key::load(const wchar_t *file, const wchar_t *password) {
	mapped_file fp(file);
	if(!fp.size()) throw std::runtime_error("unexpected end of key");

	load(fp.begin(), fp.end(), password);
}

void key::save_public(std::ostream &os) {
	std::vector<unsigned char> keybuf;
	m_key.write(keybuf, crypto::ecc_key::public_key);

	std::vector<unsigned char> base64;
	crypto::base64_encode(base64, &keybuf[0], (unsigned long)keybuf.size());

	os << "===== BEGIN PeerGuardian 3 Public Key =====" << std::endl;

	for(std::vector<unsigned char>::size_type i = 0; i < base64.size(); i += 60) {
		std::vector<unsigned char>::size_type left = base64.size() - i;

		os.write((const char*)&base64[i], std::streamsize(left <= 60 ? left : 60));
		os << std::endl;
	}

	os << "===== END PeerGuardian 3 Public Key =====" << std::endl;
}

void key::save_private(std::ostream &os, const wchar_t *password) {
	std::vector<unsigned char> keybuf;
	m_key.write(keybuf, crypto::ecc_key::private_key);

	std::vector<unsigned char> buf(salt_size + keybuf.size());
	rng_get_bytes(&buf[0], salt_size, NULL);

	std::vector<unsigned char> upass;
	unicode::transcode<unicode::wchar_encoding, unicode::utf8>(std::wstring(password), upass);
	upass.push_back(0);

	unsigned char hash[48];
	unsigned long hashlen = 48;

	crypto::hash_password(&upass[0], (unsigned long)upass.size(), &buf[0], salt_size, 100, hash, hashlen);

	{
		crypto::aes256_stream aes;

		aes.start(hash, hash + 16);
		aes.encrypt(&keybuf[0], &buf[salt_size], (unsigned long)keybuf.size());
	}

	std::vector<unsigned char> base64;
	crypto::base64_encode(base64, &buf[0], (unsigned long)buf.size());

	os << "===== BEGIN PeerGuardian 3 Private Key =====" << std::endl;

	for(std::vector<unsigned char>::size_type i = 0; i < base64.size(); i += 60) {
		std::vector<unsigned char>::size_type left = base64.size() - i;

		os.write((const char*)&base64[i], std::streamsize(left <= 60 ? left : 60));
		os << std::endl;
	}

	os << "===== END PeerGuardian 3 Private Key =====" << std::endl;
}

void key::sign(std::vector<unsigned char> &outbuf, const unsigned char *hash, unsigned long len) {
	m_key.sign(outbuf, hash, len);
}

bool key::verify(const unsigned char *sig,  unsigned long siglen, const unsigned char *hash, unsigned long hashlen) {
	return m_key.verify(sig, siglen, hash, hashlen);
}

key::key_type key::type() const {
	return m_key.type();
}

}
