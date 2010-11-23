//================================================================================================
//  listurls.h
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

#pragma once

using namespace std;


// Lets us refer to a list by ID, covering all known URL permutations
typedef enum LISTNAME
{
	LISTNAME_BT_LEVEL1,
	LISTNAME_BT_ADS,
	LISTNAME_BT_SPY,
	LISTNAME_BT_EDU,
	LISTNAME_MAX_DEFAULT = LISTNAME_BT_EDU,	// Used by caller to easily verify Default List or Not
	LISTNAME_BT_LEVEL2,
	LISTNAME_BT_LEVEL3,
	LISTNAME_BT_BOGON,
	LISTNAME_BT_DSHIELD,
	LISTNAME_BT_HIJACKED,
	LISTNAME_BT_MICROSOFT,
	LISTNAME_BT_IANA_MULTICAST,
	LISTNAME_BT_IANA_PRIVATE,
	LISTNAME_BT_IANA_RESERVED,
	LISTNAME_BT_FORNONLAN,
	LISTNAME_BT_BADPEERS,
	LISTNAME_BT_PROXY,
	LISTNAME_BT_RANGETEST,
	LISTNAME_BT_SPIDER,
	LISTNAME_BT_WEBEX_FSPAM,
	LISTNAME_BT_WEBEXPLOIT,
	LISTNAME_BT_FORUMSPAM,
	LISTNAME_DCHA_FAKER,
	LISTNAME_DCHA_HACKER,
	LISTNAME_DCHA_PEDO,
	LISTNAME_DCHA_SPAMMER,
	LISTNAME_NXS_IPFILTERX,
	LISTNAME_SPAMHAUS_DROP,
	LISTNAME_TBG_BOGON,
	LISTNAME_TBG_BUSINESS_ISP,
	LISTNAME_TBG_EDU,
	LISTNAME_TBG_CORP,
	LISTNAME_TBG_HIJACKED,
	LISTNAME_TBG_PTHREAT,
	LISTNAME_TBG_SEARCHENG,
	LISTNAME_ATMA,
	LISTNAME_ZEUS,
	LISTNAME_PB_RAPIDSHARE,
	LISTNAME_CIDR_BOGON,
	LISTNAME_CW_BOGON,
	LISTNAME_IBL_LOGMEIN,
	LISTNAME_IBL_STEAM,
	LISTNAME_IBL_XFIRE,
	LISTNAME_IBL_BLIZZARD,
	LISTNAME_IBL_UBISOFT,
	LISTNAME_IBL_NINTENDO,
	LISTNAME_IBL_ACTIVISION,
	LISTNAME_IBL_SOE,
	LISTNAME_IBL_CCP,
	LISTNAME_IBL_LINDENLAB,
	LISTNAME_IBL_EA,
	LISTNAME_IBL_SQUAREENIX,
	LISTNAME_IBL_NCSOFT,
	LISTNAME_IBL_PUNKBUSTER,
	LISTNAME_IBL_JOOST,
	LISTNAME_IBL_PANDORA,
	LISTNAME_IBL_PIRATEBAY,
	LISTNAME_IBL_APPLE,
	LISTNAME_IBL_TOR,
	LISTNAME_IBL_AOL,
	LISTNAME_IBL_COMCAST,
	LISTNAME_IBL_CABLEVISION,
	LISTNAME_IBL_VERIZON,
	LISTNAME_IBL_ATT,
	LISTNAME_IBL_COX,
	LISTNAME_IBL_ROADRUNNER,
	LISTNAME_IBL_CHARTER,
	LISTNAME_IBL_QWEST,
	LISTNAME_IBL_EMBARQ,
	LISTNAME_IBL_SUDDENLINK,
	LISTNAME_IBL_AUSTRALIA,
	LISTNAME_IBL_BRAZIL,
	LISTNAME_IBL_CANADA,
	LISTNAME_IBL_CHINA,
	LISTNAME_IBL_GERMANY,
	LISTNAME_IBL_SPAIN,
	LISTNAME_IBL_EU,
	LISTNAME_IBL_FRANCE,
	LISTNAME_IBL_UK,
	LISTNAME_IBL_ITALY,
	LISTNAME_IBL_JAPAN,
	LISTNAME_IBL_SKOREA,
	LISTNAME_IBL_MEXICO,
	LISTNAME_IBL_NETHERLANDS,
	LISTNAME_IBL_RUSSIA,
	LISTNAME_IBL_SWEDEN,
	LISTNAME_IBL_TAIWAN,
	LISTNAME_IBL_US,
	LISTNAME_COUNT
};


// Defines the order in which fields are present in the appropriate row of the url list
typedef enum LISTS_FIELD
{
	LISTS_FIELD_DESC,		// Default description of the list
	LISTS_FIELD_BESTURL		// The "Friendly" format iBlocklist URL, for most lists
};


// Flags used for sanity-checking purposes
typedef enum LISTFLAG
{
	LISTFLAG_NOT_IBL,		// Not an iblocklist.com-hosted list
	LISTFLAG_DEFAULT,		// One of the default lists
	LISTFLAG_UNFRIENDLY,	// Not in "friendly" iblocklist naming form
	LISTFLAG_WRONG,			// Known-wrong URL for this list; e.g. iblocklist "description URL"
	LISTFLAG_EXACTDUPE,		// List has already been added to config
	LISTFLAG_DIFFDUPE,		// List has already been added to config, under different URL
	LISTFLAG_COUNT
};

typedef bitset<LISTFLAG_COUNT> LISTFLAGS;


// Used to store a mapping of URL to flags, together as one entry in the Lists table
class ListData
{
public:
	wstring Url;
	LISTFLAGS Flags;

	ListData(wstring _url, LISTFLAG _flag)
	{
		Url = _url;
		Flags.set(_flag);
	};

	ListData(wstring _url)
	{
		Url = _url;
		Flags.reset();
	};

};


class ListUrls 
{

private:
	vector<vector<ListData>> Lists;

public:
	bool Init();
	wstring GetBestUrl(LISTNAME url_num) { return(Lists[url_num][LISTS_FIELD_BESTURL].Url); }; 
	LISTNAME FindListNum(wstring url);
	LISTFLAGS CheckUrl(wstring url, LISTNAME num=LISTNAME_COUNT, HWND listman=NULL);
	wstring GetListDesc(LISTNAME url_num);
	
};


// Expected Usage:
// - AddList_OnInitDialog, the window that pops up when you're adding a new list, creates an 
//   instance of this class
// - Once you select a list from the dropdown list, it prepopulates the Description field (if 
//   nothing previously user-entered)
// - After clicking OK, run through CheckUrl(), display message as appropriate per return flags
// - AddList_proc reponsible for checking for duplicates
//   - Loop through each currently added list to get it's ListNum (FindListNum()), and compare this
//     against the newly-user-added list
// - Offer to auto-correct issues, by using the LISTS_FIELD_BESTURL
// - ListUrls class instance destroyed along with AddList window as this is likely an infrequent 
//   task so we shouldn't need to keep this stuff in memory all the time

// Possible Errors:
// - one of the default lists
// - unfriendly iblocklist url
// - not iblocklist url
// - wrong url (i.e. desc-url, not update-url)
// - list already added, maybe under a different URL


