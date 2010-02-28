//================================================================================================
//  listurls.cpp
//
//  Contains all known lists, for duplicate/sanity-checking purposes.
//================================================================================================

/*
	Copyright (C) 2010 PeerBlock, LLC

	This software is provided 'as-is', without any express or implied warranty.  In no event will 
	the authors be held liable for any damages arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, including commercial 
	applications, and to alter it and redistribute it freely, subject to the 
	following restrictions:

	1. The origin of this software must not be misrepresented; you must not claim that you wrote 
		the original software. If you use this software in a product, an acknowledgment in the 
		product documentation would be appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be misrepresented as 
		being the original software.
	3. This notice may not be removed or altered from any source distribution.

*/

#include "stdafx.h"

#include "listurls.h"



//================================================================================================
//
//  FindListNum()
//
//    - Called by ?
//
/// <summary>
///   Returns the List Name ID to which the specified URL belongs, or LISTNAME_COUNT if not found.
/// </summary>
//
LISTNAME ListUrls::FindListNum(wstring _url) 
{ 
	//TRACEV("[ListUrls] [FindListNum]  > Entering routine.");

	//tstring strBuf = boost::str(tformat(_T("[ListUrls] [FindListNum]   looking for url: [%1%]")) % _url.c_str() );
	//TRACEBUFI(strBuf);

	int listName;
	bool foundUrl = false;

	// loop through each List Name
	for (listName=0; listName<LISTNAME_COUNT && foundUrl==false; ++listName)	
	{
		// loop through each URL for that List
		for (unsigned int listUrl=LISTS_FIELD_BESTURL; listUrl<Lists[listName].size() && foundUrl==false; ++listUrl)	
		{
			// check for specified URL
			if (Lists[listName][listUrl].Url.compare(_url) == 0)
			{
				//tstring strBuf = boost::str(tformat(_T("[ListUrls] [FindListNum]   found url at: [%1%][%2%]")) 
				//	% listName % listUrl );
				//TRACEBUFI(strBuf);
				foundUrl = true;	// found it!
				break;
			}
		}
	}

	if (foundUrl)
		listName -= 1;	// we're incrementing listName one extra time, after finding a match

	//TRACEI("[ListUrls] [FindListNum]  < Leaving routine.");
	return (LISTNAME)(listName);	

}; // End of FindListNum()



//================================================================================================
//
//  CheckUrl()
//
//    - Called by AddList_SanityCheckUrl(), when processing IDOK
//
/// <summary>
///   Returns the set of flags corresponding to the specified list, for use in sanity-checking.
///   If returns 0, then this is either the "best" URL or else we couldn't find it in our list
///   of lists.
/// </summary>
/// <param name="url">
///   The URL which the callers wants to sanity-check.
/// </param>
/// <param name="num">
///   A "hint" as to which row of our internal Lists array the specified URL belongs.  This 
///   parameter should have been discovered by the caller via a previous call to FindListNum().
///   If the caller wants to explicitly give no hint, this parameter should be set to
///   LISTNAME_COUNT.
/// </param>
/// <param name="listman">
///   If called from within the ListManager, this should be the HWND of the ListManager window.  
///   We're using this instead of g_config.dynamiclists[] because it will be updated as soon as 
///   lists are added, instead of only updated after the ListManager is closed.  If this 
///   parameter is NULL, we will instead check against g_config.
/// </param>
//
LISTFLAGS ListUrls::CheckUrl(wstring _url, LISTNAME _num, HWND _listman)
{
	TRACEI("[ListUrls] [CheckUrl]  > Entering routine.");

	tstring strBuf = boost::str(tformat(_T("[ListUrls] [CheckUrl]    checking url: [%1%]")) % _url.c_str() );
	TRACEBUFI(strBuf);

	if (_num == LISTNAME_COUNT)
		_num = FindListNum(_url);

	strBuf = boost::str(tformat(_T("[ListUrls] [CheckUrl]    looking in row: [%1%]")) % _num );
	TRACEBUFI(strBuf);

	// search through all URLs associated with this ListName, to find the index into the row
	unsigned int idx;
	bool found = false;
	LISTFLAGS urlFlags = 0;
	for (idx=LISTS_FIELD_BESTURL; _num!=LISTNAME_COUNT && idx<Lists[_num].size(); ++idx)
	{
		if (Lists[_num][idx].Url.compare(_url) == 0)
		{
			// found it!
			tstring strBuf = boost::str(tformat(_T("[ListUrls] [CheckUrl]    found url at: [%1%][%2%]")) 
				% _num % idx );
			TRACEBUFI(strBuf);
			found = true;
			break;
		}
	}

	if (!found)
	{
		TRACEI("[ListUrls] [CheckUrl]    can't find URL");
	}
	else
	{
		// Start off with hardcoded list flags, adjust later as appropriate
		urlFlags = Lists[_num][idx].Flags;
	}

	// Check to see if this is a Default List
	if (_num <= LISTNAME_MAX_DEFAULT)
	{
		TRACEI("[ListUrls] [CheckUrl]    url is one of the default lists");
		urlFlags.set(LISTFLAG_DEFAULT);
	}

	// Check for duplicates
	if (_listman != NULL)
	{
		TRACEI("[ListUrls] [CheckUrl]    non-null listman handle");

		// Check ListManager's ListView
		HWND listman_list = GetDlgItem(_listman, IDC_LIST);
		int count=ListView_GetItemCount(listman_list);

		for(int i=0; i<count; i++) 
		{
			LVITEM lvi={0};
			lvi.iItem=i;
			lvi.mask=LVIF_PARAM;
			ListView_GetItem(listman_list, &lvi);
			List *plist=(List*)lvi.lParam;

			if(DynamicList *l=dynamic_cast<DynamicList*>(plist))
			{
				// TODO:  Should probably refactor this stuff into its own subroutine...

				// check against user-entered url
				if (l->Url.compare(_url) == 0)
				{
					TRACEI("[ListUrls] [CheckUrl]    found exact url in listman listview");
					urlFlags.set(LISTFLAG_EXACTDUPE);
				}
				else
				{
					// check against each url for this list-name 
					LISTNAME existingListId = FindListNum(l->Url);
					for (unsigned int j=LISTS_FIELD_BESTURL; existingListId!=LISTNAME_COUNT && j<Lists[existingListId].size(); ++j)
					{
						if (Lists[existingListId][j].Url.compare(_url) == 0)
						{
							TRACEI("[ListUrls] [CheckUrl]    found similar url in listman listview");
							urlFlags.set(LISTFLAG_DIFFDUPE);
						}
					}
				}
			}
		}

		// Check for already-configured similar-URLs in default lists
		extern vector<DynamicList> g_deflists;	// defined in listsproc.cpp
		LISTNAME existingListId = LISTNAME_COUNT;
		for(vector<DynamicList>::size_type i=0; i<g_deflists.size(); i++)
		{
			// check against user-entered url
			if (g_deflists[i].Url.compare(_url) == 0)
			{
				TRACEI("[ListUrls] [CheckUrl]    found exact url in g_config dynamic lists");
				urlFlags.set(LISTFLAG_EXACTDUPE);
			}
			else
			{
				// check against each url for this list-name 
				existingListId = FindListNum(g_deflists[i].Url);
				for (unsigned int j=LISTS_FIELD_BESTURL; existingListId!=LISTNAME_COUNT && j<Lists[existingListId].size(); ++j)
				{
					if (Lists[existingListId][j].Url.compare(_url) == 0)
					{
						TRACEI("[ListUrls] [CheckUrl]    found similar url in g_config dynamic lists");
						urlFlags.set(LISTFLAG_DIFFDUPE);
					}
				}
			}
		}
	}
	else
	{
		TRACEI("[ListUrls] [CheckUrl]    null listman");

		// Check g_config.DynamicLists[]
		LISTNAME existingListId = LISTNAME_COUNT;
		for(vector<DynamicList>::size_type i = 0; i < g_config.DynamicLists.size(); ++i)
		{
			// check against user-entered url
			if (g_config.DynamicLists[i].Url.compare(_url) == 0)
			{
				TRACEI("[ListUrls] [CheckUrl]    found exact url in g_config dynamic lists");
				urlFlags.set(LISTFLAG_EXACTDUPE);
			}
			else
			{
				// check against each url for this list-name 
				existingListId = FindListNum(g_config.DynamicLists[i].Url);
				for (unsigned int j=LISTS_FIELD_BESTURL; existingListId!=LISTNAME_COUNT && j<Lists[existingListId].size(); ++j)
				{
					if (Lists[existingListId][j].Url.compare(_url) == 0)
					{
						TRACEI("[ListUrls] [CheckUrl]    found similar url in g_config dynamic lists");
						urlFlags.set(LISTFLAG_DIFFDUPE);
					}
				}
			}
		}
	}

	// Dump flags to tracelog
	if (urlFlags.test(LISTFLAG_NOT_IBL)) TRACEI("[ListUrls] [CheckUrl]   list has LISTFLAG_NOT_IBL set");
	if (urlFlags.test(LISTFLAG_DEFAULT)) TRACEI("[ListUrls] [CheckUrl]   list has LISTFLAG_DEFAULT set");
	if (urlFlags.test(LISTFLAG_UNFRIENDLY)) TRACEI("[ListUrls] [CheckUrl]   list has LISTFLAG_UNFRIENDLY set");
	if (urlFlags.test(LISTFLAG_WRONG)) TRACEI("[ListUrls] [CheckUrl]   list has LISTFLAG_WRONG set");
	if (urlFlags.test(LISTFLAG_EXACTDUPE)) TRACEI("[ListUrls] [CheckUrl]   list has LISTFLAG_EXACTDUPE set");
	if (urlFlags.test(LISTFLAG_DIFFDUPE)) TRACEI("[ListUrls] [CheckUrl]   list has LISTFLAG_DIFFDUPE set");

	// Done!
	TRACEI("[ListUrls] [CheckUrl]  < Leaving routine.");
	return urlFlags;

}; // End of CheckUrl()



//================================================================================================
//
//  GetListDesc()
//
//    - Called by ?
//
/// <summary>
///   Returns the List Description as specified in Init().
/// </summary>
//
wstring ListUrls::GetListDesc(LISTNAME url_num)
{
	return Lists[url_num][LISTS_FIELD_DESC].Url;

}; // End of GetListDesc()



//================================================================================================
//
//  Init()
//
//    - Called by ?
//
/// <summary>
///   Sets up our mapping of List Name IDs to the List Description and various URLs that 
///   correspond to them.  Also sets list flags, to let the caller know what to do about a given
///   URL when sanity-checking it.
/// </summary>
//
bool ListUrls::Init()
{
	TRACEI("[ListUrls] [Init]  > Entering routine.");

	try
	{
		Lists.resize(LISTNAME_COUNT);

		// Default Lists

		Lists[LISTNAME_BT_LEVEL1].push_back(ListData(L"Level 1 (BlueTack) (aka 'P2P')"));
		Lists[LISTNAME_BT_LEVEL1].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/level-1"));
		Lists[LISTNAME_BT_LEVEL1].push_back(ListData(L"http://list.iblocklist.com/?list=bt_level1", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_LEVEL1].push_back(ListData(L"http://www.bluetack.co.uk/config/level1.gz", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_LEVEL1].push_back(ListData(L"http://peerguardian.sourceforge.net/lists/p2p.php", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_LEVEL1].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_level1", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_ADS].push_back(ListData(L"Advertising (Bluetack)"));
		Lists[LISTNAME_BT_ADS].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n"));
		Lists[LISTNAME_BT_ADS].push_back(ListData(L"http://list.iblocklist.com/?list=bt_ads", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_ADS].push_back(ListData(L"http://www.bluetack.co.uk/config/ads-trackers-and-bad-pr0n.gz", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_ADS].push_back(ListData(L"http://peerguardian.sourceforge.net/lists/ads.php", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_ADS].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_ads", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_SPY].push_back(ListData(L"Spyware (Bluetack)"));
		Lists[LISTNAME_BT_SPY].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/spyware"));
		Lists[LISTNAME_BT_SPY].push_back(ListData(L"http://list.iblocklist.com/?list=bt_spyware", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_SPY].push_back(ListData(L"http://www.bluetack.co.uk/config/spyware.gz", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_SPY].push_back(ListData(L"http://peerguardian.sourceforge.net/lists/spy.php", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_SPY].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_spyware", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_EDU].push_back(ListData(L"Education (Bluetack)"));
		Lists[LISTNAME_BT_EDU].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/edu"));
		Lists[LISTNAME_BT_EDU].push_back(ListData(L"http://list.iblocklist.com/?list=bt_edu", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_EDU].push_back(ListData(L"http://www.bluetack.co.uk/config/edu.gz", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_EDU].push_back(ListData(L"http://peerguardian.sourceforge.net/lists/edu.php", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_EDU].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_edu", LISTFLAG_WRONG));

		// Bluetack Lists

		Lists[LISTNAME_BT_LEVEL2].push_back(ListData(L"Level 2 (Bluetack)"));
		Lists[LISTNAME_BT_LEVEL2].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/level-2"));
		Lists[LISTNAME_BT_LEVEL2].push_back(ListData(L"http://list.iblocklist.com/?list=bt_level2", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_LEVEL2].push_back(ListData(L"http://www.bluetack.co.uk/config/level2.gz", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_LEVEL2].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_level2", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_LEVEL3].push_back(ListData(L"Level 3 (Bluetack)"));
		Lists[LISTNAME_BT_LEVEL3].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/level-3"));
		Lists[LISTNAME_BT_LEVEL3].push_back(ListData(L"http://list.iblocklist.com/?list=bt_level3", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_LEVEL3].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_level3", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_BOGON].push_back(ListData(L"Bogon (Bluetack)"));
		Lists[LISTNAME_BT_BOGON].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/bogon"));
		Lists[LISTNAME_BT_BOGON].push_back(ListData(L"http://list.iblocklist.com/?list=bt_bogon", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_BOGON].push_back(ListData(L"http://www.bluetack.co.uk/config/bogon.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_BOGON].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_bogon", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_DSHIELD].push_back(ListData(L"DShield (Bluetack)"));
		Lists[LISTNAME_BT_DSHIELD].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/dshield"));
		Lists[LISTNAME_BT_DSHIELD].push_back(ListData(L"http://list.iblocklist.com/?list=bt_dshield", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_DSHIELD].push_back(ListData(L"http://www.bluetack.co.uk/config/dshield.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_DSHIELD].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_dshield", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_HIJACKED].push_back(ListData(L"Hijacked (Bluetack)"));
		Lists[LISTNAME_BT_HIJACKED].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/hijacked"));
		Lists[LISTNAME_BT_HIJACKED].push_back(ListData(L"http://list.iblocklist.com/?list=bt_hijacked", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_HIJACKED].push_back(ListData(L"http://www.bluetack.co.uk/config/hijacked.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_HIJACKED].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_hijacked", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_MICROSOFT].push_back(ListData(L"Microsoft (Bluetack)"));
		Lists[LISTNAME_BT_MICROSOFT].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/microsoft"));
		Lists[LISTNAME_BT_MICROSOFT].push_back(ListData(L"http://list.iblocklist.com/?list=bt_microsoft", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_MICROSOFT].push_back(ListData(L"http://www.bluetack.co.uk/config/Microsoft.gz", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_MICROSOFT].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_microsoft", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_IANA_MULTICAST].push_back(ListData(L"IANA Multicast (Bluetack)"));
		Lists[LISTNAME_BT_IANA_MULTICAST].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/iana-multicast"));
		Lists[LISTNAME_BT_IANA_MULTICAST].push_back(ListData(L"http://list.iblocklist.com/?list=pwqnlynprfgtjbgqoizj", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_IANA_MULTICAST].push_back(ListData(L"http://www.bluetack.co.uk/config/iana-multicast.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_IANA_MULTICAST].push_back(ListData(L"http://iblocklist.com/list.php?list=pwqnlynprfgtjbgqoizj", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_IANA_PRIVATE].push_back(ListData(L"IANA Private (Bluetack)"));
		Lists[LISTNAME_BT_IANA_PRIVATE].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/iana-private"));
		Lists[LISTNAME_BT_IANA_PRIVATE].push_back(ListData(L"http://list.iblocklist.com/?list=cslpybexmxyuacbyuvib", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_IANA_PRIVATE].push_back(ListData(L"http://www.bluetack.co.uk/config/iana-private.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_IANA_PRIVATE].push_back(ListData(L"http://iblocklist.com/list.php?list=cslpybexmxyuacbyuvib", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_IANA_RESERVED].push_back(ListData(L"IANA Reserved (Bluetack)"));
		Lists[LISTNAME_BT_IANA_RESERVED].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/iana-reserved"));
		Lists[LISTNAME_BT_IANA_RESERVED].push_back(ListData(L"http://list.iblocklist.com/?list=bcoepfyewziejvcqyhqo", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_IANA_RESERVED].push_back(ListData(L"http://www.bluetack.co.uk/config/iana-reserved.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_IANA_RESERVED].push_back(ListData(L"http://iblocklist.com/list.php?list=bcoepfyewziejvcqyhqo", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_FORNONLAN].push_back(ListData(L"For Non-LAN Computers (Bluetack)"));
		Lists[LISTNAME_BT_FORNONLAN].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/for-non-lan-computers"));
		Lists[LISTNAME_BT_FORNONLAN].push_back(ListData(L"http://list.iblocklist.com/?list=jhaoawihmfxgnvmaqffp", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_FORNONLAN].push_back(ListData(L"http://www.bluetack.co.uk/config/fornonlancomputers.zip", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_BT_FORNONLAN].push_back(ListData(L"http://iblocklist.com/list.php?list=jhaoawihmfxgnvmaqffp", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_BADPEERS].push_back(ListData(L"Bad Peers (Bluetack)"));
		Lists[LISTNAME_BT_BADPEERS].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/bad-peers"));
		Lists[LISTNAME_BT_BADPEERS].push_back(ListData(L"http://list.iblocklist.com/?list=bt_templist", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_BADPEERS].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_templist", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_PROXY].push_back(ListData(L"Proxy (Bluetack)"));
		Lists[LISTNAME_BT_PROXY].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/proxy"));
		Lists[LISTNAME_BT_PROXY].push_back(ListData(L"http://list.iblocklist.com/?list=bt_proxy", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_PROXY].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_proxy", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_RANGETEST].push_back(ListData(L"Range Test (Bluetack)"));
		Lists[LISTNAME_BT_RANGETEST].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/range-test"));
		Lists[LISTNAME_BT_RANGETEST].push_back(ListData(L"http://list.iblocklist.com/?list=bt_rangetest", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_RANGETEST].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_rangetest", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_SPIDER].push_back(ListData(L"Spider (Bluetack)"));
		Lists[LISTNAME_BT_SPIDER].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/spider"));
		Lists[LISTNAME_BT_SPIDER].push_back(ListData(L"http://list.iblocklist.com/?list=bt_spider", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_SPIDER].push_back(ListData(L"http://iblocklist.com/list.php?list=bt_spider", LISTFLAG_WRONG));

		Lists[LISTNAME_BT_WEBEX_FSPAM].push_back(ListData(L"WebExploit ForumSpam (Bluetack)"));
		Lists[LISTNAME_BT_WEBEX_FSPAM].push_back(ListData(L"http://list.iblocklist.com/lists/bluetack/webexploit-forumspam"));
		Lists[LISTNAME_BT_WEBEX_FSPAM].push_back(ListData(L"http://list.iblocklist.com/?list=bimsvyvtgxeelunveyal", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_BT_WEBEX_FSPAM].push_back(ListData(L"http://iblocklist.com/list.php?list=bimsvyvtgxeelunveyal", LISTFLAG_WRONG));

		// DCHA Lists

		Lists[LISTNAME_DCHA_FAKER].push_back(ListData(L"Faker (DHCA)"));
		Lists[LISTNAME_DCHA_FAKER].push_back(ListData(L"http://list.iblocklist.com/lists/dchubad/faker"));
		Lists[LISTNAME_DCHA_FAKER].push_back(ListData(L"http://list.iblocklist.com/?list=dcha_faker", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_DCHA_FAKER].push_back(ListData(L"http://iblocklist.com/list.php?list=dcha_faker", LISTFLAG_WRONG));

		Lists[LISTNAME_DCHA_HACKER].push_back(ListData(L"Hacker (DHCA)"));
		Lists[LISTNAME_DCHA_HACKER].push_back(ListData(L"http://list.iblocklist.com/lists/dchubad/hacker"));
		Lists[LISTNAME_DCHA_HACKER].push_back(ListData(L"http://list.iblocklist.com/?list=dcha_hacker", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_DCHA_HACKER].push_back(ListData(L"http://iblocklist.com/list.php?list=dcha_hacker", LISTFLAG_WRONG));

		Lists[LISTNAME_DCHA_PEDO].push_back(ListData(L"Pedophiles (DHCA)"));
		Lists[LISTNAME_DCHA_PEDO].push_back(ListData(L"http://list.iblocklist.com/lists/dchubad/pedophiles"));
		Lists[LISTNAME_DCHA_PEDO].push_back(ListData(L"http://list.iblocklist.com/?list=dcha_pedophiles", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_DCHA_PEDO].push_back(ListData(L"http://iblocklist.com/list.php?list=dcha_pedophiles", LISTFLAG_WRONG));

		Lists[LISTNAME_DCHA_SPAMMER].push_back(ListData(L"Spammer (DHCA)"));
		Lists[LISTNAME_DCHA_SPAMMER].push_back(ListData(L"http://list.iblocklist.com/lists/dchubad/spammer"));
		Lists[LISTNAME_DCHA_SPAMMER].push_back(ListData(L"http://list.iblocklist.com/?list=dcha_spammer", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_DCHA_SPAMMER].push_back(ListData(L"http://iblocklist.com/list.php?list=dcha_spammer", LISTFLAG_WRONG));

		// TBG Lists

		Lists[LISTNAME_TBG_BOGON].push_back(ListData(L"Bogon (TBG)"));
		Lists[LISTNAME_TBG_BOGON].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/bogon"));
		Lists[LISTNAME_TBG_BOGON].push_back(ListData(L"http://list.iblocklist.com/?list=ewqglwibdgjttwttrinl", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_BOGON].push_back(ListData(L"http://iblocklist.com/list.php?list=ewqglwibdgjttwttrinl", LISTFLAG_WRONG));

		Lists[LISTNAME_TBG_BUSINESS_ISP].push_back(ListData(L"Business ISPs (TBG)"));
		Lists[LISTNAME_TBG_BUSINESS_ISP].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/business-isps"));
		Lists[LISTNAME_TBG_BUSINESS_ISP].push_back(ListData(L"http://list.iblocklist.com/?list=jcjfaxgyyshvdbceroxf", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_BUSINESS_ISP].push_back(ListData(L"http://iblocklist.com/list.php?list=jcjfaxgyyshvdbceroxf", LISTFLAG_WRONG));

		Lists[LISTNAME_TBG_EDU].push_back(ListData(L"Educational Institutions (TBG)"));
		Lists[LISTNAME_TBG_EDU].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/educational-institutions"));
		Lists[LISTNAME_TBG_EDU].push_back(ListData(L"http://list.iblocklist.com/?list=lljggjrpmefcwqknpalp", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_EDU].push_back(ListData(L"http://iblocklist.com/list.php?list=lljggjrpmefcwqknpalp", LISTFLAG_WRONG));

		Lists[LISTNAME_TBG_CORP].push_back(ListData(L"Corporate Ranges (TBG)"));
		Lists[LISTNAME_TBG_CORP].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/general-corporate-ranges"));
		Lists[LISTNAME_TBG_CORP].push_back(ListData(L"http://list.iblocklist.com/?list=ecqbsykllnadihkdirsh", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_CORP].push_back(ListData(L"http://iblocklist.com/list.php?list=ecqbsykllnadihkdirsh", LISTFLAG_WRONG));

		Lists[LISTNAME_TBG_HIJACKED].push_back(ListData(L"Hijacked (TBG)"));
		Lists[LISTNAME_TBG_HIJACKED].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/hijacked"));
		Lists[LISTNAME_TBG_HIJACKED].push_back(ListData(L"http://list.iblocklist.com/?list=tbnuqfclfkemqivekikv", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_HIJACKED].push_back(ListData(L"http://iblocklist.com/list.php?list=tbnuqfclfkemqivekikv", LISTFLAG_WRONG));

		Lists[LISTNAME_TBG_PTHREAT].push_back(ListData(L"Primary Threats (TBG)"));
		Lists[LISTNAME_TBG_PTHREAT].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/primary-threats"));
		Lists[LISTNAME_TBG_PTHREAT].push_back(ListData(L"http://list.iblocklist.com/?list=ijfqtofzixtwayqovmxn", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_PTHREAT].push_back(ListData(L"http://iblocklist.com/list.php?list=ijfqtofzixtwayqovmxn", LISTFLAG_WRONG));

		Lists[LISTNAME_TBG_SEARCHENG].push_back(ListData(L"Search Engines (TBG)"));
		Lists[LISTNAME_TBG_SEARCHENG].push_back(ListData(L"http://list.iblocklist.com/lists/tbg/search-engines"));
		Lists[LISTNAME_TBG_SEARCHENG].push_back(ListData(L"http://list.iblocklist.com/?list=pfefqteoxlfzopecdtyw", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_TBG_SEARCHENG].push_back(ListData(L"http://iblocklist.com/list.php?list=pfefqteoxlfzopecdtyw", LISTFLAG_WRONG));

		// Misc Lists

		Lists[LISTNAME_NXS_IPFILTERX].push_back(ListData(L"IpFilterX (Nexus23)"));
		Lists[LISTNAME_NXS_IPFILTERX].push_back(ListData(L"http://list.iblocklist.com/lists/nexus23/ipfilterx"));
		Lists[LISTNAME_NXS_IPFILTERX].push_back(ListData(L"http://list.iblocklist.com/?list=nxs23_ipfilterx", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_NXS_IPFILTERX].push_back(ListData(L"http://iblocklist.com/list.php?list=nxs23_ipfilterx", LISTFLAG_WRONG));

		Lists[LISTNAME_SPAMHAUS_DROP].push_back(ListData(L"Drop (SpamHaus)"));
		Lists[LISTNAME_SPAMHAUS_DROP].push_back(ListData(L"http://list.iblocklist.com/lists/spamhaus/drop"));
		Lists[LISTNAME_SPAMHAUS_DROP].push_back(ListData(L"http://list.iblocklist.com/?list=sh_drop", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_SPAMHAUS_DROP].push_back(ListData(L"http://iblocklist.com/list.php?list=sh_drop", LISTFLAG_WRONG));

		Lists[LISTNAME_ATMA].push_back(ListData(L"Atma (ATMA)"));
		Lists[LISTNAME_ATMA].push_back(ListData(L"http://list.iblocklist.com/lists/atma/atma"));
		Lists[LISTNAME_ATMA].push_back(ListData(L"http://list.iblocklist.com/?list=tzmtqbbsgbtfxainogvm", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_ATMA].push_back(ListData(L"http://iblocklist.com/list.php?list=tzmtqbbsgbtfxainogvm", LISTFLAG_WRONG));
		Lists[LISTNAME_ATMA].push_back(ListData(L"http://www.atma.es/atma.p2p", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_ATMA].push_back(ListData(L"http://www.ataca.me/atma.p2p", LISTFLAG_NOT_IBL));
		Lists[LISTNAME_ATMA].push_back(ListData(L"http://galinux.myftp.org/blacklist.p2p", LISTFLAG_NOT_IBL));

		Lists[LISTNAME_PB_RAPIDSHARE].push_back(ListData(L"RapidShare (PB)"));
		Lists[LISTNAME_PB_RAPIDSHARE].push_back(ListData(L"http://list.iblocklist.com/lists/peerblock/rapidshare"));
		Lists[LISTNAME_PB_RAPIDSHARE].push_back(ListData(L"http://list.iblocklist.com/?list=zfucwtjkfwkalytktyiw", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_PB_RAPIDSHARE].push_back(ListData(L"http://iblocklist.com/list.php?list=zfucwtjkfwkalytktyiw", LISTFLAG_WRONG));

		Lists[LISTNAME_CIDR_BOGON].push_back(ListData(L"Bogon (CIDR)"));
		Lists[LISTNAME_CIDR_BOGON].push_back(ListData(L"http://list.iblocklist.com/lists/cidr-report/bogon"));
		Lists[LISTNAME_CIDR_BOGON].push_back(ListData(L"http://list.iblocklist.com/?list=cr_bogon", LISTFLAG_UNFRIENDLY));
		Lists[LISTNAME_CIDR_BOGON].push_back(ListData(L"http://iblocklist.com/list.php?list=cr_bogon", LISTFLAG_WRONG));

		// iBlocklist Service Lists

		Lists[LISTNAME_IBL_LOGMEIN].push_back(ListData(L"LogMeIn (I-Blocklist)"));
		Lists[LISTNAME_IBL_LOGMEIN].push_back(ListData(L"http://list.iblocklist.com/?list=logmein"));
		Lists[LISTNAME_IBL_LOGMEIN].push_back(ListData(L"http://iblocklist.com/list.php?list=logmein", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_STEAM].push_back(ListData(L"Steam (I-Blocklist)"));
		Lists[LISTNAME_IBL_STEAM].push_back(ListData(L"http://list.iblocklist.com/?list=steam"));
		Lists[LISTNAME_IBL_STEAM].push_back(ListData(L"http://iblocklist.com/list.php?list=steam", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_XFIRE].push_back(ListData(L"Xfire (I-Blocklist)"));
		Lists[LISTNAME_IBL_XFIRE].push_back(ListData(L"http://list.iblocklist.com/?list=xfire"));
		Lists[LISTNAME_IBL_XFIRE].push_back(ListData(L"http://iblocklist.com/list.php?list=xfire", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_BLIZZARD].push_back(ListData(L"Blizzard (I-Blocklist)"));
		Lists[LISTNAME_IBL_BLIZZARD].push_back(ListData(L"http://list.iblocklist.com/?list=blizzard"));
		Lists[LISTNAME_IBL_BLIZZARD].push_back(ListData(L"http://iblocklist.com/list.php?list=blizzard", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_UBISOFT].push_back(ListData(L"Ubisoft (I-Blocklist)"));
		Lists[LISTNAME_IBL_UBISOFT].push_back(ListData(L"http://list.iblocklist.com/?list=ubisoft"));
		Lists[LISTNAME_IBL_UBISOFT].push_back(ListData(L"http://iblocklist.com/list.php?list=ubisoft", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_NINTENDO].push_back(ListData(L"Nintendo (I-Blocklist)"));
		Lists[LISTNAME_IBL_NINTENDO].push_back(ListData(L"http://list.iblocklist.com/?list=nintendo"));
		Lists[LISTNAME_IBL_NINTENDO].push_back(ListData(L"http://iblocklist.com/list.php?list=nintendo", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_ACTIVISION].push_back(ListData(L"Activision (I-Blocklist)"));
		Lists[LISTNAME_IBL_ACTIVISION].push_back(ListData(L"http://list.iblocklist.com/?list=activision"));
		Lists[LISTNAME_IBL_ACTIVISION].push_back(ListData(L"http://iblocklist.com/list.php?list=activision", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_SOE].push_back(ListData(L"Sony Online Entertainment (I-Blocklist)"));
		Lists[LISTNAME_IBL_SOE].push_back(ListData(L"http://list.iblocklist.com/?list=soe"));
		Lists[LISTNAME_IBL_SOE].push_back(ListData(L"http://iblocklist.com/list.php?list=soe", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_CCP].push_back(ListData(L"Crowd Control Productions (I-Blocklist)"));
		Lists[LISTNAME_IBL_CCP].push_back(ListData(L"http://list.iblocklist.com/?list=ccp"));
		Lists[LISTNAME_IBL_CCP].push_back(ListData(L"http://iblocklist.com/list.php?list=ccp", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_LINDENLAB].push_back(ListData(L"Linden Lab (I-Blocklist)"));
		Lists[LISTNAME_IBL_LINDENLAB].push_back(ListData(L"http://list.iblocklist.com/?list=lindenlab"));
		Lists[LISTNAME_IBL_LINDENLAB].push_back(ListData(L"http://iblocklist.com/list.php?list=lindenlab", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_EA].push_back(ListData(L"Electronic Arts (I-Blocklist)"));
		Lists[LISTNAME_IBL_EA].push_back(ListData(L"http://list.iblocklist.com/?list=electronicarts"));
		Lists[LISTNAME_IBL_EA].push_back(ListData(L"http://iblocklist.com/list.php?list=electronicarts", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_SQUAREENIX].push_back(ListData(L"Square Enix (I-Blocklist)"));
		Lists[LISTNAME_IBL_SQUAREENIX].push_back(ListData(L"http://list.iblocklist.com/?list=squareenix"));
		Lists[LISTNAME_IBL_SQUAREENIX].push_back(ListData(L"http://iblocklist.com/list.php?list=squareenix", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_NCSOFT].push_back(ListData(L"NCsoft (I-Blocklist)"));
		Lists[LISTNAME_IBL_NCSOFT].push_back(ListData(L"http://list.iblocklist.com/?list=ncsoft"));
		Lists[LISTNAME_IBL_NCSOFT].push_back(ListData(L"http://iblocklist.com/list.php?list=ncsoft", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_PUNKBUSTER].push_back(ListData(L"PunkBuster (I-Blocklist)"));
		Lists[LISTNAME_IBL_PUNKBUSTER].push_back(ListData(L"http://list.iblocklist.com/?list=punkbuster"));
		Lists[LISTNAME_IBL_PUNKBUSTER].push_back(ListData(L"http://iblocklist.com/list.php?list=punkbuster", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_JOOST].push_back(ListData(L"Joost (I-Blocklist)"));
		Lists[LISTNAME_IBL_JOOST].push_back(ListData(L"http://list.iblocklist.com/?list=joost"));
		Lists[LISTNAME_IBL_JOOST].push_back(ListData(L"http://iblocklist.com/list.php?list=joost", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_PANDORA].push_back(ListData(L"Pandora (I-Blocklist)"));
		Lists[LISTNAME_IBL_PANDORA].push_back(ListData(L"http://list.iblocklist.com/?list=aevzidimyvwybzkletsg"));
		Lists[LISTNAME_IBL_PANDORA].push_back(ListData(L"http://iblocklist.com/list.php?list=aevzidimyvwybzkletsg", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_PIRATEBAY].push_back(ListData(L"The Pirate Bay (I-Blocklist)"));
		Lists[LISTNAME_IBL_PIRATEBAY].push_back(ListData(L"http://list.iblocklist.com/?list=nzldzlpkgrcncdomnttb"));
		Lists[LISTNAME_IBL_PIRATEBAY].push_back(ListData(L"http://iblocklist.com/list.php?list=nzldzlpkgrcncdomnttb", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_APPLE].push_back(ListData(L"Apple (I-Blocklist)"));
		Lists[LISTNAME_IBL_APPLE].push_back(ListData(L"http://list.iblocklist.com/?list=aphcqvpxuqgrkgufjruj"));
		Lists[LISTNAME_IBL_APPLE].push_back(ListData(L"http://iblocklist.com/list.php?list=aphcqvpxuqgrkgufjruj", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_TOR].push_back(ListData(L"The Onion Router (I-Blocklist)"));
		Lists[LISTNAME_IBL_TOR].push_back(ListData(L"http://list.iblocklist.com/?list=tor"));
		Lists[LISTNAME_IBL_TOR].push_back(ListData(L"http://iblocklist.com/list.php?list=tor", LISTFLAG_WRONG));

		// iBlocklist ISP Lists

		Lists[LISTNAME_IBL_AOL].push_back(ListData(L"AOL (I-Blocklist)"));
		Lists[LISTNAME_IBL_AOL].push_back(ListData(L"http://list.iblocklist.com/?list=aol"));
		Lists[LISTNAME_IBL_AOL].push_back(ListData(L"http://iblocklist.com/list.php?list=aol", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_COMCAST].push_back(ListData(L"Comcast (I-Blocklist)"));
		Lists[LISTNAME_IBL_COMCAST].push_back(ListData(L"http://list.iblocklist.com/?list=comcast"));
		Lists[LISTNAME_IBL_COMCAST].push_back(ListData(L"http://iblocklist.com/list.php?list=comcast", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_CABLEVISION].push_back(ListData(L"Cablevision (I-Blocklist)"));
		Lists[LISTNAME_IBL_CABLEVISION].push_back(ListData(L"http://list.iblocklist.com/?list=cablevision"));
		Lists[LISTNAME_IBL_CABLEVISION].push_back(ListData(L"http://iblocklist.com/list.php?list=cablevision", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_VERIZON].push_back(ListData(L"Verizon (I-Blocklist)"));
		Lists[LISTNAME_IBL_VERIZON].push_back(ListData(L"http://list.iblocklist.com/?list=verizon"));
		Lists[LISTNAME_IBL_VERIZON].push_back(ListData(L"http://iblocklist.com/list.php?list=verizon", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_ATT].push_back(ListData(L"AT&T (I-Blocklist)"));
		Lists[LISTNAME_IBL_ATT].push_back(ListData(L"http://list.iblocklist.com/?list=att"));
		Lists[LISTNAME_IBL_ATT].push_back(ListData(L"http://iblocklist.com/list.php?list=att", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_COX].push_back(ListData(L"Cox (I-Blocklist)"));
		Lists[LISTNAME_IBL_COX].push_back(ListData(L"http://list.iblocklist.com/?list=cox"));
		Lists[LISTNAME_IBL_COX].push_back(ListData(L"http://iblocklist.com/list.php?list=cox", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_ROADRUNNER].push_back(ListData(L"Road Runner (I-Blocklist)"));
		Lists[LISTNAME_IBL_ROADRUNNER].push_back(ListData(L"http://list.iblocklist.com/?list=roadrunner"));
		Lists[LISTNAME_IBL_ROADRUNNER].push_back(ListData(L"http://iblocklist.com/list.php?list=roadrunner", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_CHARTER].push_back(ListData(L"Charter (I-Blocklist)"));
		Lists[LISTNAME_IBL_CHARTER].push_back(ListData(L"http://list.iblocklist.com/?list=charter"));
		Lists[LISTNAME_IBL_CHARTER].push_back(ListData(L"http://iblocklist.com/list.php?list=charter", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_QWEST].push_back(ListData(L"Qwest (I-Blocklist)"));
		Lists[LISTNAME_IBL_QWEST].push_back(ListData(L"http://list.iblocklist.com/?list=qwest"));
		Lists[LISTNAME_IBL_QWEST].push_back(ListData(L"http://iblocklist.com/list.php?list=qwest", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_EMBARQ].push_back(ListData(L"Embarq (I-Blocklist)"));
		Lists[LISTNAME_IBL_EMBARQ].push_back(ListData(L"http://list.iblocklist.com/?list=embarq"));
		Lists[LISTNAME_IBL_EMBARQ].push_back(ListData(L"http://iblocklist.com/list.php?list=embarq", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_SUDDENLINK].push_back(ListData(L"Suddenlink (I-Blocklist)"));
		Lists[LISTNAME_IBL_SUDDENLINK].push_back(ListData(L"http://list.iblocklist.com/?list=suddenlink"));
		Lists[LISTNAME_IBL_SUDDENLINK].push_back(ListData(L"http://iblocklist.com/list.php?list=suddenlink", LISTFLAG_WRONG));

		// iBlocklist Country Lists

		Lists[LISTNAME_IBL_AUSTRALIA].push_back(ListData(L"Australia (I-Blocklist)"));
		Lists[LISTNAME_IBL_AUSTRALIA].push_back(ListData(L"http://list.iblocklist.com/?list=au"));
		Lists[LISTNAME_IBL_AUSTRALIA].push_back(ListData(L"http://iblocklist.com/list.php?list=au", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_BRAZIL].push_back(ListData(L"Brazil (I-Blocklist)"));
		Lists[LISTNAME_IBL_BRAZIL].push_back(ListData(L"http://list.iblocklist.com/?list=br"));
		Lists[LISTNAME_IBL_BRAZIL].push_back(ListData(L"http://iblocklist.com/list.php?list=br", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_CANADA].push_back(ListData(L"Canada (I-Blocklist)"));
		Lists[LISTNAME_IBL_CANADA].push_back(ListData(L"http://list.iblocklist.com/?list=ca"));
		Lists[LISTNAME_IBL_CANADA].push_back(ListData(L"http://iblocklist.com/list.php?list=ca", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_CHINA].push_back(ListData(L"China (I-Blocklist)"));
		Lists[LISTNAME_IBL_CHINA].push_back(ListData(L"http://list.iblocklist.com/?list=cn"));
		Lists[LISTNAME_IBL_CHINA].push_back(ListData(L"http://iblocklist.com/list.php?list=cn", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_GERMANY].push_back(ListData(L"Germany (I-Blocklist)"));
		Lists[LISTNAME_IBL_GERMANY].push_back(ListData(L"http://list.iblocklist.com/?list=de"));
		Lists[LISTNAME_IBL_GERMANY].push_back(ListData(L"http://iblocklist.com/list.php?list=de", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_SPAIN].push_back(ListData(L"Spain (I-Blocklist)"));
		Lists[LISTNAME_IBL_SPAIN].push_back(ListData(L"http://list.iblocklist.com/?list=es"));
		Lists[LISTNAME_IBL_SPAIN].push_back(ListData(L"http://iblocklist.com/list.php?list=es", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_EU].push_back(ListData(L"European Union (I-Blocklist)"));
		Lists[LISTNAME_IBL_EU].push_back(ListData(L"http://list.iblocklist.com/?list=eu"));
		Lists[LISTNAME_IBL_EU].push_back(ListData(L"http://iblocklist.com/list.php?list=eu", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_FRANCE].push_back(ListData(L"France (I-Blocklist)"));
		Lists[LISTNAME_IBL_FRANCE].push_back(ListData(L"http://list.iblocklist.com/?list=fr"));
		Lists[LISTNAME_IBL_FRANCE].push_back(ListData(L"http://iblocklist.com/list.php?list=fr", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_UK].push_back(ListData(L"United Kingdom (I-Blocklist)"));
		Lists[LISTNAME_IBL_UK].push_back(ListData(L"http://list.iblocklist.com/?list=uk"));
		Lists[LISTNAME_IBL_UK].push_back(ListData(L"http://iblocklist.com/list.php?list=uk", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_ITALY].push_back(ListData(L"Italy (I-Blocklist)"));
		Lists[LISTNAME_IBL_ITALY].push_back(ListData(L"http://list.iblocklist.com/?list=it"));
		Lists[LISTNAME_IBL_ITALY].push_back(ListData(L"http://iblocklist.com/list.php?list=it", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_JAPAN].push_back(ListData(L"Japan (I-Blocklist)"));
		Lists[LISTNAME_IBL_JAPAN].push_back(ListData(L"http://list.iblocklist.com/?list=jp"));
		Lists[LISTNAME_IBL_JAPAN].push_back(ListData(L"http://iblocklist.com/list.php?list=jp", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_SKOREA].push_back(ListData(L"Republic of Korea (I-Blocklist)"));
		Lists[LISTNAME_IBL_SKOREA].push_back(ListData(L"http://list.iblocklist.com/?list=kr"));
		Lists[LISTNAME_IBL_SKOREA].push_back(ListData(L"http://iblocklist.com/list.php?list=kr", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_MEXICO].push_back(ListData(L"Mexico (I-Blocklist)"));
		Lists[LISTNAME_IBL_MEXICO].push_back(ListData(L"http://list.iblocklist.com/?list=mx"));
		Lists[LISTNAME_IBL_MEXICO].push_back(ListData(L"http://iblocklist.com/list.php?list=mx", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_NETHERLANDS].push_back(ListData(L"Netherlands (I-Blocklist)"));
		Lists[LISTNAME_IBL_NETHERLANDS].push_back(ListData(L"http://list.iblocklist.com/?list=nl"));
		Lists[LISTNAME_IBL_NETHERLANDS].push_back(ListData(L"http://iblocklist.com/list.php?list=nl", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_RUSSIA].push_back(ListData(L"Russia (I-Blocklist)"));
		Lists[LISTNAME_IBL_RUSSIA].push_back(ListData(L"http://list.iblocklist.com/?list=ru"));
		Lists[LISTNAME_IBL_RUSSIA].push_back(ListData(L"http://iblocklist.com/list.php?list=ru", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_SWEDEN].push_back(ListData(L"Sweden (I-Blocklist)"));
		Lists[LISTNAME_IBL_SWEDEN].push_back(ListData(L"http://list.iblocklist.com/?list=se"));
		Lists[LISTNAME_IBL_SWEDEN].push_back(ListData(L"http://iblocklist.com/list.php?list=se", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_TAIWAN].push_back(ListData(L"Taiwan (I-Blocklist)"));
		Lists[LISTNAME_IBL_TAIWAN].push_back(ListData(L"http://list.iblocklist.com/?list=tw"));
		Lists[LISTNAME_IBL_TAIWAN].push_back(ListData(L"http://iblocklist.com/list.php?list=tw", LISTFLAG_WRONG));

		Lists[LISTNAME_IBL_US].push_back(ListData(L"United States (I-Blocklist)"));
		Lists[LISTNAME_IBL_US].push_back(ListData(L"http://list.iblocklist.com/?list=us"));
		Lists[LISTNAME_IBL_US].push_back(ListData(L"http://iblocklist.com/list.php?list=us", LISTFLAG_WRONG));

	}
	catch(...)
	{
		TRACEE("[ListUrls] [Init]  * ERROR: Exception caught while searching through lists");
		TRACEI("[ListUrls] [Init]  < Leaving routine (false).");
		return false;
	}

	TRACEI("[ListUrls] [Init]  < Leaving routine (true).");
	return true;

} // End of Init()


