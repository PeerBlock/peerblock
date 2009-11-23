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

#include <string>
#include <fstream>
#include <boost/intrusive/list.hpp>

#include "path.hpp"
#include "config.hpp"
#include "backend.hpp"
#include "curl.hpp"

class updater {
public:
	updater(global_config &config, const backend::updating_function &onupdate, async_state &cancel);
	~updater();

	void start();

private:
	struct update_ctx : public boost::intrusive::list_base_hook<> {
		updater &u;
		std::vector<backend::listupdate_state>::size_type stateidx;

		curl::easy_handle con;
		std::string url, proxy;
		
		virtual void finish(long result) = 0;
		virtual ~update_ctx() {}

		static void on_preconnect(void *arg, struct sockaddr *addr, int addrlen);
		static int on_progress(void *arg, double dltotal, double dlnow, double ultotal, double ulnow);
		
		struct disposer {
			void operator()(update_ctx *ctx) const { delete ctx; }
		};

	protected:
		update_ctx(updater &u) : u(u) {}
	};

	struct pg_ctx : public update_ctx {
		std::string version;
		
		static size_t on_write(char *buffer, size_t size, size_t nitems, void *arg);

		pg_ctx(updater &u) : update_ctx(u) {}
		void finish(long result);
	};

	struct list_ctx : public update_ctx {
		pg3_list &list;

		std::ofstream file;
		path tempfile, newfile;
		
		static size_t on_write(char *buffer, size_t size, size_t nitems, void *arg);
		
		list_ctx(updater &u, pg3_list &list) : update_ctx(u),list(list) {}
		~list_ctx() {
			if(file.is_open()) file.close();
			if(path::exists(tempfile)) path::remove(tempfile);
		}

		void finish(long result);
	};

	typedef boost::intrusive::list<update_ctx> ctxlist_type;

	curl::multi_handle m_multi;

	ctxlist_type m_updates;
	backend::updater_state m_state;
	
	global_config &m_config;
	backend::updating_function m_onupdate;

	async_state &m_cancel;
};
