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

#include "defs.hpp"

// libcurl stuff.
#define CURL_STATICLIB

// 7-zip stuff.
#define _SZ_ONE_DIRECTORY
#define _LZMA_PROB32
#define _LZMA_LOC_OPT
#define _LZMA_IN_CB

// zlib stuff.
#define NO_GZCOMPRESS

// libtomcrypt stuff.
#define LTM_DESC

#include <ctime>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <list>
#include <vector>
#include <string>
#include <limits>
#include <memory>
#include <hash_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <exception>

#pragma warning(push)
#pragma warning(disable:6011 6334)
// disable some code analysis warnings that VC++ generates.
// 6011 disables "Dereferencing NULL pointer"
// 6334 disables "sizeof operator applied to an expression with an operator might yield unexpected results"

#include <boost/crc.hpp>
#include <boost/bind.hpp>
#include <boost/assert.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#pragma warning(pop)

#include <windows.h>
#include <winioctl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <cguid.h>
#include <atlbase.h>
#include <xmllite.h>

#include <sqlite3.h>

#include <tomcrypt.h>

#include <curl/curl.h>

#include <zlib.h>
#include <unzip.h>

extern "C" {
#include <7zCrc.h>
#include <7zIn.h>
#include <7zExtract.h>
}
