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
#include <stdexcept>
#include <boost/iostreams/filtering_stream.hpp>
#include <tomcrypt.h>
#include "platform.hpp"

namespace crypto {

COMMON_EXPORT void init();

void base64_encode(std::vector<unsigned char> &buf, const unsigned char *in, unsigned long len);
void base64_decode(std::vector<unsigned char> &buf, const unsigned char *in, unsigned long len);

void hash_password(const unsigned char *pass, unsigned long passlen, const unsigned char *salt, unsigned long saltlen, int iters, unsigned char *out, unsigned long outlen);

class crypto_error : public std::exception {
public:
	crypto_error(const char *func, int code) : m_func(func), m_code(code) {}

	COMMON_EXPORT const char* what() const;

	const char* func() const { return m_func; }
	int code() const { return m_code; }

private:
	const char *m_func;
	int m_code;
};

class sha512 {
public:
	sha512();
	void process(const unsigned char *in, unsigned long len);
	void finalize(unsigned char (&hash)[64]);

private:
	// noncopyable
	sha512(const sha512&);
	void operator=(const sha512&);

	hash_state m_state;
};

class sha512_filter : public boost::iostreams::multichar_dual_use_filter {
public:
	sha512_filter(sha512 &sha) : m_sha(sha) {}

	template<typename SinkType>
	std::streamsize write(SinkType& sink, const char *s, std::streamsize n) {
		n = boost::iostreams::write(sink, s, n);
		m_sha.process((const unsigned char*)s, (unsigned long)n);
		return n;
	}
	
	template<typename SourceType>
	std::streamsize read(SourceType& src, char *s, std::streamsize n) {
		n = boost::iostreams::read(src, s, n);
		m_sha.process((const unsigned char*)s, (unsigned long)n);
		return n;
	}

private:
	sha512 &m_sha;
};

class aes256_stream {
public:
	aes256_stream() : m_init(false) {}
	~aes256_stream() { close(); }

	void start(unsigned char *iv, unsigned char *key);
	void reset(unsigned char *iv);
	void close();

	void encrypt(const unsigned char *in, unsigned char *out, unsigned long len);
	void decrypt(const unsigned char *in, unsigned char *out, unsigned long len);

private:
	// noncopyable
	aes256_stream(const aes256_stream&);
	void operator=(const aes256_stream&);

	symmetric_CTR m_ctr;
	bool m_init;
};

class ecc_key {
public:
	enum key_size {
		key_112 = 12,
		key_128 = 16,
		key_160 = 20,
		key_192 = 24,
		key_224 = 28,
		key_256 = 32,
		key_384 = 48,
		key_521 = 65
	};

	enum key_type {
		public_key = PK_PUBLIC,
		private_key = PK_PRIVATE,
		empty_key
	};

	ecc_key() : m_init(false) {}
	~ecc_key() { close(); }

	void generate(key_size size);

	void read(const unsigned char *in, unsigned long inlen);
	void write(std::vector<unsigned char> &out, key_type type);

	void close();

	void sign(std::vector<unsigned char> &out, const unsigned char *in, unsigned long inlen);
	bool verify(const unsigned char *sig,  unsigned long siglen, const unsigned char *hash, unsigned long hashlen);

	key_type type() const;

private:
	// noncopyable
	ecc_key(const ecc_key&);
	void operator=(const ecc_key&);

	::ecc_key m_key;
	bool m_init;
};

}
