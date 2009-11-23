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
#include "decompress.hpp"

class zip_memfile : public zlib_filefunc_def {
public:
	zip_memfile(const char *mem, std::size_t len) : m_mem(mem),m_iter(mem),m_len(len),m_left(len) {
		zopen_file = open_file;
		zread_file = read_file;
		zwrite_file = write_file;
		ztell_file = tell_file;
		zseek_file = seek_file;
		zclose_file = close_file;
		zerror_file = error_file;
		opaque = this;
	}

private:
	static void* open_file(void *obj, const char*, int) {
		return obj;
	}

	static uLong read_file(void *obj, void*, void *buf, uLong size) {
		zip_memfile *ms = (zip_memfile*)obj;

		std::size_t sz = std::min(ms->m_left, (std::size_t)size);

		if(sz) {
			memcpy(buf, ms->m_iter, sz);

			ms->m_iter += sz;
			ms->m_left -= sz;
		}

		return (uLong)sz;
	}

	static uLong write_file(void*, void*, const void *, uLong) {
		return 0;
	}

	static long tell_file(void *obj, void*) {
		zip_memfile *ms = (zip_memfile*)obj;
		return (long)(ms->m_iter - ms->m_mem);
	}

	static long seek_file(void *obj, void*, uLong offset, int origin) {
		zip_memfile *ms = (zip_memfile*)obj;

		std::size_t pos;

		switch(origin) {
			case ZLIB_FILEFUNC_SEEK_SET:
				pos = std::min(ms->m_len, (std::size_t)offset);
				break;
			case ZLIB_FILEFUNC_SEEK_CUR:
				pos = std::min(ms->m_len, ms->m_len - ms->m_left + (std::size_t)offset);
				break;
			case ZLIB_FILEFUNC_SEEK_END:
				pos = std::min(ms->m_len, ms->m_len - (std::size_t)offset);
				break;
			default:
				pos = 0;
				break;
		}

		ms->m_iter = ms->m_mem + pos;
		ms->m_left = ms->m_len - pos;

		return 0;
	}

	static int close_file(void*, void*) {
		return 0;
	}

	static int error_file(void*, void*) {
		return 0;
	}

	const char *m_mem, *m_iter;
	std::size_t m_len, m_left;
};

void unzip(std::vector<std::vector<char> > &bufs, const char *mem, std::size_t len) {
	zip_memfile ms(mem, len);

	unzFile fp = unzOpen2("ignored", &ms);
	if(!fp) throw std::runtime_error("unzOpen2 failed");

	int ret = unzGoToFirstFile(fp);
	if(ret != UNZ_OK && ret != UNZ_END_OF_LIST_OF_FILE) {
		unzClose(fp);
		throw std::runtime_error("unzGoToFirstFile failed");
	}

	if(ret != UNZ_END_OF_LIST_OF_FILE) {
		do {
			ret = unzOpenCurrentFile(fp);
			if(ret != UNZ_OK) {
				unzClose(fp);
				throw std::runtime_error("unzOpenCurrentFile failed");
			}

			unz_file_info info;
			ret = unzGetCurrentFileInfo(fp, &info, NULL, 0, NULL, 0, NULL, 0);
			if(ret != UNZ_OK) {
				unzCloseCurrentFile(fp);
				unzClose(fp);
				throw std::runtime_error("unzGetCurrentFileInfo failed");
			}

			bufs.push_back(std::vector<char>());
			std::vector<char> &buf = bufs.back();

			buf.resize(info.uncompressed_size);

			ret = unzReadCurrentFile(fp, &buf.front(), info.uncompressed_size);
			if(ret != info.uncompressed_size) {
				unzCloseCurrentFile(fp);
				unzClose(fp);
				throw std::runtime_error("unzReadCurrentFile failed");
			}

			ret = unzCloseCurrentFile(fp);
			if(ret != UNZ_OK) {
				unzClose(fp);
				throw std::runtime_error("unzCloseCurrentFile failed");
			}
		} while((ret = unzGoToNextFile(fp)) == UNZ_OK);
	}

	unzClose(fp);
	
	if(ret != UNZ_END_OF_LIST_OF_FILE) {
		throw std::runtime_error("unzGoToNextFile failed");
	}
}
