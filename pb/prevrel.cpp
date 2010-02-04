/*
	Copyright (C) 2009-2010 PeerBlock, LLC

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
#include "resource.h"

#include "ListUrls.h"


//================================================================================================
//
//  PerformPrevRelUpdates()
//
//    - Called by Main_OnInitDialog()
//
/// <summary>
///   Checks the version of the last time we ran, and if that number falls within certain ranges
///	  we'll do some release-specific cleanup.
/// </summary>
//
void PerformPrevRelUpdates(HWND _hwnd)
{
	int prevRelease = g_config.LastVersionRun;

	if (prevRelease == PB_VER_BUILDNUM)
	{
		TRACEW("[mainproc] [PerformPrevRelUpdates]    no version change, so no updates to perform");
		return;
	}

	if (prevRelease > PB_VER_BUILDNUM)
	{
		TRACEW("[mainproc] [PerformPrevRelUpdates]    WARNING:  Downgrade detected!");
		return;
	}


	//--------------------------------------------------
	// Update PG hosted lists to iblocklist

	if (prevRelease < 134)
	{
		TRACEW("[mainproc] [PerformPrevRelUpdates]    Checking for old peerguardian-hosted lists, and updating any found to iblocklist.com-hosted ones");

		bool bOldUrlFound = false;
		vector<DynamicList> tempList;

		// check each list in configured lists
		for(vector<DynamicList>::size_type i = 0; i < g_config.DynamicLists.size(); ++i)
		{
			// if it's a peerguardian list
			DynamicList *list = &(g_config.DynamicLists[i]);	
			if (list->Url.find(_T("http://peerguardian.sourceforge.net/lists/")) != string::npos)
			{
				// swap it out
				tstring strBuf = boost::str(tformat(_T("[mainproc] [PerformPrevRelUpdates]    found old URL: [%1%]")) % list->Url );
				TRACEBUFW(strBuf);
				bOldUrlFound = true;

				if (list->Url.find(_T("ads.php")) != string::npos)
				{
					// http://list.iblocklist.com/?list=bt_ads
					TRACEW("[mainproc] [PerformPrevRelUpdates]    - replacing ads.php list with bt_ads");
					//list->Url = _T("http://list.iblocklist.com/?list=bt_ads");
					DynamicList newList = *list;
					newList.Url = _T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n");
					tempList.push_back(newList);
				}
				else if (list->Url.find(_T("edu.php")) != string::npos)
				{
					// http://list.iblocklist.com/?list=bt_edu
					TRACEW("[mainproc] [PerformPrevRelUpdates]    - replacing edu.php list with bt_edu");
					//list->Url = _T("http://list.iblocklist.com/?list=bt_edu");
					DynamicList newList = *list;
					newList.Url = _T("http://list.iblocklist.com/lists/bluetack/edu");
					tempList.push_back(newList);
				}
				else if (list->Url.find(_T("p2p.php")) != string::npos)
				{
					// http://list.iblocklist.com/?list=bt_level1
					TRACEW("[mainproc] [PerformPrevRelUpdates]    - replacing p2p.php list with bt_level1");
					//list->Url = _T("http://list.iblocklist.com/?list=bt_level1");
					DynamicList newList = *list;
					newList.Url = _T("http://list.iblocklist.com/lists/bluetack/level-1");
					tempList.push_back(newList);
				}
				else if (list->Url.find(_T("spy.php")) != string::npos)
				{
					// http://list.iblocklist.com/?list=bt_spyware
					TRACEW("[mainproc] [PerformPrevRelUpdates]    - replacing spy.php list with bt_spyware");
					//list->Url = _T("http://list.iblocklist.com/?list=bt_spyware");
					DynamicList newList = *list;
					newList.Url = _T("http://list.iblocklist.com/lists/bluetack/spyware");
					tempList.push_back(newList);
				}
				else if (list->Url.find(_T("gov.php")) != string::npos)
				{
					// remove list
					TRACEW("[mainproc] [PerformPrevRelUpdates]    - removing gov list");
				}
				else
				{
					TRACEE("[mainproc] [PerformPrevRelUpdates]    ERROR:  Unknown PG2 list!!");
				}
			}
			else
			{
				TRACED("[mainproc] [PerformPrevRelUpdates]    found non-PG2 URL");
				DynamicList newList = *list;
				tempList.push_back(newList);
			}
		}

		// Rebuild list if we need to remove Gov list.  Also, check for duplicates.
		g_config.DynamicLists.clear();
		for(vector<DynamicList>::size_type i = 0; i < tempList.size(); ++i)
		{
			if (std::find(g_config.DynamicLists.begin(), g_config.DynamicLists.end(), tempList[i]) == g_config.DynamicLists.end())
			{
				g_config.DynamicLists.push_back(tempList[i]);
			}
		}
	}


	//--------------------------------------------------
	// Update old list-names to new ones

	// Now that iblocklist.com has support for some "friendly" URLs for lists, we can migrate people
	// away from the slow, unreliable bluetack.co.uk servers and over to iblocklist.  We're checking
	// against any of the bluetack URLs from our old dropdown list, and changing 'em over.  Same 
	// thing for any of the old naming-scheme iblocklist URLs.  We're also removing the bluetack
	// "trojan" list, since it resolves to an unusable file and there is no iblocklist equivalent.

	if (prevRelease < 268)
	{
		TRACEW("[mainproc] [PerformPrevRelUpdates]    Checking for old-URL lists, and migrating them to the new naming scheme (r268)");
		int result = MessageBox(_hwnd, IDS_PREVREL268TEXT, IDS_PREVREL, MB_ICONINFORMATION|MB_YESNO);
		if (result == IDNO)
		{
			TRACEI("[mainproc] [PerformPrevRelUpdates]    user clicked No");
		}
		else if (result == IDYES)
		{
			TRACEI("[mainproc] [PerformPrevRelUpdates]    user clicked Yes");

			bool bOldUrlFound = false;
			vector<DynamicList> tempList;
			ListUrls listUrls;
			listUrls.Init();

			// check each list in configured lists
			for(vector<DynamicList>::size_type i = 0; i < g_config.DynamicLists.size(); ++i)
			{
				DynamicList *list = &(g_config.DynamicLists[i]);
				LISTNAME listId = listUrls.FindListNum(list->Url);
				if (listId != LISTNAME_COUNT)
				{
					DynamicList newList = *list;
					newList.Url = listUrls.GetBestUrl(listId);
					tempList.push_back(newList);
					if (newList.Url.find(list->Url) != string::npos)
					{
						tstring strBuf = boost::str(tformat(_T("[mainproc] [PerformPrevRelUpdates]    - found no better list url than [%1%]")) 
							% list->Url.c_str() );
						TRACEBUFW(strBuf);
					}
					else
					{
						tstring strBuf = boost::str(tformat(_T("[mainproc] [PerformPrevRelUpdates]    - migrated list from [%1%] to [%2%]")) 
							% list->Url.c_str() % newList.Url.c_str() );
						TRACEBUFW(strBuf);
					}
				}
				else if (list->Url.find(_T("http://www.bluetack.co.uk/config/trojan.zip")) != string::npos)
				{
					TRACEW("[mainproc] [PerformPrevRelUpdates]    - removing no-longer-existant bt trojan");
				}
				else
				{
					tstring strBuf = boost::str(tformat(_T("[mainproc] [PerformPrevRelUpdates]    - found unknown list url [%1%], copying over as-is")) 
						% list->Url.c_str() );
					TRACEBUFW(strBuf);
					DynamicList newList = *list;
					tempList.push_back(newList);
				}
			}

			// Rebuild list, checking for duplicates.
			g_config.DynamicLists.clear();
			for(vector<DynamicList>::size_type i = 0; i < tempList.size(); ++i)
			{
				if (std::find(g_config.DynamicLists.begin(), g_config.DynamicLists.end(), tempList[i]) == g_config.DynamicLists.end())
				{
					g_config.DynamicLists.push_back(tempList[i]);
				}
			}
		}
	}

}; // End of PerformPrevRelUpdates()



