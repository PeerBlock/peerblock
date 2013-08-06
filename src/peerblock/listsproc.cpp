//================================================================================================
//  listsproc.cpp
//
//  This file contains all the routines used to handle the List Manager window.
//================================================================================================

/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2011 PeerBlock, LLC

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
using namespace std;


static INT_PTR g_ret;
static const UINT ID_CONTEXT_MAKESTATIC=200;
vector<DynamicList> g_deflists; // temporary storage of default lists

HWND g_hListsDlg = NULL;

//------------------------------------------------------------
// Internal routines.  If this were a class, these routines would be marked as Private.

static void AddToDefLists(tstring _url, tstring _desc, List::ListType _type)
{
	tstring strBuf = boost::str(tformat(_T("[listsproc] [AddToDefLists]    adding url:[%1%] desc:[%2%]")) % _url % _desc );
	TRACEBUFD(strBuf);

	DynamicList *list = new DynamicList;
	list->Url = _url;
	list->Description = _desc;
	list->Type = _type;
	list->Enabled = true;

	g_deflists.push_back(*list);
	g_ret|=LISTS_NEEDUPDATE;

} // End of AddToDefLists()


static void RemoveFromDefLists(tstring _url)
{
	tstring strBuf = boost::str(tformat(_T("[listsproc] [AddToDefLists]    removing url:[%1%]")) % _url );
	TRACEBUFD(strBuf);

	vector<DynamicList>::iterator invalid;
	invalid = remove(g_deflists.begin(), g_deflists.end(), _url);
	g_deflists.erase(invalid, g_deflists.end());
	g_ret|=LISTS_NEEDRELOAD;

} // End of RemoveFromDefLists()



static vector<DynamicList>::size_type FindInDefLists(tstring _url)
{
	tstring strBuf = boost::str(tformat(_T("[listsproc] [FindInDefLists]    finding url:[%1%]")) % _url );
	TRACEBUFV(strBuf);

	for(vector<DynamicList>::size_type i=0; i<g_deflists.size(); i++)
	{
		strBuf = boost::str(tformat(_T("[listsproc] [FindInDefLists]    checking def[%1%] url:[%2%]")) % i % g_deflists[i].Url);
		TRACEBUFV(strBuf);
		if (g_deflists[i].Url.compare(_url) == 0)
		{
			strBuf = boost::str(tformat(_T("[listsproc] [FindInDefLists]    found def[%1%] url:[%2%]")) % i % g_deflists[i].Url);
			TRACEBUFV(strBuf);
			return i;
		}
	}

	return -1;

} // End of FindInDefLists()



//================================================================================================
//
//  Lists_OnClose()
//
//    - Called by Lists_DlgProc()
//
/// <summary>
///   Closes the List Manager window.
/// </summary>
//
static void Lists_OnClose(HWND hwnd)
{
	TRACEI("[listsproc] [Lists_OnClose]  > Entering routine.");

	HWND list=GetDlgItem(hwnd, IDC_LIST);
	int count=ListView_GetItemCount(list);

	for(int i=0; i<count; i++)
	{
		LVITEM lvi={0};
		lvi.mask=LVIF_PARAM;
		lvi.iItem=i;
		ListView_GetItem(list, &lvi);

		List *l=(List*)lvi.lParam;

		bool checked=(ListView_GetCheckState(list, i)!=0);

		if(l->Enabled!=checked)
		{
			l->Enabled=checked;
			g_ret|=LISTS_NEEDRELOAD;
		}
	}

	EndDialog(hwnd, g_ret);
	TRACEI("[listsproc] [Lists_OnClose]  < Leaving routine.");

} // End of Lists_OnClose()



//================================================================================================
//
//  InsertItem(*plist)
//
//    - Called by lots of people
//
/// <summary>
///   Inserts a list into our Lists ListView.
/// </summary>
/// <remarks>
///   Note that there are two InsertItem routines.  This one takes a List pointer as the third arg.
/// </remarks>
//
static void InsertItem(HWND list, int index, List *plist)
{
	LVITEM lvi={0};
	lvi.mask=LVIF_TEXT|LVIF_PARAM;
	lvi.iItem=index;

	lvi.iSubItem=0;
	lvi.lParam=(LPARAM)plist;

	if(StaticList *l=dynamic_cast<StaticList*>(plist))
		lvi.pszText=(LPTSTR)l->File.c_str();
	else if(DynamicList *l=dynamic_cast<DynamicList*>(plist))
		lvi.pszText=(LPTSTR)l->Url.c_str();

	ListView_InsertItem(list, &lvi);

	lvi.mask=LVIF_TEXT;

	tstring buf=LoadString((plist->Type==List::Allow)?IDS_ALLOW:IDS_BLOCK);

	lvi.iSubItem=1;
	lvi.pszText=(LPTSTR)buf.c_str();
	ListView_SetItem(list, &lvi);

	lvi.iSubItem=2;
	lvi.pszText=(LPTSTR)plist->Description.c_str();
	ListView_SetItem(list, &lvi);

	ListView_SetCheckState(list, index, plist->Enabled?TRUE:FALSE);

} // End of InsertItem()



//================================================================================================
//
//  InsertItem(&plist)
//
//    - Called by lots of people
//
/// <summary>
///   Inserts a list into our Lists ListView.
/// </summary>
/// <remarks>
///   Note that there are two InsertItem routines.  This one takes a List ref as the third arg.
/// </remarks>
//
static void InsertItem(HWND list, int index, List &plist)
{
	List *nlist = {0};

	if(StaticList *l=dynamic_cast<StaticList*>(&plist))
		nlist=new StaticList(*l);
	else if(DynamicList *l=dynamic_cast<DynamicList*>(&plist))
		nlist=new DynamicList(*l);

	InsertItem(list, index, nlist);

} // End of InsertItem()



//================================================================================================
//
//  InsertColumn()
//
//    - Called from Lists_OnInitDialog()
//
/// <summary>
///   Inserts a new column into our Lists ListView.
/// </summary>
//
static void InsertColumn(HWND hList, INT iSubItem, INT iWidth, UINT idText)
{
	LVCOLUMN lvc={0};

	lvc.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
	lvc.fmt=LVCFMT_LEFT;
	lvc.cx=iWidth;
	lvc.iSubItem=iSubItem;

	tstring buf=LoadString(idText);
	lvc.pszText=(LPTSTR)buf.c_str();

	ListView_InsertColumn(hList, iSubItem, &lvc);

} // End of InsertColumn()



//================================================================================================
//
//  Lists_OnCommand()
//
//    - Called by Lists_DlgProc()
//
/// <summary>
///   Handles clicks on buttons/items on List Manager window.
/// </summary>
//
static void Lists_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id)
	{
		case IDC_P2PLIST:
		{
			TRACED("[listsproc] [Lists_OnCommand]    clicked on IDC_P2PLIST");
			if (IsDlgButtonChecked(hwnd, IDC_P2PLIST)==BST_CHECKED)
			{
				TRACEI("[listsproc] [Lists_OnCommand]    P2P list now selected");
				AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/level-1"), _T("Default P2P List"), List::Block);
				EnableWindow(GetDlgItem(hwnd, IDC_OPENP2P), true);
			}
			else
			{
				TRACEI("[listsproc] [Lists_OnCommand]    P2P list now NOT selected");
				RemoveFromDefLists(_T("http://list.iblocklist.com/lists/bluetack/level-1"));
				EnableWindow(GetDlgItem(hwnd, IDC_OPENP2P), false);
			}

		} break; // end case IDC_P2PLIST


		case IDC_SPYLIST:
		{
			TRACED("[listsproc] [Lists_OnCommand]    clicked on IDC_SPYLIST");
			if (IsDlgButtonChecked(hwnd, IDC_SPYLIST)==BST_CHECKED)
			{
				TRACEI("[listsproc] [Lists_OnCommand]    Spy list now selected");
				AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/spyware"), _T("Default Spy List"), List::Block);
				EnableWindow(GetDlgItem(hwnd, IDC_OPENSPY), true);
			}
			else
			{
				TRACEI("[listsproc] [Lists_OnCommand]    Spy list now NOT selected");
				RemoveFromDefLists(_T("http://list.iblocklist.com/lists/bluetack/spyware"));
				EnableWindow(GetDlgItem(hwnd, IDC_OPENSPY), false);
			}

		} break; // end case IDC_SPYLIST


		case IDC_ADSLIST:
		{
			TRACED("[listsproc] [Lists_OnCommand]    clicked on IDC_ADSLIST");
			if (IsDlgButtonChecked(hwnd, IDC_ADSLIST)==BST_CHECKED)
			{
				TRACEI("[listsproc] [Lists_OnCommand]    Ads list now selected");
				AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n"), _T("Default Ads List"), List::Block);
				EnableWindow(GetDlgItem(hwnd, IDC_OPENADS), true);
			}
			else
			{
				TRACEI("[listsproc] [Lists_OnCommand]    Ads list now NOT selected");
				RemoveFromDefLists(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n"));
				EnableWindow(GetDlgItem(hwnd, IDC_OPENADS), false);
			}

		} break; // end case IDC_ADSLIST


		case IDC_EDULIST:
		{
			TRACED("[listsproc] [Lists_OnCommand]    clicked on IDC_EDULIST");
			if (IsDlgButtonChecked(hwnd, IDC_EDULIST)==BST_CHECKED)
			{
				TRACEI("[listsproc] [Lists_OnCommand]    Edu list now selected");
				AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/edu"), _T("Default Edu List"), List::Block);
				EnableWindow(GetDlgItem(hwnd, IDC_OPENEDU), true);
			}
			else
			{
				TRACEI("[listsproc] [Lists_OnCommand]    Edu list now NOT selected");
				RemoveFromDefLists(_T("http://list.iblocklist.com/lists/bluetack/edu"));
				EnableWindow(GetDlgItem(hwnd, IDC_OPENEDU), false);
			}

		} break; // end case IDC_EDULIST


		case IDC_OPENP2P:
		{
			TRACEI("[listsproc] [Lists_OnCommand]    clicked on IDC_OPENP2P");

			vector<DynamicList>::size_type i = FindInDefLists(_T("http://list.iblocklist.com/lists/bluetack/level-1"));
			if (i != -1)
			{
				TRACEV("[listsproc] [Lists_OnCommand]    found P2P list in def-lists");
				if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIST), hwnd, List_DlgProc, (LPARAM)&g_deflists[i])==IDOK)
					g_ret|=LISTS_NEEDRELOAD;
			}
			else
			{
				TRACEW("[listsproc] [Lists_OnCommand]    couldn't find P2P list in def-lists");
			}

		} break; // end case IDC_OPENP2P


		case IDC_OPENSPY:
		{
			TRACEI("[listsproc] [Lists_OnCommand]    clicked on IDC_OPENSPY");

			vector<DynamicList>::size_type i = FindInDefLists(_T("http://list.iblocklist.com/lists/bluetack/spyware"));
			if (i != -1)
			{
				TRACEV("[listsproc] [Lists_OnCommand]    found Spy list in def-lists");
				if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIST), hwnd, List_DlgProc, (LPARAM)&g_deflists[i])==IDOK)
					g_ret|=LISTS_NEEDRELOAD;
			}
			else
			{
				TRACEW("[listsproc] [Lists_OnCommand]    couldn't find Spy list in def-lists");
			}

		} break; // end case IDC_OPENSPY


		case IDC_OPENADS:
		{
			TRACEI("[listsproc] [Lists_OnCommand]    clicked on IDC_OPENADS");

			vector<DynamicList>::size_type i = FindInDefLists(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n"));
			if (i != -1)
			{
				TRACEV("[listsproc] [Lists_OnCommand]    found Ads list in def-lists");
				if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIST), hwnd, List_DlgProc, (LPARAM)&g_deflists[i])==IDOK)
					g_ret|=LISTS_NEEDRELOAD;
			}
			else
			{
				TRACEW("[listsproc] [Lists_OnCommand]    couldn't find P2P list in def-lists");
			}

		} break; // end case IDC_OPENADS


		case IDC_OPENEDU:
		{
			TRACEI("[listsproc] [Lists_OnCommand]    clicked on IDC_OPENEDU");

			vector<DynamicList>::size_type i = FindInDefLists(_T("http://list.iblocklist.com/lists/bluetack/edu"));
			if (i != -1)
			{
				TRACEV("[listsproc] [Lists_OnCommand]    found Edu list in def-lists");
				if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIST), hwnd, List_DlgProc, (LPARAM)&g_deflists[i])==IDOK)
					g_ret|=LISTS_NEEDRELOAD;
			}
			else
			{
				TRACEW("[listsproc] [Lists_OnCommand]    couldn't find P2P list in def-lists");
			}

		} break; // end case IDC_OPENEDU


		case IDC_ADD:
		{
			List *l = {0};
			if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDLIST), hwnd, AddList_DlgProc, (LPARAM)&l)==IDOK)
			{
				tstring s;

				if(DynamicList *dl=dynamic_cast<DynamicList*>(l))
					s=dl->Url;
				else if(StaticList *sl=dynamic_cast<StaticList*>(l))
					s=sl->File.file_str();

				if (s.empty())
				{
					TRACEW("[listsproc] [Lists_OnCommand]    no list to add");
					delete l;
					break;
				}

				// make sure this newly-added list isn't a dupe of one of the Default Lists
				if (s.compare(_T("http://list.iblocklist.com/lists/bluetack/level-1")) == 0 ||
					s.compare(_T("http://list.iblocklist.com/lists/bluetack/spyware")) == 0 ||
					s.compare(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n")) == 0 ||
					s.compare(_T("http://list.iblocklist.com/lists/bluetack/edu")) == 0 )
				{
					// found dupe
					tstring strBuf = boost::str(tformat(_T("[listsproc] [Lists_OnCommand]    Newly-added url:[%1%] found in custom lists!")) % s );
					TRACEBUFI(strBuf);

					vector<DynamicList>::size_type idx = FindUrl(s, g_deflists);
					if (idx != -1)
					{
						// url already present and enabled, ignore
						TRACEI("[listsproc] [Lists_OnCommand]    found url in selected def-lists");
					}
					else
					{
						// url not already enabled in def lists, so enable it
						TRACEI("[listsproc] [Lists_OnCommand]    url NOT found in selected def-lists, enabling it");
						if (s.compare(_T("http://list.iblocklist.com/lists/bluetack/level-1")) == 0)
						{
							AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/level-1"), _T("Default P2P List"), List::Block);
							EnableWindow(GetDlgItem(hwnd, IDC_OPENP2P), true);
							CheckDlgButton(hwnd, IDC_P2PLIST, BST_CHECKED);
						}
						else if (s.compare(_T("http://list.iblocklist.com/lists/bluetack/spyware")) == 0)
						{
							AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/spyware"), _T("Default Spy List"), List::Block);
							EnableWindow(GetDlgItem(hwnd, IDC_OPENSPY), true);
							CheckDlgButton(hwnd, IDC_SPYLIST, BST_CHECKED);
						}
						else if (s.compare(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n")) == 0)
						{
							AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n"), _T("Default Ads List"), List::Block);
							EnableWindow(GetDlgItem(hwnd, IDC_OPENADS), true);
							CheckDlgButton(hwnd, IDC_ADSLIST, BST_CHECKED);
						}
						else if (s.compare(_T("http://list.iblocklist.com/lists/bluetack/edu")) == 0)
						{
							AddToDefLists(_T("http://list.iblocklist.com/lists/bluetack/edu"), _T("Default Edu List"), List::Block);
							EnableWindow(GetDlgItem(hwnd, IDC_OPENEDU), true);
							CheckDlgButton(hwnd, IDC_EDULIST, BST_CHECKED);
						}
						else
						{
							TRACEE("[listsproc] [Lists_OnCommand]  * ERROR:  Can't figure out which list to select!!");
						}
					}

					delete l;
					break;
				}
				else
				{
					tstring strBuf = boost::str(tformat(_T("[listsproc] [Lists_OnCommand]    newly-added url:[%1%] NOT found in custom lists")) % s );
					TRACEBUFV(strBuf);
				}

				HWND list=GetDlgItem(hwnd, IDC_LIST);

				LVFINDINFO lvfi;
				lvfi.flags=LVFI_STRING;
				lvfi.psz=s.c_str();

				int index=ListView_FindItem(list, -1, &lvfi);
				if(index==-1)
				{
					InsertItem(list, ListView_GetItemCount(list), l);

					if(typeid(*l)==typeid(DynamicList))
						g_ret|=LISTS_NEEDUPDATE;
					g_ret|=LISTS_NEEDRELOAD;
				}
				else
				{
					delete l;

					int count=ListView_GetItemCount(list);
					for(int i=0; i<count; i++)
					{
						if(i==index)
						{
							ListView_EnsureVisible(list, i, FALSE);
							ListView_SetItemState(list, i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
						}
						else ListView_SetItemState(list, i, 0, LVIS_SELECTED);
					}
				}
			}
		} break;


		case IDC_EDIT:
		{
			HWND list=GetDlgItem(hwnd, IDC_LIST);
			int index=ListView_GetSelectionMark(list);

			if(index!=-1)
			{
				LVITEM lvi={0};
				lvi.iItem=index;
				lvi.mask=LVIF_PARAM;
				ListView_GetItem(list, &lvi);

				List *plist=(List*)lvi.lParam;

				INT_PTR ret=DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EDITLIST), hwnd, EditList_DlgProc, (LPARAM)&plist);
				if(ret)
				{
					g_ret|=ret;

					lvi.lParam=(LPARAM)plist;
					ListView_SetItem(list, &lvi);

					tstring type=LoadString((plist->Type==List::Allow)?IDS_ALLOW:IDS_BLOCK);

					ListView_SetItemText(list, index, 1, (LPTSTR)type.c_str());
					ListView_SetItemText(list, index, 2, (LPTSTR)plist->Description.c_str());

					if(StaticList *l=dynamic_cast<StaticList*>(plist))
					{
						ListView_SetItemText(list, index, 0, (LPTSTR)l->File.c_str());
					}
					else if(DynamicList *l=dynamic_cast<DynamicList*>(plist))
					{
						ListView_SetItemText(list, index, 0, (LPTSTR)l->Url.c_str());

						if(!path::exists(l->File()))
							g_ret|=LISTS_NEEDUPDATE;
					}
				}
			}
		} break;


		case IDC_REMOVE:
		{
			HWND list=GetDlgItem(hwnd, IDC_LIST);

			stack<int> items;

			for(int index=ListView_GetNextItem(list, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(list, index, LVNI_SELECTED))
				items.push(index);

			if(!items.empty())
				g_ret|=LISTS_NEEDRELOAD;

			while(!items.empty())
			{
				int index=items.top(); items.pop();

				LVITEM lvi={0};
				lvi.iItem=index;
				lvi.mask=LVIF_PARAM;
				ListView_GetItem(list, &lvi);
				ListView_DeleteItem(list, index);

				List *l=(List*)lvi.lParam;
				delete l;
			}
		} break;


		case IDC_OPEN:
		{
			HWND list=GetDlgItem(hwnd, IDC_LIST);
			int index=ListView_GetSelectionMark(list);

			if(index!=-1)
			{
				LVITEM lvi={0};
				lvi.iItem=index;
				lvi.mask=LVIF_PARAM;
				ListView_GetItem(list, &lvi);

				if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIST), hwnd, List_DlgProc, lvi.lParam)==IDOK)
					g_ret|=LISTS_NEEDRELOAD;
			}
		} break;


		case IDC_CREATE:
		{
			StaticList *sl = {0};

			if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CREATELIST), hwnd, CreateList_DlgProc, (LPARAM)&sl)==IDOK)
			{
				if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIST), hwnd, List_DlgProc, (LPARAM)sl)==IDOK)
				{
					HWND list=GetDlgItem(hwnd, IDC_LIST);
					InsertItem(list, ListView_GetItemCount(list), sl);

					g_ret|=LISTS_NEEDRELOAD;
				}
				else delete sl;
			}
		} break;


		case ID_CONTEXT_EXPORTTO:
		{
			HWND list=GetDlgItem(hwnd, IDC_LIST);

			if(ListView_GetSelectedCount(list)>0)
			{
				TCHAR file[MAX_PATH]={0};

				OPENFILENAME ofn={0};
				ofn.lStructSize=sizeof(ofn);
				ofn.hwndOwner=hwnd;
				ofn.lpstrFile=file;
				ofn.lpstrFilter=_T("PeerBlock Text Lists (*.p2p)\0*.p2p\0PeerBlock Binary Lists (*.p2b)\0*.p2b\0");
				ofn.lpstrDefExt=_T("p2p");
				ofn.nMaxFile=MAX_PATH;
				ofn.Flags=OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;

				if(GetSaveFileName(&ofn))
				{
					p2p::list l;

					for(int index=ListView_GetNextItem(list, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(list, index, LVNI_SELECTED))
					{
						LVITEM lvi={0};
						lvi.iItem=index;
						lvi.mask=LVIF_PARAM;
						ListView_GetItem(list, &lvi);

						List *list=(List*)lvi.lParam;

						if(StaticList *lp=dynamic_cast<StaticList*>(list))
							LoadList(lp->File, l);
						else if(DynamicList *lp=dynamic_cast<DynamicList*>(list))
							LoadList(lp->File(), l);
					}

					l.optimize();
					l.save(TSTRING_MBS(file), (ofn.nFilterIndex==1)?p2p::list::file_p2p:p2p::list::file_p2b);
				}
			}
		} break;


		case ID_CONTEXT_MAKESTATIC:
		{
			HWND list=GetDlgItem(hwnd, IDC_LIST);

			if(ListView_GetSelectedCount(list)==1 && MessageBox(hwnd, IDS_MAKESTATICTEXT, IDS_WARNING, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
			{
				TCHAR file[MAX_PATH]={0};

				OPENFILENAME ofn={0};
				ofn.lStructSize=sizeof(ofn);
				ofn.hwndOwner=hwnd;
				ofn.lpstrFile=file;

				ofn.lpstrFilter=_T("PeerBlock Text Lists (*.p2p)\0*.p2p\0PeerBlock Binary Lists (*.p2b)\0*.p2b\0");
				ofn.lpstrDefExt=_T("p2p");

				ofn.nMaxFile=MAX_PATH;
				ofn.Flags=OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;

				if(GetSaveFileName(&ofn))
				{
					path p=path::relative_to(path::base_dir(), file);
					tstring fs=p.file_str();

					LVITEM lvi={0};
					lvi.mask=LVIF_PARAM;
					lvi.iItem=ListView_GetSelectionMark(list);
					ListView_GetItem(list, &lvi);

					DynamicList *dl=(DynamicList*)lvi.lParam;

					p2p::list l;
					LoadList(dl->File(), l);

					l.optimize();
					l.save(TSTRING_MBS(file), (ofn.nFilterIndex==1)?p2p::list::file_p2p:p2p::list::file_p2b);

					StaticList *sl=new StaticList;
					sl->File=p;
					sl->Type=dl->Type;
					sl->Description=dl->Description;

					delete dl;

					lvi.mask=LVIF_TEXT|LVIF_PARAM;
					lvi.lParam=(LPARAM)sl;
					lvi.pszText=(LPTSTR)fs.c_str();
					ListView_SetItem(list, &lvi);

					g_ret|=LISTS_NEEDRELOAD;
				}
			}
		} break;
	}

} // End of Lists_OnCommand()



//================================================================================================
//
//  Lists_OnDestroy()
//
//    - Called by Windows
//
/// <summary>
///   Handles List Manager window destruction, and saves current set of lists to g_config.
/// </summary>
//
static void Lists_OnDestroy(HWND hwnd)
{
	g_config.StaticLists.clear();
	g_config.DynamicLists.clear();

	// Check default-lists first
	for(vector<DynamicList>::size_type i=0; i<g_deflists.size(); i++)
	{
		g_config.DynamicLists.push_back(g_deflists[i]);
	}
	g_deflists.clear();

	// Now check non-standard lists
	HWND list=GetDlgItem(hwnd, IDC_LIST);
	int count=ListView_GetItemCount(list);

	for(int i=0; i<count; i++)
	{
		LVITEM lvi={0};
		lvi.iItem=i;
		lvi.mask=LVIF_PARAM;
		ListView_GetItem(list, &lvi);

		List *plist=(List*)lvi.lParam;

		if(StaticList *l=dynamic_cast<StaticList*>(plist))
			g_config.StaticLists.push_back(*l);
		else if(DynamicList *l=dynamic_cast<DynamicList*>(plist))
			g_config.DynamicLists.push_back(*l);

		delete plist;
	}

	SaveListColumns(list, g_config.ListManagerColumns);

	SaveWindowPosition(hwnd, g_config.ListManagerWindowPos);

	std::sort(g_config.DynamicLists.begin(), g_config.DynamicLists.end());

	g_hListsDlg = NULL;

} // End of Lists_OnDestroy()



//================================================================================================
//
//  Lists_OnGetMinMaxInfo()
//
//    - Called by ???
//
/// <summary>
///   Returns size of the client area of our window.
/// </summary>
//
static void Lists_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	RECT rc={0};
	rc.right=349;
	rc.bottom=219;

	MapDialogRect(hwnd, &rc);

	lpMinMaxInfo->ptMinTrackSize.x=rc.right;
	lpMinMaxInfo->ptMinTrackSize.y=rc.bottom;

} // End of Lists_OnGetMinMaxInfo()



static void Lists_OnSize(HWND hwnd, UINT state, int cx, int cy);



//================================================================================================
//
//  Lists_OnInitDialog()
//
//    - Called by Lists_DlgProc()
//
/// <summary>
///   Sets up our List Manager window.  Configures our Lists ListView, reading in the master "list
///   of lists" from g_config.
/// </summary>
//
static BOOL Lists_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	TRACEI("[listsproc] [Lists_OnInitDialog]  > Entering routine.");

	g_hListsDlg = hwnd;

	HWND list=GetDlgItem(hwnd, IDC_LIST);
	ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

	InsertColumn(list, 0, g_config.ListManagerColumns[0], IDS_FILE);
	InsertColumn(list, 1, g_config.ListManagerColumns[1], IDS_TYPE);
	InsertColumn(list, 2, g_config.ListManagerColumns[2], IDS_DESCRIPTION);

	int index=0;

	for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++)
		InsertItem(list, index++, g_config.StaticLists[i]);

	EnableWindow(GetDlgItem(hwnd, IDC_OPENP2P), false);
	EnableWindow(GetDlgItem(hwnd, IDC_OPENSPY), false);
	EnableWindow(GetDlgItem(hwnd, IDC_OPENADS), false);
	EnableWindow(GetDlgItem(hwnd, IDC_OPENEDU), false);

	std::sort(g_config.DynamicLists.begin(), g_config.DynamicLists.end());
	g_deflists.clear();
	for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++)
	{
		if (g_config.DynamicLists[i].Url.compare(_T("http://list.iblocklist.com/lists/bluetack/level-1")) == 0)
		{
			TRACED("[listsproc] [Lists_OnInitDialog]    found bt_level1 list");
			CheckDlgButton(hwnd, IDC_P2PLIST, BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDC_OPENP2P), true);
			g_deflists.push_back(g_config.DynamicLists[i]);
		}
		else if (g_config.DynamicLists[i].Url.compare(_T("http://list.iblocklist.com/lists/bluetack/spyware")) == 0)
		{
			TRACED("[listsproc] [Lists_OnInitDialog]    found spy list");
			CheckDlgButton(hwnd, IDC_SPYLIST, BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDC_OPENSPY), true);
			g_deflists.push_back(g_config.DynamicLists[i]);
		}
		else if (g_config.DynamicLists[i].Url.compare(_T("http://list.iblocklist.com/lists/bluetack/ads-trackers-and-bad-pr0n")) == 0)
		{
			TRACED("[listsproc] [Lists_OnInitDialog]    found ads list");
			CheckDlgButton(hwnd, IDC_ADSLIST, BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDC_OPENADS), true);
			g_deflists.push_back(g_config.DynamicLists[i]);
		}
		else if (g_config.DynamicLists[i].Url.compare(_T("http://list.iblocklist.com/lists/bluetack/edu")) == 0)
		{
			TRACED("[listsproc] [Lists_OnInitDialog]    found edu list");
			CheckDlgButton(hwnd, IDC_EDULIST, BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd, IDC_OPENEDU), true);
			g_deflists.push_back(g_config.DynamicLists[i]);
		}
		else
		{
			InsertItem(list, index++, g_config.DynamicLists[i]);
		}
	}

	g_ret=0;

	if(	g_config.ListManagerWindowPos.left!=0 || g_config.ListManagerWindowPos.top!=0 ||
		g_config.ListManagerWindowPos.right!=0 || g_config.ListManagerWindowPos.bottom!=0 )
	{
		SetWindowPos(hwnd, NULL,
			g_config.ListManagerWindowPos.left,
			g_config.ListManagerWindowPos.top,
			g_config.ListManagerWindowPos.right-g_config.ListManagerWindowPos.left,
			g_config.ListManagerWindowPos.bottom-g_config.ListManagerWindowPos.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	}

	RECT rc;
	GetClientRect(hwnd, &rc);
	Lists_OnSize(hwnd, 0, rc.right, rc.bottom);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	TRACEI("[listsproc] [Lists_OnInitDialog]  < Exiting routine.");
	return TRUE;

} // End of Lists_OnInitDialog()



//================================================================================================
//
//  Lists_OnNotify()
//
//    - Called by Lists_DlgProc()
//
/// <summary>
///   Handles list-selection and right-clicking.
/// </summary>
//
static INT_PTR Lists_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh)
{
	if(nmh->idFrom==IDC_LIST)
	{
		if(nmh->code==LVN_ITEMCHANGED)
		{
			unsigned int num=ListView_GetSelectedCount(nmh->hwndFrom);

			BOOL open, edit, remove;

			switch(num)
			{
				case 0:
					open=FALSE;
					edit=FALSE;
					remove=FALSE;
					break;
				case 1:
					open=TRUE;
					edit=TRUE;
					remove=TRUE;
				break;
				default:
					open=FALSE;
					edit=FALSE;
					remove=TRUE;
			}

			EnableWindow(GetDlgItem(hwnd, IDC_OPEN), open);
			EnableWindow(GetDlgItem(hwnd, IDC_EDIT), edit);
			EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), remove);
		}
		else if(nmh->code==NM_RCLICK)
		{
			int count=ListView_GetSelectedCount(nmh->hwndFrom);

			if(count>0)
			{
				NMITEMACTIVATE *nmia=(NMITEMACTIVATE*)nmh;

				HMENU menu=LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_LISTSCONTEXT));
				HMENU context=GetSubMenu(menu, 0);

				if(count==1)
				{
					tstring buf;
					MENUITEMINFO info={0};

					info.cbSize=sizeof(info);
					info.fMask=MIIM_ID|MIIM_TYPE;
					info.fType=MFT_STRING;

					{
						LVITEM lvi={0};
						lvi.mask=LVIF_PARAM;
						lvi.iItem=nmia->iItem;
						ListView_GetItem(nmh->hwndFrom, &lvi);

						List *l=(List*)lvi.lParam;

						if(typeid(*l)==typeid(DynamicList))
						{
							buf=LoadString(IDS_MAKESTATIC);
							info.wID=ID_CONTEXT_MAKESTATIC;
							info.dwTypeData=(LPTSTR)buf.c_str();
							InsertMenuItem(context, 1, TRUE, &info);
						}
					}

					InsertMenu(context, 0, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);

					buf=LoadString(IDS_OPEN);
					info.wID=IDC_OPEN;
					info.dwTypeData=(LPTSTR)buf.c_str();
					InsertMenuItem(context, 0, TRUE, &info);
				}

				RECT rect;
				GetWindowRect(nmh->hwndFrom, &rect);

				SetForegroundWindow(hwnd);
				TrackPopupMenuEx(context, TPM_TOPALIGN, rect.left+nmia->ptAction.x, rect.top+nmia->ptAction.y, hwnd, NULL);
				DestroyMenu(menu);
			}
		}
		else if(nmh->code==NM_DBLCLK && ListView_GetSelectedCount(nmh->hwndFrom)==1)
			SendMessage(GetDlgItem(hwnd, IDC_EDIT), BM_CLICK, 0, 0);
	}
	else if(nmh->code==PSN_SETACTIVE)
		PropSheet_SetWizButtons(nmh->hwndFrom, PSWIZB_BACK|PSWIZB_NEXT);
	return FALSE;

} // End of Lists_OnNotify()



//================================================================================================
//
//  Lists_OnSize()
//
//    - Called by Lists_DlgProc()
//
/// <summary>
///   Handles resizing of the List Manager window.
/// </summary>
//
static void Lists_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	HWND list=GetDlgItem(hwnd, IDC_LIST);
	HWND open=GetDlgItem(hwnd, IDC_OPEN);
	HWND create=GetDlgItem(hwnd, IDC_CREATE);
	HWND add=GetDlgItem(hwnd, IDC_ADD);
	HWND edit=GetDlgItem(hwnd, IDC_EDIT);
	HWND remove=GetDlgItem(hwnd, IDC_REMOVE);
	HWND morelists=GetDlgItem(hwnd, IDC_MORELISTS);
	HWND defgroup=GetDlgItem(hwnd, IDC_DEFGROUP);

	RECT rc;
	GetWindowRect(open, &rc);

	HDWP dwp=BeginDeferWindowPos(8);

	DeferWindowPos(dwp, defgroup, NULL, 7, 2, cx-14, 120, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, list, NULL, 7, 130, cx-14, cy-21-140-(rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, open, NULL, 7, cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, create, NULL, (rc.right-rc.left)+14, cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, add, NULL, cx-((rc.right-rc.left+7)*3), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, edit, NULL, cx-((rc.right-rc.left+7)*2), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, remove, NULL, cx-(rc.right-rc.left+7), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, morelists, NULL, cx/2-90, cy-7-(rc.bottom-rc.top)-20, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);

	if(state==SIZE_RESTORED)
	{
		SaveWindowPosition(hwnd, g_config.ListManagerWindowPos);
	}

} // End of Lists_OnSize()



//================================================================================================
//
//  Lists_DlgProc()
//
//    - Called by Windows
//
/// <summary>
///   The List Manager's messageproc, it simply passes off handleable messages to the appropriate
///   subroutine.
/// </summary>
//
INT_PTR CALLBACK Lists_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch(msg)
		{
			HANDLE_MSG(hwnd, WM_CLOSE, Lists_OnClose);
			HANDLE_MSG(hwnd, WM_COMMAND, Lists_OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, Lists_OnDestroy);
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, Lists_OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_INITDIALOG, Lists_OnInitDialog);
			HANDLE_MSG(hwnd, WM_NOTIFY, Lists_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, Lists_OnSize);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(hwnd);
				return 1;
			default: return 0;
		}
	}
	catch(exception &ex)
	{
		UncaughtExceptionBox(hwnd, ex, __FILE__, __LINE__);
		return 0;
	}
	catch(...)
	{
		UncaughtExceptionBox(hwnd, __FILE__, __LINE__);
		return 0;
	}

} // End of Lists_DlgProc()
