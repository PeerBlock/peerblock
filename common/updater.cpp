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
#include "unicode.hpp"
#include "updater.hpp"

updater::updater(global_config &config, const backend::updating_function &onupdate, async_state &cancel)
: m_config(config),m_onupdate(onupdate),m_cancel(cancel)
{
	m_multi.pipelining(true);
}

updater::~updater() {
	m_updates.clear_and_dispose(update_ctx::disposer());
}

static unsigned __int64 get_version() {
	path p = path::base_dir();

	DWORD handle;
	DWORD size = GetFileVersionInfoSize(p.c_str(), &handle);

	if(size) {
		boost::scoped_array<char> data(new char[size]);

		BOOL res = GetFileVersionInfo(p.c_str(), handle, size, data.get());
		if(res) {
			VS_FIXEDFILEINFO *ffi;
			UINT ffilen;

			res = VerQueryValue(data.get(), L"\\", (void**)&ffi, &ffilen);
			if(res) {
				return (static_cast<unsigned __int64>(ffi->dwFileDateMS) << 32) | ffi->dwFileDateLS;
			}	
		}
	}
	
	return 0;
}

static const curl_slist g_headers[] = {
	{ "Accept: application/x-7z-compressed, application/x-gzip, application/zip, application/octet-stream, text/plain", (curl_slist*)&g_headers[1] },
	{ "User-Agent: PeerGuardian/3.0", 0 }
};

void updater::start() {
	// create pg update context.

	if(m_config.updates.updatepg) {
		pg_ctx &ctx = *new pg_ctx(*this);
		m_updates.push_back(ctx);

		{
			char buf[256];
			sprintf(buf, "http://peerguardian.sourceforge.net/update.php?build=%x-%I64x", NTDDI_VERSION, get_version());

			ctx.url = buf;
		}
		
		ctx.stateidx = m_state.updates.size();
		m_state.updates.push_back(backend::listupdate_state());
		backend::listupdate_state &uctx = m_state.updates.back();

		uctx.stage = backend::listupdate_state::connecting;
		unicode::transcode<unicode::utf8, unicode::wchar_encoding>(ctx.url, uctx.url);
		uctx.progress = 0.0f;

		if(m_config.updates.proxy.server.size()) {
			unicode::transcode<unicode::wchar_encoding, unicode::utf8>(m_config.updates.proxy.server, ctx.proxy);
		}
		
		ctx.con.url(ctx.url.c_str());

		if(ctx.proxy.size()) {
			ctx.con.proxy_type(m_config.updates.proxy.type);
			ctx.con.proxy(ctx.proxy.c_str());
		}

		ctx.con.header(&g_headers[0]);

		ctx.con.timeout(m_config.updates.timeout);
		ctx.con.connect_timeout(m_config.updates.timeout);

		ctx.con.private_data(&ctx);

		ctx.con.preconnect(update_ctx::on_preconnect);
		ctx.con.preconnect_data((update_ctx*)&ctx);

		ctx.con.progress(update_ctx::on_progress);
		ctx.con.progress_data((update_ctx*)&ctx);
		ctx.con.noprogress(false);

		ctx.con.write(pg_ctx::on_write);
		ctx.con.write_data(&ctx);

		m_multi.add_handle(ctx.con);
	}

	// create list update contexts.

	if(m_config.updates.updatelists) {
		for(std::vector<pg3_list>::size_type i = 0; i < m_config.lists.lists.size(); ++i) {
			pg3_list &list = m_config.lists.lists[i];

			list_ctx &ctx = *new list_ctx(*this, list);
			m_updates.push_back(ctx);
			
			ctx.stateidx = m_state.updates.size();
			m_state.updates.push_back(backend::listupdate_state());
			backend::listupdate_state &uctx = m_state.updates.back();

			uctx.stage = backend::listupdate_state::connecting;
			uctx.url = list.path;
			uctx.progress = 0.0f;
			
			ctx.newfile = list.real_path();
			ctx.tempfile = path::temp_file(L"pg3");

			ctx.file.open(ctx.tempfile.c_str(), std::ofstream::binary);

			if(!ctx.file.is_open()) {
				m_updates.erase_and_dispose(m_updates.iterator_to(ctx), update_ctx::disposer());

				uctx.stage = backend::listupdate_state::error;
				uctx.progress = 1.0f;

				continue;
			}

			unicode::transcode<unicode::wchar_encoding, unicode::utf8>(list.path, ctx.url);

			if(list.proxy.server.size()) {
				ctx.con.proxy_type(list.proxy.type);
				unicode::transcode<unicode::wchar_encoding, unicode::utf8>(list.proxy.server, ctx.proxy);
			}
			else if(m_config.updates.proxy.server.size()) {
				ctx.con.proxy_type(m_config.updates.proxy.type);
				unicode::transcode<unicode::wchar_encoding, unicode::utf8>(m_config.updates.proxy.server, ctx.proxy);
			}

			ctx.con.url(ctx.url.c_str());

			if(list.lastupdate && path::exists(list.real_path())) {
				ctx.con.time_condition(CURL_TIMECOND_IFMODSINCE);
				ctx.con.time_value((long)list.lastupdate);
			}

			if(ctx.proxy.size()) {
				ctx.con.proxy(ctx.proxy.c_str());
			}

			ctx.con.header(&g_headers[0]);

			ctx.con.timeout(m_config.updates.timeout);
			ctx.con.connect_timeout(m_config.updates.timeout);

			ctx.con.private_data(&ctx);

			ctx.con.preconnect(update_ctx::on_preconnect);
			ctx.con.preconnect_data((update_ctx*)&ctx);

			ctx.con.progress(update_ctx::on_progress);
			ctx.con.progress_data((update_ctx*)&ctx);
			ctx.con.noprogress(false);

			ctx.con.write(list_ctx::on_write);
			ctx.con.write_data(&ctx);

			m_multi.add_handle(ctx.con);
		}
	}

	// send initial "starting" state.
	m_state.stage = backend::updater_state::starting;
	m_state.progress = 0.0f;
	m_state.pgupdate = false;
	m_onupdate(m_state);

	m_state.stage = backend::updater_state::working;

	// start updating.

	int running = 1;
	while(m_multi.perform(running));

	while(running) {
		if(m_cancel.needs_cancel()) {
			m_state.stage = backend::updater_state::aborted;
			m_onupdate(m_state);
			return;
		}

		m_multi.select();

		while(m_multi.perform(running));

		curl::multi_info info;
		while(m_multi.read_info(info)) {
			if(info.message() == CURLMSG_DONE) {
				update_ctx &ctx = *(update_ctx*)info.handle().private_data();
				backend::listupdate_state &uctx = m_state.updates[ctx.stateidx];

				if(info.result() == CURLE_OK) {
					long res = ctx.con.response_code();

					ctx.finish(res);
				}
				else {
					m_state.updates[ctx.stateidx].stage = backend::listupdate_state::network_error;
					m_updates.erase_and_dispose(m_updates.iterator_to(ctx), update_ctx::disposer());
				}
				
				// send state.

				uctx.progress = 1.0f;

				m_state.progress = 0.0f;
				for(std::vector<backend::listupdate_state>::size_type i = 0; i < m_state.updates.size(); ++i) {
					m_state.progress += m_state.updates[i].progress;
				}
				m_state.progress /= m_state.updates.size();

				m_onupdate(m_state);
			}
		}
	}

	m_state.stage = backend::updater_state::finished;
	m_state.progress = 1.0f;
	m_onupdate(m_state);
}

void updater::pg_ctx::finish(long result) {
	if(result == 200) {
		char *endptr;
		unsigned __int64 build = _strtoi64(version.c_str(), &endptr, 16);

		if(build && endptr == (version.c_str() + version.length())) {
			u.m_state.pgupdate = (build > get_version());
		}

		u.m_state.updates[stateidx].stage = backend::listupdate_state::done;
	}
	else {
		u.m_state.updates[stateidx].stage = backend::listupdate_state::network_error;
	}
	
	u.m_updates.erase_and_dispose(u.m_updates.iterator_to(*this), update_ctx::disposer());
}

void updater::list_ctx::finish(long result) {
	file.close();

	if(result == 200) {
		try {
			path::move(tempfile, newfile, true);
			
			list.lastupdate = time(NULL);
			list.failedupdate = false;
			
			u.m_state.updates[stateidx].stage = backend::listupdate_state::done;
		}
		catch(...) {
			list.failedupdate = true;
			u.m_state.updates[stateidx].stage = backend::listupdate_state::error;
		}
	}
	else if(result == 304) {
		list.lastupdate = time(NULL);
		list.failedupdate = false;

		u.m_state.updates[stateidx].stage = backend::listupdate_state::not_available;
	}
	else {
		list.failedupdate = true;
		u.m_state.updates[stateidx].stage = backend::listupdate_state::network_error;
	}

	u.m_updates.erase_and_dispose(u.m_updates.iterator_to(*this), update_ctx::disposer());
}

void updater::update_ctx::on_preconnect(void *arg, struct sockaddr *addr, int addrlen) {
	update_ctx &ctx = *(update_ctx*)arg;
	//TODO: allow the ip in the backend.
}

int updater::update_ctx::on_progress(void *arg, double total, double pos, double, double) {
	if(total > 0.0) {
		update_ctx &ctx = *(update_ctx*)arg;
		backend::updater_state &state = ctx.u.m_state;

		state.updates[ctx.stateidx].progress = float(pos / total);
		
		// send state.
		
		state.progress = 0.0f;
		for(std::vector<backend::listupdate_state>::size_type i = 0; i < state.updates.size(); ++i) {
			state.progress += state.updates[i].progress;
		}
		state.progress /= state.updates.size();
		
		ctx.u.m_onupdate(state);
	}

	return 0;
}

size_t updater::pg_ctx::on_write(char *buffer, size_t size, size_t nitems, void *arg) {
	size_t len = size * nitems;

	pg_ctx &ctx = *(pg_ctx*)arg;
	ctx.version.append(buffer, len);

	return len;
}

size_t updater::list_ctx::on_write(char *buffer, size_t size, size_t nitems, void *arg) {
	size_t len = size * nitems;

	list_ctx &ctx = *(list_ctx*)arg;
	ctx.file.write(buffer, (std::streamsize)len);

	return len;
}
