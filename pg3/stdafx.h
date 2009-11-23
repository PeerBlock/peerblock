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

#define _WIN32_DCOM

#include "defs.hpp"

#include <cstddef>
#include <cstring>
#include <ctime>

#include <list>
#include <deque>
#include <vector>
#include <hash_map>
#include <string>
#include <memory>
#include <ios>
#include <iomanip>
#include <ostream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <exception>

#include <boost/crc.hpp>
#include <boost/bind.hpp>

#pragma warning(push)
#pragma warning(disable:4244)
#include <boost/format.hpp>
#pragma warning(pop)

#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/intrusive/list.hpp>

#include <windows.h>
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cguid.h>
#include <atlbase.h>
#include <olectl.h>
#include <ocidl.h>
#include <uxtheme.h>

#include "agg.hpp"
