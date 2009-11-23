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

class sz_memstream : public ISzInStream {
public:
	sz_memstream(const char *mem, std::size_t len) : m_mem(mem),m_iter(mem),m_len(len),m_left(len) {
		Read = read;
		Seek = seek;
	}

private:
	static SZ_RESULT read(void *obj, void **buf, size_t maxrequired, size_t *processed) {
		sz_memstream *ms = (sz_memstream*)obj;

		size_t size = std::min(maxrequired, ms->m_left);

		*buf = (void*)ms->m_iter;
		if(processed) *processed = size;
		
		ms->m_iter += size;
		ms->m_left -= size;

		return SZ_OK;
	}

	static SZ_RESULT seek(void *obj, CFileSize pos) {
		sz_memstream *ms = (sz_memstream*)obj;

		std::size_t size = std::min(ms->m_len, (std::size_t)pos);

		ms->m_iter = ms->m_mem + size;
		ms->m_left = ms->m_len - size;

		return SZ_OK;
	}
	
	const char *m_mem, *m_iter;
	std::size_t m_len, m_left;
};

void un7zip(std::vector<std::vector<char> > &bufs, const char *mem, std::size_t len) {
	sz_memstream ms(mem, len);

	ISzAlloc ai;
	ai.Alloc=SzAlloc;
	ai.Free=SzFree;

	ISzAlloc aitemp;
	aitemp.Alloc=SzAllocTemp;
	aitemp.Free=SzFreeTemp;

	{
		volatile static bool crcinit = false;
		if(!crcinit) {
			CrcGenerateTable();
			crcinit = true;
		}
	}

	CArchiveDatabaseEx db;
	SzArDbExInit(&db);

	SZ_RESULT res = SzArchiveOpen(&ms, &db, &ai, &aitemp);
	if(res != SZ_OK) {
		SzArDbExFree(&db, ai.Free);
		throw std::runtime_error("SzArchiveOpen failed");
	}

	bufs.resize(db.Database.NumFiles);

	Byte *outbuf = 0;
	size_t outbufsize = 0;
	UInt32 blockindex = 0;

	for(UInt32 i = 0; i < db.Database.NumFiles; ++i) {
		size_t offset, processed;

		res = SzExtract(&ms, &db, i, &blockindex, &outbuf, &outbufsize, &offset, &processed, &ai, &aitemp);
		if(res != SZ_OK) {
			if(outbuf) ai.Free(outbuf);
			SzArDbExFree(&db, ai.Free);

			throw std::runtime_error("SzExtract failed");
		}

		if(processed) {
			bufs[i].assign(outbuf, outbuf + processed);
		}
	}
	
	if(outbuf) ai.Free(outbuf);
	SzArDbExFree(&db, ai.Free);
}
