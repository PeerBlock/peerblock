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
#include "crypto.hpp"

static int g_crypto_rng = -1;
static int g_aes_cipher = -1;
static int g_sha512_hash = -1;

void crypto::init() {
	ltc_mp = ltm_desc;

	int ret = register_prng(&sprng_desc);
	if(ret == -1) throw crypto_error("register_prng", CRYPT_INVALID_PRNG);
	
	g_crypto_rng = ret;

	ret = register_cipher(&aes_desc);
	if(ret == -1) throw crypto_error("register_cipher", CRYPT_INVALID_CIPHER);

	g_aes_cipher = ret;

	ret = register_hash(&sha512_desc);
	if(ret == -1) throw crypto_error("register_hash", CRYPT_INVALID_HASH);

	g_sha512_hash = ret;
}

void crypto::base64_encode(std::vector<unsigned char> &buf, const unsigned char *in, unsigned long len) {
	unsigned long buflen = (len / 3 + 1) * 4;
	buf.resize(buflen);

	int ret = ::base64_encode(in, len, &buf[0], &buflen);

	if(ret == CRYPT_BUFFER_OVERFLOW) {
		buf.resize(buflen);
		ret = ::base64_encode(in, len, &buf[0], &buflen);
	}

	if(ret != CRYPT_OK) {
		throw crypto::crypto_error("base64_encode", ret);
	}
	
	buf.resize(buflen);
}

void crypto::base64_decode(std::vector<unsigned char> &buf, const unsigned char *in, unsigned long len) {
	unsigned long buflen = len / 4 * 3;
	buf.resize(buflen);

	int ret = ::base64_decode(in, len, &buf[0], &buflen);

	if(ret == CRYPT_BUFFER_OVERFLOW) {
		buf.resize(buflen);
		ret = ::base64_decode(in, len, &buf[0], &buflen);
	}

	if(ret != CRYPT_OK) {
		throw crypto::crypto_error("base64_decode", ret);
	}
	
	buf.resize(buflen);
}

void crypto::hash_password(const unsigned char *pass, unsigned long passlen, const unsigned char *salt, unsigned long saltlen, int iters, unsigned char *out, unsigned long outlen) {
	int ret = pkcs_5_alg2(pass, passlen, salt, saltlen, iters, g_sha512_hash, out, &outlen);
	if(ret != CRYPT_OK) throw crypto_error("pkcs_5_alg2", ret);
}

const char* crypto::crypto_error::what() const {
	return error_to_string(m_code);
}

crypto::sha512::sha512() {
	int ret = sha512_init(&m_state);
	if(ret != CRYPT_OK) throw crypto_error("sha512_init", ret);
}

void crypto::sha512::process(const unsigned char *in, unsigned long len) {
	int ret = sha512_process(&m_state, in, len);
	if(ret != CRYPT_OK) throw crypto_error("sha512_process", ret);
}
void crypto::sha512::finalize(unsigned char (&hash)[64]) {
	int ret = sha512_done(&m_state, hash);
	if(ret != CRYPT_OK) throw crypto_error("sha512_done", ret);
}

void crypto::aes256_stream::start(unsigned char *iv, unsigned char *key) {
	BOOST_ASSERT(m_init == false);

	int ret = ctr_start(g_aes_cipher, iv, key, 32, 0, CTR_COUNTER_LITTLE_ENDIAN, &m_ctr);
	if(ret != CRYPT_OK) throw crypto_error("crt_start", ret);

	m_init = true;
}

void crypto::aes256_stream::reset(unsigned char *iv) {
	BOOST_ASSERT(m_init == true);

	int ret = ctr_setiv(iv, 16, &m_ctr);
	if(ret != CRYPT_OK) throw crypto_error("ctr_setiv", ret);
}

void crypto::aes256_stream::close() {
	if(m_init) {
		ctr_done(&m_ctr);
		m_init = false;
	}
}

void crypto::aes256_stream::encrypt(const unsigned char *in, unsigned char *out, unsigned long len) {
	BOOST_ASSERT(m_init == true);

	int ret = ctr_encrypt(in, out, len, &m_ctr);
	if(ret != CRYPT_OK) throw crypto_error("ctr_encrypt", ret);
}

void crypto::aes256_stream::decrypt(const unsigned char *in, unsigned char *out, unsigned long len) {
	BOOST_ASSERT(m_init == true);

	int ret = ctr_decrypt(in, out, len, &m_ctr);
	if(ret != CRYPT_OK) throw crypto_error("ctr_decrypt", ret);
}

void crypto::ecc_key::generate(key_size size) {
	BOOST_ASSERT(m_init == false);

	int ret = ecc_make_key(NULL, g_crypto_rng, (int)size, &m_key);
	if(ret != CRYPT_OK) throw crypto_error("ecc_make_key", ret);

	m_init = true;
}

void crypto::ecc_key::read(const unsigned char *in, unsigned long inlen) {
	BOOST_ASSERT(m_init == false);

	int ret = ecc_import(in, inlen, &m_key);
	if(ret != CRYPT_OK) throw crypto_error("ecc_import", ret);

	m_init = true;
}

void crypto::ecc_key::write(std::vector<unsigned char> &out, key_type type) {
	BOOST_ASSERT(m_init == true);
	
	unsigned long outlen = 213;
	out.resize(outlen);

	int ret = ecc_export(&out[0], &outlen, (int)type, &m_key);

	if(ret == CRYPT_BUFFER_OVERFLOW) {
		out.resize(outlen);
		ret = ecc_export(&out[0], &outlen, (int)type, &m_key);
	}

	if(ret != CRYPT_OK) throw crypto_error("ecc_export", ret);
	
	out.resize(outlen);
}

void crypto::ecc_key::close() {
	if(m_init) {
		ecc_free(&m_key);
		m_init = false;
	}
}

void crypto::ecc_key::sign(std::vector<unsigned char> &out, const unsigned	char *in, unsigned long inlen) {
	BOOST_ASSERT(m_init == true);

	unsigned long outlen = 139;
	out.resize(outlen);

	int ret = ecc_sign_hash(in, inlen, &out[0], &outlen, NULL, g_crypto_rng, &m_key);

	if(ret == CRYPT_BUFFER_OVERFLOW) {
		out.resize(outlen);
		ret = ecc_sign_hash(in, inlen, &out[0], &outlen, NULL, g_crypto_rng, &m_key);
	}

	if(ret != CRYPT_OK) throw crypto_error("ecc_sign_hash", ret);

	out.resize(outlen);
}

bool crypto::ecc_key::verify(const unsigned char *sig,  unsigned long siglen, const unsigned char *hash, unsigned long hashlen) {
	BOOST_ASSERT(m_init == true);

	int stat;

	int ret = ecc_verify_hash(sig, siglen, hash, hashlen, &stat, &m_key);
	if(ret != CRYPT_OK) throw crypto_error("ecc_sign_hash", ret);

	return stat != 0;
}

crypto::ecc_key::key_type crypto::ecc_key::type() const {
	return m_init ? ((key_type)m_key.type) : empty_key;
}
