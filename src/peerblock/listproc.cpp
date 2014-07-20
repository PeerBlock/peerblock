//================================================================================================
//  listproc.cpp
//
//  This file contains all the routines used to handle the editing or creation of a lists: the
//  window that pops up after you open the List Manager and then select a list and click Open List,
//  or if you click Create List, select a list File, and then click OK.
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

#define IDC_SUBCTRL 200
static const UINT ID_CONTEXT_REMOVE=200;

class ListRow
{
private:
	static tstring format_ip(unsigned int ip)
	{
		const unsigned char *bytes=reinterpret_cast<const unsigned char*>(&ip);

		tstringstream ss;
		ss << (int)bytes[3] << _T('.') << (int)bytes[2] << _T('.') << (int)bytes[1] << _T('.') << (int)bytes[0];

		return ss.str();
	}

public:
	tstring name, start, end;
	unsigned int lstart, lend;

	ListRow() {}

	ListRow(const p2p::range &r)
		: name(WCHAR_TSTRING(r.name)),start(format_ip(r.start.ipl)),end(format_ip(r.end.ipl)),lstart(r.start.ipl),lend(r.end.ipl) {}
};



static WNDPROC g_oldlvproc;
static HWND g_subctrl=NULL;
static int g_item, g_subitem;
static bool g_cansub=false;
static vector<ListRow> g_rows;

HWND g_hListDlg = NULL;


//================================================================================================
//
//  ListView_SubProc()
//
//    - Called by Windows
//
/// <summary>
///   Message handler for sub-controls.
/// </summary>
/// <remarks>
///   This routine is called a LOT while the window is open; hundreds of times per second.  These
///   "spammy" calls fail to pass any of the tests to actually do anything but pass the message on
///   to CallWindowProc(), but just be aware of it and don't put any code in here that's not
///   inside an if-statement!
/// </remarks>
//
static LRESULT CALLBACK ListView_SubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg==WM_COMMAND && LOWORD(wParam)==IDC_SUBCTRL && HIWORD(wParam)==EN_KILLFOCUS)
	{
		TRACEV("[listproc] [ListView_SubProc]    kill focus");
		TCHAR buf[256];
		GetWindowText(g_subctrl, buf, 256);

		if(_tcslen(buf)>0)
		{
			TRACEV("[listproc] [ListView_SubProc]    no subctrl text");
			ListRow &r=g_rows[g_item];

			switch(g_subitem) {
				case 0:
					r.name=buf;
					break;
				case 1:
					r.start=buf;
					break;
				case 2:
					r.end=buf;
					break;
			}
		}
		else if(g_subitem==0 && g_item==ListView_GetItemCount(hwnd)-1)
		{
			TRACEV("[listproc] [ListView_SubProc]    no subitem; removing item");
			ListView_DeleteItem(hwnd, g_item);
			g_rows.erase(g_rows.begin()+g_item);
			InvalidateRect(hwnd, NULL, FALSE);
		}

		TRACEV("[listproc] [ListView_SubProc]    destroying g_subctrl");
		DestroyWindow(g_subctrl);
		g_subctrl=NULL;
	}
	else if(msg==WM_KEYDOWN && wParam==VK_DELETE && ListView_GetSelectedCount(hwnd)>0 && GetDlgItem(GetParent(hwnd), IDC_REMOVE))
	{
		TRACEI("[listproc] [ListView_SubProc]    delete/remove item");
		stack<int> items;
		for(int index=ListView_GetNextItem(hwnd, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(hwnd, index, LVNI_SELECTED))
			items.push(index);

		SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
		for(; !items.empty(); items.pop())
		{
			ListView_DeleteItem(hwnd, items.top());
			g_rows.erase(g_rows.begin()+items.top());
		}
		SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(hwnd, NULL, FALSE);
	}

	return CallWindowProc(g_oldlvproc, hwnd, msg, wParam, lParam);

} // End of ListView_SubProc()



//================================================================================================
//
//  ParseIp()
//
//    - Called by ???
//
/// <summary>
///   Takes the specified string, and if it's an IPv4 address converts it to an unsigned int.
/// </summary>
/// <remarks>
///   This routine is used to parse text the user's entered into the Search box, to see if it's
///   an IPv4 address or not.  TODO: update ParseIp to handle IPv6 addresses too!
/// </remarks>
//
static unsigned int ParseIp(LPCTSTR str)
{
	unsigned int ipa, ipb, ipc, ipd;

	if(_stscanf(str, _T("%u.%u.%u.%u"), &ipa, &ipb, &ipc, &ipd)==4)
	{
		union {
			unsigned int ip;
			unsigned char bytes[4];
		};

		bytes[0]=ipd;
		bytes[1]=ipc;
		bytes[2]=ipb;
		bytes[3]=ipa;

		return ip;
	}

	tstring strBuf = boost::str(tformat(_T("[listproc] [ParseIp]    string [%1%] is not an IPv4 address")) % str );
	TRACEBUFD(strBuf);

	return 0;

} // End of ParseIp()



//================================================================================================
//
//  List_OnClose()
//
//    - Called by List_DlgProc
//
/// <summary>
///   Processes the List Edit window default close-button; the X button in the corner.
/// </summary>
/// <remarks>
///   Note that this routine is NOT called when the user clicks the Close button, meaning the
///   in-window button labelled "Close".  Only the default Windows close (X) button.
/// </remarks>
//
static void List_OnClose(HWND hwnd)
{
	TRACEI("[listproc] [List_OnClose]    closing list-edit window");
	EndDialog(hwnd, IDCANCEL);

} // End of List_OnClose()



//================================================================================================
//
//  List_OnCommand()
//
//    - Called by List_DlgProc
//
/// <summary>
///   Handles the user clicking on buttons in the List Manager window.
/// </summary>
//
static void List_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	if(g_subctrl) // haven't found a better way to avoid the default push button :/
	{
		TRACEV("[listproc] [List_OnCommand]    global subctrl");
		SetFocus(hwnd);

		if(g_subitem!=2)
		{
			TRACEV("[listproc] [List_OnCommand]    g_subitem != 2");
			HWND list=GetDlgItem(hwnd, IDC_LIST);

			TCHAR buf[32];
			ListView_GetItemText(list, g_item, ++g_subitem, buf, 32);

			if(buf[0]==_T('\0'))
			{
				TRACEV("[listproc] [List_OnCommand]    null buffer");
				RECT rc;
				ListView_GetSubItemRect(list,	g_item, g_subitem, LVIR_BOUNDS, &rc);

				g_subctrl=CreateWindow(WC_IPADDRESS, NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|CBS_DROPDOWNLIST,
					rc.left-4, rc.top-4, rc.right-rc.left+8, rc.bottom-rc.top+8, list, (HMENU)IDC_SUBCTRL, GetModuleHandle(NULL), NULL);

				SetWindowFont(g_subctrl, GetStockFont(DEFAULT_GUI_FONT), FALSE);

				SetFocus(g_subctrl);
			}
		}

		return;
	}

	switch(id)
	{
		case IDC_ADD:
		{
			TRACEI("[listproc] [List_OnCommand]    user clicked Add");
			g_rows.push_back(ListRow());

			HWND list=GetDlgItem(hwnd, IDC_LIST);

			LVITEM lvi={0};
			lvi.iItem=ListView_GetItemCount(list);
			ListView_InsertItem(list, &lvi);
			ListView_EnsureVisible(list, lvi.iItem, FALSE);

			g_item=lvi.iItem;
			g_subitem=0;

			RECT rc, sub;
			ListView_GetSubItemRect(list, lvi.iItem, 0, LVIR_BOUNDS, &rc);
			ListView_GetSubItemRect(list, lvi.iItem, 1, LVIR_BOUNDS, &sub);

			g_subctrl=CreateWindow(WC_EDIT, NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
				1, rc.top-2, sub.left, rc.bottom-rc.top+4, list, (HMENU)IDC_SUBCTRL, GetModuleHandle(NULL), NULL);

			SetWindowFont(g_subctrl, GetStockFont(DEFAULT_GUI_FONT), FALSE);

			SetFocus(g_subctrl);

		} break; // End of case IDC_ADD


		case IDC_REMOVE:
		{
			TRACEI("[listproc] [List_OnCommand]    user clicked Remove");
			HWND list=GetDlgItem(hwnd, IDC_LIST);

			stack<int> items;

			for(int index=ListView_GetNextItem(list, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(list, index, LVNI_SELECTED))
				items.push(index);

			for(; !items.empty(); items.pop())
			{
				ListView_DeleteItem(list, items.top());
				g_rows.erase(g_rows.begin()+items.top());
			}

		} break; // End of case IDC_REMOVE


		case IDOK:
		{
			TRACEI("[listproc] [List_OnCommand]    user clicked OK");
			p2p::list l;

			for(vector<ListRow>::size_type i=0; i<g_rows.size(); i++)
			{
				p2p::range r;

				r.name=TSTRING_WCHAR(g_rows[i].name);
				r.start=ParseIp(g_rows[i].start.c_str());
				r.end=ParseIp(g_rows[i].end.c_str());

				l.insert(r);
			}

			l.optimize();

			StaticList *sl=(StaticList*)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);

			path p=sl->File.has_root()?sl->File:(path::base_dir()/sl->File);

			l.save(TSTRING_MBS(p.file_str()), p2p::list::file_p2p);

			EndDialog(hwnd, IDOK);

		} break; // End of case IDC_OK


		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;

		case IDC_SEARCH:
			TRACED("[listproc] [List_OnCommand]    user clicked into or out of Search box, or entered some text");
			if(codeNotify==EN_CHANGE)
			{
				HWND list=GetDlgItem(hwnd, IDC_LIST);
				tstring query=GetDlgItemText(hwnd, IDC_SEARCH);

				LVFINDINFO find={0};
				find.flags=LVFI_PARTIAL|LVFI_STRING|LVFI_WRAP;
				find.psz=query.c_str();

				stack<int> items;
				for(int index=ListView_GetNextItem(list, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(list, index, LVNI_SELECTED))
					items.push(index);

				int i=ListView_FindItem(list, items.empty()?-1:(items.top()-1), &find);

				if(i!=-1) {
					for(; !items.empty(); items.pop())
						ListView_SetItemState(list, items.top(), 0, LVIS_SELECTED);

					ListView_SetItemState(list, i, LVIS_SELECTED, LVIS_SELECTED);
					ListView_EnsureVisible(list, i, FALSE);
				}
			}
			break; // End of case IDC_SEARCH


		case IDC_NEXT:
		{
			TRACEV("[listproc] [List_OnCommand]    user clicked Find Next (search) button");
			HWND list=GetDlgItem(hwnd, IDC_LIST);
			tstring query=GetDlgItemText(hwnd, IDC_SEARCH);

			LVFINDINFO find={0};
			find.flags=LVFI_PARTIAL|LVFI_STRING|LVFI_WRAP;
			find.psz=query.c_str();

			stack<int> items;
			for(int index=ListView_GetNextItem(list, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(list, index, LVNI_SELECTED))
				items.push(index);

			int i=ListView_FindItem(list, items.empty()?-1:items.top(), &find);

			for(; !items.empty(); items.pop())
				ListView_SetItemState(list, items.top(), 0, LVIS_SELECTED);

			if(i!=-1) {
				ListView_SetItemState(list, i, LVIS_SELECTED, LVIS_SELECTED);
				ListView_EnsureVisible(list, i, FALSE);
			}

		} break; // end case IDC_NEXT

	} // end switch(id)

} // End of List_OnCommand()



//================================================================================================
//
//  List_OnDestroy()
//
//    - Called from List_DlgProc
//
/// <summary>
///   Called when we're destroying the window.
/// </summary>
//
static void List_OnDestroy(HWND hwnd)
{
	TRACEV("[listproc] [List_OnDestroy]  > Entering routine.");
	g_rows.clear();

	HWND list=GetDlgItem(hwnd, IDC_LIST);
	SaveListColumns(list, g_config.ListEditorColumns);

	g_hListDlg = NULL;

	TRACEV("[listproc] [List_OnDestroy]  < Leaving routine.");

} // End of List_OnDestroy()



//================================================================================================
//
//  InsertColumn()
//
//    - Called by ???
//
/// <summary>
///   Adds a column to our ListView.
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
//  List_OnGetMinMaxInfo()
//
//    - Called by ???
//
/// <summary>
///   Specifies max size of our window.
/// </summary>
//
static void List_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	RECT rc={0};
	rc.right=372;
	rc.bottom=188;

	MapDialogRect(hwnd, &rc);

	lpMinMaxInfo->ptMinTrackSize.x=rc.right;
	lpMinMaxInfo->ptMinTrackSize.y=rc.bottom;

} // End of List_OnGetMinMaxInfo()



static void List_OnSize(HWND hwnd, UINT state, int cx, int cy);



//================================================================================================
//
//  List_OnInitDialog()
//
//    - Called by List_DlgProc
//
/// <summary>
///   Initializes the List Manager window.  Adds lists to our ListView, etc.
/// </summary>
//
static BOOL List_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	TRACEI("[listproc] [List_OnInitDialog]  > Entering routine.");

	g_hListDlg = hwnd;

	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);

	HWND list=GetDlgItem(hwnd, IDC_LIST);
	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

	InsertColumn(list, 0, g_config.ListEditorColumns[0], IDS_RANGE);
	InsertColumn(list, 1, g_config.ListEditorColumns[1], IDS_STARTIP);
	InsertColumn(list, 2, g_config.ListEditorColumns[1], IDS_ENDIP);

	List *lp=(List*)lParam;

	path p;

	if(DynamicList *dl=dynamic_cast<DynamicList*>(lp))
	{
		DestroyWindow(GetDlgItem(hwnd, IDC_ADD));
		DestroyWindow(GetDlgItem(hwnd, IDC_REMOVE));
		DestroyWindow(GetDlgItem(hwnd, IDOK));

		SetDlgItemText(hwnd, IDCANCEL, LoadString(IDS_CLOSE).c_str());
		p=dl->File();
	}
	else if(StaticList *sl=dynamic_cast<StaticList*>(lp))
	{
		if(sl->File.has_root()) p=sl->File;
		else if(!sl->File.empty()) p=path::base_dir()/sl->File;
	}

	if(!p.empty() && path::exists(p))
	{
		p2p::list l;
		LoadList(p, l);

		g_rows.reserve(l.size());
		g_rows.assign(l.begin(), l.end());
		ListView_SetItemCount(list, g_rows.size());
	}

	g_oldlvproc=(WNDPROC)SetWindowLongPtr(list, GWLP_WNDPROC, (LONG_PTR)ListView_SubProc);

	if( g_config.ListEditorWindowPos.left!=0 || g_config.ListEditorWindowPos.top!=0 ||
		g_config.ListEditorWindowPos.right!=0 || g_config.ListEditorWindowPos.bottom!=0	)
	{
		SetWindowPos(hwnd, NULL,
			g_config.ListEditorWindowPos.left,
			g_config.ListEditorWindowPos.top,
			g_config.ListEditorWindowPos.right-g_config.ListEditorWindowPos.left,
			g_config.ListEditorWindowPos.bottom-g_config.ListEditorWindowPos.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	}

	RECT rc;
	GetClientRect(hwnd, &rc);
	List_OnSize(hwnd, 0, rc.right, rc.bottom);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	TRACEI("[listproc] [List_OnInitDialog]  < Leaving routine.");
	return TRUE;

} // End of List_OnInitDialog()



//================================================================================================
//
//  List_OnNotify()
//
//    - Called by List_DlgProc
//
/// <summary>
///   Handles notification events, including right-mouse clicks.
/// </summary>
//
static INT_PTR List_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh)
{
	if(nmh->idFrom==IDC_LIST)
	{
		switch(nmh->code)
		{
			case LVN_GETDISPINFO:
			{
				// This gets called hundreds of times per second while mouse is hovering over
				// entries in the Range List.
				NMLVDISPINFO *di=(NMLVDISPINFO*)nmh;

				LPCTSTR str;

				switch(di->item.iSubItem)
				{
					case 0:
						str=g_rows[di->item.iItem].name.c_str();
						break;
					case 1:
						str=g_rows[di->item.iItem].start.c_str();
						break;
					case 2:
						str=g_rows[di->item.iItem].end.c_str();
						break;
					default:
						__assume(0);
				}
				di->item.pszText=(LPTSTR)str;

			} break; // End of case LVN_GETDISPINFO


			case LVN_ODFINDITEM:
			{
				TRACEI("[listproc] [List_OnNotify]    searching for item");
				NMLVFINDITEM &fi=*(NMLVFINDITEM*)nmh;

				tstring s(fi.lvfi.psz);
				boost::to_lower(s);

				unsigned int ip=ParseIp(fi.lvfi.psz);

				int end=(int)g_rows.size();

				for(int i=fi.iStart; i<end; i++)
				{
					if(ip && g_rows[i].lstart<=ip && ip<=g_rows[i].lend)
					{
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, i);
						return i;
					}
					else
					{
						tstring str=g_rows[i].name;
						boost::to_lower(str);

						if(str.compare(s) == 0)
						{
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, i);
							return i;
						}
					}

					if(i==end-1 && fi.iStart>0)
					{
						i=-1;
						end=fi.iStart;

						fi.iStart=0;
					}
				}

				SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
				return -1;

			} break; // End of case LVN_ODFINDITEM


			case LVN_ITEMCHANGED:
			{
				// this is processed (multiple times) whenever the user clicks on an item in the Range List
				TRACED("[listproc] [List_OnNotify]    item changed");
				NMLISTVIEW *nm=(NMLISTVIEW*)nmh;

				if(
					((nm->uOldState & LVIS_SELECTED) != LVIS_SELECTED) &&
					((nm->uNewState & LVIS_SELECTED) == LVIS_SELECTED)
				) g_cansub=false;

			} break; // End of case LVN_ITEMCHANGED


			case NM_CLICK:
			{
				TRACEV("[listproc] [List_OnNotify]    click detected in client-area of range list");
				{
					List *l=(List*)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);
					if(typeid(*l)!=typeid(StaticList)) break;
				}

				if(!g_cansub)
					g_cansub=(ListView_GetSelectedCount(nmh->hwndFrom)==1);
				else
				{
					TRACEV("[listproc] [List_OnNotify]    + cansub");
					LVHITTESTINFO info={0};
					info.pt=((NMITEMACTIVATE*)nmh)->ptAction;

					if(g_subctrl)
					{
						TRACEV("[listproc] [List_OnNotify]    + g_subctrl");
						DestroyWindow(g_subctrl);
						g_subctrl=NULL;
					}

					if(ListView_SubItemHitTest(nmh->hwndFrom, &info)!=-1)
					{
						TRACEV("[listproc] [List_OnNotify]    + subitem hit");
						RECT rc;
						ListView_GetSubItemRect(nmh->hwndFrom,	 info.iItem, info.iSubItem, LVIR_BOUNDS, &rc);

						TCHAR buf[256];
						ListView_GetItemText(nmh->hwndFrom, info.iItem, info.iSubItem, buf, 256);

						g_item=info.iItem;
						g_subitem=info.iSubItem;

						if(info.iSubItem==0)
						{
							TRACEV("[listproc] [List_OnNotify]    + subitem: 0");
							RECT sub;
							ListView_GetSubItemRect(nmh->hwndFrom, info.iItem, 1, LVIR_BOUNDS, &sub);

							g_subctrl=CreateWindow(WC_EDIT, NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
								1, rc.top-2, sub.left, rc.bottom-rc.top+4, nmh->hwndFrom, (HMENU)IDC_SUBCTRL, GetModuleHandle(NULL), NULL);

							SetWindowFont(g_subctrl, GetStockFont(DEFAULT_GUI_FONT), FALSE);

							SetWindowText(g_subctrl, buf);
							SetFocus(g_subctrl);
						}
						else
						{
							TRACEV("[listproc] [List_OnNotify]    + subitem: not 0");
							g_subctrl=CreateWindow(WC_IPADDRESS, NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|CBS_DROPDOWNLIST,
								rc.left-4, rc.top-4, rc.right-rc.left+8, rc.bottom-rc.top+8, nmh->hwndFrom, (HMENU)IDC_SUBCTRL, GetModuleHandle(NULL), NULL);

							SetWindowFont(g_subctrl, GetStockFont(DEFAULT_GUI_FONT), FALSE);

							SendMessage(g_subctrl, IPM_SETADDRESS, 0, ParseIp(buf));
							SetFocus(g_subctrl);
						}
					}
				}

			} break; // End of case NM_CLICK


			case NM_RCLICK:
			{
				TRACEV("[listproc] [List_OnNotify]    user right-clicked in client area of range list");
				NMITEMACTIVATE *nmia=(NMITEMACTIVATE*)nmh;

				if(nmia->iItem!=-1)
				{
					TRACED("[listproc] [List_OnNotify]    + click detected on a list entry");
					p2p::list l;
					stack<int> indexes;

					for(int index=ListView_GetNextItem(nmh->hwndFrom, -1, LVNI_SELECTED); index!=-1; index=ListView_GetNextItem(nmh->hwndFrom, index, LVNI_SELECTED))
					{
						TCHAR name[256], start[16], end[16];

						ListView_GetItemText(nmh->hwndFrom, index, 0, name, 256);
						ListView_GetItemText(nmh->hwndFrom, index, 1, start, 16);
						ListView_GetItemText(nmh->hwndFrom, index, 2, end, 16);

						l.insert(p2p::range(TSTRING_WCHAR(name), ParseIp(start), ParseIp(end)));

						indexes.push(index);
					}

					HMENU menu=LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_LISTCONTEXT));
					HMENU context=GetSubMenu(menu, 0);

					{
						List *l=(List*)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);

						if(typeid(*l)==typeid(StaticList))
						{
							InsertMenu(context, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
							InsertMenu(context, -1, MF_BYPOSITION|MF_STRING, ID_CONTEXT_REMOVE, LoadString(IDS_REMOVE).c_str());
						}
					}

					RECT rect;
					GetWindowRect(nmh->hwndFrom, &rect);

					TRACED("[listproc] [List_OnNotify]    + popping up context menu");
					SetForegroundWindow(hwnd);
					UINT ret=TrackPopupMenuEx(context, TPM_TOPALIGN|TPM_RETURNCMD, rect.left+nmia->ptAction.x, rect.top+nmia->ptAction.y, hwnd, NULL);

					DestroyMenu(menu);

					switch(ret)
					{
						case ID_CONTEXT_ALLOWFOR15MINUTES:
						{
							TRACEI("[listproc] [List_OnNotify]    + allow for 15 minutes");
							g_tempallow.insert(l);

							time_t t=time(NULL)+900;

							for(p2p::list::const_iterator iter=l.begin(); iter!=l.end(); ++iter)
								g_allowlist.push_back(make_pair(t, *iter));

							LoadLists(hwnd);

						} break; // End of case ID_CONTEXT_ALLOWFOR15MINUTES


						case ID_CONTEXT_ALLOWFORONEHOUR:
						{
							TRACEI("[listproc] [List_OnNotify]    + allow for one hour");
							g_tempallow.insert(l);

							time_t t=time(NULL)+3600;

							for(p2p::list::const_iterator iter=l.begin(); iter!=l.end(); ++iter)
								g_allowlist.push_back(make_pair(t, *iter));

							LoadLists(hwnd);

						} break; // End of case ID_CONTEXT_ALLOWFORONEHOUR


						case ID_CONTEXT_ALLOWPERMANENTLY:
						{
							TRACEI("[listproc] [List_OnNotify]    + allow permanently");
							const path dir=path::base_dir()/_T("lists");

							if(!path::exists(dir)) path::create_directory(dir);

							const path file=dir/_T("permallow.p2b");

							p2p::list list;
							LoadList(file, list);

							list.insert(l);
							list.optimize();

							list.save(TSTRING_MBS(file.file_str()), p2p::list::file_p2b);

							bool found=false;

							for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++)
							{
								if(g_config.StaticLists[i].File==_T("lists\\permallow.p2b"))
								{
									found=true;
									break;
								}
							}

							if(!found)
							{
								StaticList list;
								list.Type=List::Allow;
								list.Description=LoadString(IDS_PERMALLOWS);
								list.File=_T("lists\\permallow.p2b");

								g_config.StaticLists.push_back(list);
							}

							LoadLists(hwnd);

						} break; // End of case ID_CONTEXT_ALLOWPERMANENTLY


						case ID_CONTEXT_REMOVE:
						{
							TRACEI("[listproc] [List_OnNotify]    + remove");
							SendMessage(nmh->hwndFrom, WM_SETREDRAW, FALSE, 0);
							for(; !indexes.empty(); indexes.pop())
							{
								ListView_DeleteItem(nmh->hwndFrom, indexes.top());
								g_rows.erase(g_rows.begin()+indexes.top());
							}
							SendMessage(nmh->hwndFrom, WM_SETREDRAW, TRUE, 0);
							InvalidateRect(nmh->hwndFrom, NULL, FALSE);

						} break; // End of case ID_CONTEXT_REMOVE
					}
				}
			} break;
		}
	}
	else if(nmh->code==PSN_SETACTIVE)
	{
		TRACEI("[listproc] [List_OnNotify]    psn set active");
		PropSheet_SetWizButtons(nmh->hwndFrom, PSWIZB_BACK|PSWIZB_NEXT);
	}

	return 0;

} // End of case List_OnNotify()



//================================================================================================
//
//  List_OnSize()
//
//    - Called by List_DlgProc
//
/// <summary>
///   Handles resizing of the window.
/// </summary>
//
static void List_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	TRACED("[listproc] [List_OnSize]  > Entering routine.");
	HWND searchtext=GetDlgItem(hwnd, IDC_SEARCHTEXT);
	HWND search=GetDlgItem(hwnd, IDC_SEARCH);
	HWND next=GetDlgItem(hwnd, IDC_NEXT);
	HWND list=GetDlgItem(hwnd, IDC_LIST);
	HWND add=GetDlgItem(hwnd, IDC_ADD);
	HWND remove=GetDlgItem(hwnd, IDC_REMOVE);
	HWND save=GetDlgItem(hwnd, IDOK);
	HWND close=GetDlgItem(hwnd, IDCANCEL);

	RECT rc, st, s;
	GetWindowRect(close, &rc);
	GetWindowRect(searchtext, &st);
	GetWindowRect(search, &s);

	HDWP dwp=BeginDeferWindowPos(add?8:2);

	DeferWindowPos(dwp, searchtext, NULL, 7, 7, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, search, NULL, (st.right-st.left)+14, 7, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, next, NULL, (st.right-st.left)+(s.right-s.left)+21, 7, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, list, NULL, 7, (st.bottom-st.top)+14, cx-14, cy-28-(st.bottom-st.top)-(rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);

	if(add)
	{
		DeferWindowPos(dwp, add, NULL, 7, cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE);
		DeferWindowPos(dwp, remove, NULL, (rc.right-rc.left)+14, cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE);
		DeferWindowPos(dwp, save, NULL, cx-((rc.right-rc.left+7)*2), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	}

	DeferWindowPos(dwp, close, NULL, cx-(rc.right-rc.left+7), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);

	if(state==SIZE_RESTORED)
	{
		SaveWindowPosition(hwnd, g_config.ListEditorWindowPos);
	}

	TRACED("[listproc] [List_OnSize]  < Leaving routine.");

} // End of case List_OnSize



//================================================================================================
//
//  List_DlgProc()
//
//    - Called by Windows
//
/// <summary>
///   The main messageproc of this thread.  Dispatches incoming messages to other routines.
/// </summary>
//
INT_PTR CALLBACK List_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch(msg)
		{
			HANDLE_MSG(hwnd, WM_CLOSE, List_OnClose);
			HANDLE_MSG(hwnd, WM_COMMAND, List_OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, List_OnDestroy);
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, List_OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_INITDIALOG, List_OnInitDialog);
			HANDLE_MSG(hwnd, WM_NOTIFY, List_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, List_OnSize);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(hwnd);
				return 1;
			default: return 0;
		}
	}
	catch(exception &ex)
	{
		TRACEC("[listproc] [List_DlgProc]    ERROR:  Caught EXCEPTION!!");
		UncaughtExceptionBox(hwnd, ex, __FILE__, __LINE__);
		return 0;
	}
	catch(...)
	{
		TRACEC("[listproc] [List_DlgProc]    ERROR:  Caught unknown EXCEPTION!!");
		UncaughtExceptionBox(hwnd, __FILE__, __LINE__);
		return 0;
	}

} // End of case List_DlgProc
