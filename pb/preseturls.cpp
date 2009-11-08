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
	_T("http://list.iblocklist.com/?list=bt_ads"),
	_T("http://list.iblocklist.com/?list=bt_edu"),
	_T("http://list.iblocklist.com/?list=bt_level1"),
	_T("http://list.iblocklist.com/?list=bt_spyware"),
	_T("http://www.bluetack.co.uk/config/ads-trackers-and-bad-pr0n.gz"),
	_T("http://www.bluetack.co.uk/config/level1.gz"),
	_T("http://www.bluetack.co.uk/config/level2.gz"),
	_T("http://www.bluetack.co.uk/config/bogon.zip"),
	_T("http://www.bluetack.co.uk/config/dshield.zip"),
	_T("http://www.bluetack.co.uk/config/edu.gz"),
	_T("http://www.bluetack.co.uk/config/hijacked.zip"),
	_T("http://www.bluetack.co.uk/config/iana-multicast.zip"),
	_T("http://www.bluetack.co.uk/config/iana-private.zip"),
	_T("http://www.bluetack.co.uk/config/iana-reserved.zip"),
	_T("http://www.bluetack.co.uk/config/Microsoft.gz"),
	_T("http://www.bluetack.co.uk/config/fornonlancomputers.zip"),
	_T("http://www.bluetack.co.uk/config/spider.gz"),
	_T("http://www.bluetack.co.uk/config/spyware.gz"),
	_T("http://www.bluetack.co.uk/config/trojan.zip")
};

const size_t g_presetcount=sizeof(g_presets)/sizeof(LPCTSTR);
