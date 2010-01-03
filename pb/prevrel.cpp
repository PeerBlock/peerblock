/*
	Copyright (C) 2009 PeerBlock, LLC

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
		return;

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

	if (prevRelease < 237)
	{
		TRACEW("[mainproc] [PerformPrevRelUpdates]    Checking for old-URL lists, and migrating them to the new naming scheme");
		MessageBox(_hwnd, IDS_PREVREL237TEXT, IDS_PREVREL, MB_ICONINFORMATION|MB_OK);

		bool bOldUrlFound = false;
		vector<DynamicList> tempList;

		// check each list in configured lists
		for(vector<DynamicList>::size_type i = 0; i < g_config.DynamicLists.size(); ++i)
		{
			// if it's an old-style list
			DynamicList *list = &(g_config.DynamicLists[i]);	

			if (list->Url.find(_T("http://list.iblocklist.com/?list=bt_ads")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt_ads");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://list.iblocklist.com/?list=bt_edu")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt_edu");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/edu");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://list.iblocklist.com/?list=bt_level1")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt_level1");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/level-1");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://list.iblocklist.com/?list=bt_spyware")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt_spyware");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/spyware");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://list.iblocklist.com/?list=bt_level2")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt_level2");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/level-2");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/ads-trackers-and-bad-pr0n.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt ads-trackers-and-bad-pr0n");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/level1.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt level1");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/level-1");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/level2.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt level2");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/level-2");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/bogon.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt bogon");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/bogon");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/dshield.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt dshield");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/dshield");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/edu.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt edu");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/edu");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/iana-multicast.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt iana-multicast");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/iana-multicast");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/iana-private.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt iana-private");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/iana-private");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/iana-reserved.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt iana-reserved");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/iana-reserved");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/Microsoft.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt Microsoft");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/microsoft");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/fornonlancomputers.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt fornonlancomputers");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/for-non-lan-computers");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/spider.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt spider");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/spider");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/spyware.gz")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - migrating bt spyware");
				DynamicList newList = *list;
				newList.Url = _T("http://list.iblocklist.com/lists/bluetack/spyware");
				tempList.push_back(newList);
			}
			else if (list->Url.find(_T("http://www.bluetack.co.uk/config/trojan.zip")) != string::npos)
			{
				TRACEW("[mainproc] [PerformPrevRelUpdates]    - removing bt trojan");
			}
			else
			{
				TRACED("[mainproc] [PerformPrevRelUpdates]    found non-old URL");
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

}; // End of PerformPrevRelUpdates()



