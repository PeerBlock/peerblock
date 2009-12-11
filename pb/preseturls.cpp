/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC

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

LPCTSTR const g_presets[]={
	_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n"),
	_T("http://list.iblocklist.com/lists/bluetack/edu"),
	_T("http://list.iblocklist.com/lists/bluetack/level-1"),
	_T("http://list.iblocklist.com/lists/bluetack/spyware"),
	_T("http://list.iblocklist.com/lists/bluetack/level-2"),
	_T("http://list.iblocklist.com/lists/bluetack/level-3"),
	_T("http://list.iblocklist.com/lists/bluetack/bogon"),
	_T("http://list.iblocklist.com/lists/bluetack/dshield"),
	_T("http://list.iblocklist.com/lists/bluetack/hijacked"),
	_T("http://list.iblocklist.com/lists/bluetack/microsoft"),
	_T("http://list.iblocklist.com/lists/bluetack/iana-multicast"),
	_T("http://list.iblocklist.com/lists/bluetack/iana-private"),
	_T("http://list.iblocklist.com/lists/bluetack/iana-reserved"),
	_T("http://list.iblocklist.com/lists/bluetack/for-non-lan-computers"),
};

const size_t g_presetcount=sizeof(g_presets)/sizeof(LPCTSTR);
