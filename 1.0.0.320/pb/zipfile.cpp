/*
	Copyright (C) 2004-2005 Cory Nelson

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
using namespace std;

ZipFile::ZipFile() : fp(NULL) {
}

ZipFile::ZipFile(const path &file) : fp(NULL) {
	this->Open(file);
}

ZipFile::~ZipFile() {
	this->Close();
}

void ZipFile::Open(const path &file) {
	this->Close();
	this->fp=unzOpen(TSTRING_MBS(file.file_str()).c_str());
	if(this->fp==NULL) throw zip_error("unzOpen");
}

void ZipFile::Close() {
	if(this->IsOpen()) {
		int ret=unzClose(this->fp);
		if(ret!=UNZ_OK) throw zip_error("unzClose", ret);
		this->fp=NULL;
	}
}

bool ZipFile::IsOpen() const {
	return (this->fp!=NULL);
}

bool ZipFile::GoToFirstFile() {
	return (unzGoToFirstFile(this->fp)==UNZ_OK);
}

bool ZipFile::GoToNextFile() {
	return (unzGoToNextFile(this->fp)==UNZ_OK);
}

void ZipFile::OpenCurrentFile() {
	int ret=unzOpenCurrentFile(this->fp);
	if(ret!=UNZ_OK) throw zip_error("unzOpenCurrentFile", ret);
}

void ZipFile::CloseCurrentFile() {
	int ret=unzCloseCurrentFile(this->fp);
	if(ret!=UNZ_OK) throw zip_error("unzCloseCurrentFile", ret);
}

string ZipFile::GetCurrentFileName() {
	char buf[1024];

	int ret=unzGetCurrentFileInfo(this->fp, NULL, buf, sizeof(buf), NULL, 0, NULL, 0);
	if(ret!=UNZ_OK) throw zip_error("unzGetCurrentFileInfo", ret);

	return buf;
}

pair<boost::shared_array<char>,size_t> ZipFile::ReadCurrentFile() {
	unz_file_info info;
	int ret=unzGetCurrentFileInfo(this->fp, &info, NULL, 0, NULL, 0, NULL, 0);
	if(ret!=UNZ_OK) throw zip_error("unzGetCurrentFileInfo", ret);

	boost::shared_array<char> buf(new char[info.uncompressed_size]);

	ret=unzReadCurrentFile(fp, buf.get(), info.uncompressed_size);
	if(ret!=info.uncompressed_size) throw zip_error("unzReadCurrentFile", ret);

	return make_pair(buf, info.uncompressed_size);
}

string zip_error::errmsg(int e) {
	switch(e) {
		case UNZ_END_OF_LIST_OF_FILE:		return "end of file list reached";
		case UNZ_EOF:						return "end of file reached";
		case UNZ_PARAMERROR:				return "invalid parameter given";
		case UNZ_BADZIPFILE:				return "bad zip file";
		case UNZ_INTERNALERROR:				return "internal error";
		case UNZ_CRCERROR:					return "crc error, file is corrupt";
		case UNZ_ERRNO:						return strerror(errno);
		default:							return "unknown error ("+boost::lexical_cast<string>(e)+")";
	}
}