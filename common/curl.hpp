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

#include <stdexcept>
#include <windows.h>
#include <winsock2.h>
#include <curl/curl.h>
#include "win32_error.hpp"

namespace curl {

class curl_error : public std::runtime_error {
public:
	curl_error(CURLcode code) : std::runtime_error(curl_easy_strerror(code)) {}
	curl_error(CURLMcode code) : std::runtime_error(curl_multi_strerror(code)) {}
	curl_error(const char *msg) : std::runtime_error(msg) {}
};

class multi_handle;

class easy_handle {
public:
	easy_handle() : m_handle(curl_easy_init()),m_multi(0) {
		if(!m_handle) throw curl_error("unable to init easy handle");

		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PRIVATE, this);
		if(ret != CURLE_OK) {
			curl_easy_cleanup(m_handle);
			throw curl_error(ret);
		}
	}

	~easy_handle() {
		if(m_multi) {
			curl_multi_remove_handle(m_multi, m_handle);
		}
		curl_easy_cleanup(m_handle);
	}

	void* private_data() const { return m_private; }
	void private_data(void *data) { m_private = data; }

	void preconnect(curl_preconnect_callback cb) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PRECONNECT, cb);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void preconnect_data(void *data) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PRECONNECTDATA, data);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void progress(curl_progress_callback cb) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PROGRESSFUNCTION, cb);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void progress_data(void *data) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PROGRESSDATA, data);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void noprogress(bool np) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_NOPROGRESS, np ? 1 : 0);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void write(curl_write_callback cb) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, cb);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void write_data(void *data) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, data);
		if(ret != CURLE_OK) throw curl_error(ret);
	}
	
	void url(const char *url) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_URL, url);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void accept_encodings(const char *encodings) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_ENCODING, encodings);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void follow_location(bool follow) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, follow ? 1 : 0);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void auto_referer(bool sendreferer) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_AUTOREFERER, sendreferer ? 1 : 0);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void header(const curl_slist *slist) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, slist);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void timeout(long t) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, t);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void connect_timeout(long t) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, t);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void proxy(const char *host) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PROXY, host);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void proxy_type(long type) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_PROXYTYPE, type);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void time_condition(long cond) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_TIMECONDITION, cond);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	void time_value(long val) {
		CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_TIMEVALUE, val);
		if(ret != CURLE_OK) throw curl_error(ret);
	}

	long response_code() {
		long code;

		CURLcode ret = curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &code);
		if(ret != CURLE_OK) throw curl_error(ret);

		return code;
	}

private:
	friend multi_handle;

	easy_handle(const easy_handle&);
	void operator=(const easy_handle&);

	CURL *m_handle, *m_multi;
	void *m_private;
};

class multi_info {
public:
	CURLMSG message() const { return m_msg->msg; }
	void *data() const { return m_msg->data.whatever; }
	CURLcode result() const { return m_msg->data.result; }

	easy_handle& handle() const {
		void *data;

		CURLcode ret = curl_easy_getinfo(m_msg->easy_handle, CURLINFO_PRIVATE, &data);
		if(ret != CURLE_OK) throw curl_error(ret);

		return *(easy_handle*)data;
	}

private:
	friend multi_handle;

	CURLMsg *m_msg;
};

class multi_handle {
public:
	multi_handle() : m_handle(curl_multi_init()) {
		if(!m_handle) throw curl_error("unable to init multi handle");
	}

	~multi_handle() {
		curl_multi_cleanup(m_handle);
	}

	void pipelining(bool pipeline) {
		CURLMcode ret = curl_multi_setopt(m_handle, CURLMOPT_PIPELINING, pipeline ? 1 : 0);
		if(ret != CURLM_OK) throw curl_error(ret);
	}

	void add_handle(easy_handle &easy) {
		CURLMcode ret = curl_multi_add_handle(m_handle, easy.m_handle);
		if(ret != CURLM_OK) throw curl_error(ret);

		easy.m_multi = m_handle;
	}

	void remove_handle(easy_handle &easy) {
		if(easy.m_multi != m_handle) throw curl_error("easy handle does not exist in multi handle");

		CURLMcode ret = curl_multi_remove_handle(m_handle, easy.m_handle);
		if(ret != CURLM_OK) throw curl_error(ret);

		easy.m_multi = 0;
	}

	long timeout() {
		long t;

		CURLMcode ret = curl_multi_timeout(m_handle, &t);
		if(ret != CURLM_OK) throw curl_error(ret);

		return t;
	}

	void fdsets(fd_set &read, fd_set &write, fd_set &err, int &maxfd) {
		CURLMcode ret = curl_multi_fdset(m_handle, &read, &write, &err, &maxfd);
		if(ret != CURLM_OK) throw curl_error(ret);
	}

	void select() {
		fd_set rfds, wfds, efds;
		int maxfd;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		fdsets(rfds, wfds, efds, maxfd);

		long t = timeout();

		timeval tv;
		tv.tv_sec = t / 1000;
		tv.tv_usec = (t % 1000) * 1000;

		int ret = ::select(maxfd + 1, &rfds, &wfds, &efds, &tv);
		if(ret != SOCKET_ERROR) throw win32_error("select");
	}

	bool perform(int &handles) {
		CURLMcode ret = curl_multi_perform(m_handle, &handles);
		if(ret == CURLM_CALL_MULTI_PERFORM) return true;
		else if(ret == CURLM_OK) return false;
		else throw curl_error(ret);
	}

	bool read_info(multi_info &info) {
		int msgcount;
		info.m_msg = curl_multi_info_read(m_handle, &msgcount);

		return (info.m_msg != 0);
	}

private:
	multi_handle(const multi_handle&);
	void operator=(const multi_handle&);

	CURLM *m_handle;
};

}
