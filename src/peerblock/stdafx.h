/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2010 PeerBlock, LLC

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

#define INITGUID
#define WIN32_LEAN_AND_MEAN
#define TIXML_USE_STL
#define NOMINMAX
#define CURL_STATICLIB
#define STRICT
#define _LZMA_PROB32
#define _LZMA_LOC_OPT
#define _LZMA_SYSTEM_SIZE_T
#define _LZMA_IN_CB
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE
#define PB_REPORT_BUGS
#define BOOST_USE_WINDOWS_H

#include <set>
#include <map>
#include <list>
#include <stack>
#include <queue>
#include <bitset>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <hash_map>
#include <strstream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <boost/crc.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/functional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/random/mersenne_twister.hpp>
#pragma warning(disable:4244 4267)
#include <boost/format.hpp>
#pragma warning(default:4244 4267)
#include <p2p/list.hpp>
#include <p2p/range.hpp>

#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _WIN32_WINNT
#if _WIN32_WINNT == 0x0500
#include <windns.h>
#endif
#include <winioctl.h>
#endif
#include <iphlpapi.h>
#include <olectl.h>
#include <ole2.h>
#include <strsafe.h>

#include <zlib.h>
#include <unzip.h>

extern "C" {
#include <7zCrc.h>
#include <7zIn.h>
#include <7zExtract.h>
}

#include <curl/curl.h>
#include <tinyxml.h>
#include <sqlite3x.hpp>

#include "versioninfo_parsed.h"
#include "tstring.h"
#include "char_convert.h"

#ifdef _WIN32_WINNT
#include <filter.h>
#include "win32_error.h"
#include "driver.h"
#if _WIN32_WINNT >= 0x0600
#include "pbfilter_wfp.h"
#else
#include "pbfilter_nt.h"
#endif
#endif

#include "loadstring.h"
#include "messagebox.h"
#include "configuration.h"
#include "lists.h"
#include "getips.h"
#include "zipfile.h"
#include "pathx.hpp"
#include "threadx.hpp"
#include "preseturls.h"
#include "threaded_sqlite3x.hpp"

#include "BaseDialog.h"
#include "mainproc.h"
#include "logproc.h"
#include "settingsproc.h"
#include "listsproc.h"
#include "addlistproc.h"
#include "editlistproc.h"
#include "createlistproc.h"
#include "splashproc.h"
#include "aboutproc.h"
#include "listproc.h"
#include "loadingproc.h"
#include "historyproc.h"
#include "exporthistoryproc.h"
#include "historycalendarproc.h"
#include "historyfindproc.h"
#include "updatelists.h"
#include "startupwizard.h"
#include "colorpicker.h"
#include "editportsproc.h"
#include "portprofileproc.h"
