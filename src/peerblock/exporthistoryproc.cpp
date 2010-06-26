/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2010 PeerBlock, LLC

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
using namespace sqlite3x;

HWND g_hExportHistoryDlg = NULL;

static void ExportHistory_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCANCEL);
}

static void ExportHistory_OnDestroy(HWND hwnd) {
	g_hExportHistoryDlg = NULL;
}

static ostream& operator<<(ostream &os, const SYSTEMTIME &st) {
	os << setfill('0')
		<< setw(4) << st.wYear << '-'
		<< setw(2) << st.wMonth << '-'
		<< setw(2) << st.wDay;

	return os;
}

class ExportFuncs {
private:
	HWND hwnd;
	ofstream fs;
	boost::shared_ptr<sqlite3_command> cmd;
	sqlite3_reader reader;
	sqlite3_lock lock;

	static void FormatIp(ostream &os, unsigned int ip, unsigned short port) {
		const unsigned char *bytes=reinterpret_cast<const unsigned char*>(&ip);

		os << (int)bytes[3] << '.' << (int)bytes[2] << '.' << (int)bytes[1] << '.' << (int)bytes[0];
		if(port!=0) os << ':' << (int)port;
	}

public:
	ExportFuncs(HWND hwnd) : hwnd(hwnd),lock(g_con, false, false) {
		path p=GetDlgItemText(hwnd, IDC_FILE);
		if(!p.has_root()) p=path::base_dir()/p;

		fs.open(TSTRING_MBS(p.file_str()).c_str());
	}
	~ExportFuncs() {
		reader.close();
		cmd=boost::shared_ptr<sqlite3_command>();
	}

	bool IsOpen() const { return fs.is_open(); }

	int Init() {
		string query, cquery;
		{
			ostringstream ss, css;
			ss << "select datetime(time, 'localtime'),name,source,sourceport,destination,destport,protocol,action from t_history,t_names";
			css << "select count(*) from t_history";

			bool first=true;

			if(IsDlgButtonChecked(hwnd, IDC_ACTION)==BST_CHECKED) {
				int blocked=ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_ACTIONLIST))==0;
				ss << " where action=" << blocked;
				css << " where action=" << blocked;
				first=false;
			}

			if(IsDlgButtonChecked(hwnd, IDC_FROM)==BST_CHECKED) {
				SYSTEMTIME st;
				DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_FROMDATE), &st);

				if(first) {
					ss << " where";
					css << " where";
					first=false;
				}
				else {
					ss << " and";
					css << " and";
				}

				ss << " time>=julianday('" << st << "', 'utc')";
				css << " time>=julianday('" << st << "', 'utc')";
			}

			if(IsDlgButtonChecked(hwnd, IDC_TO)==BST_CHECKED) {
				SYSTEMTIME st;
				DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_TODATE), &st);

				if(first) {
					ss << " where";
					css << " where";
					first=false;
				}
				else {
					ss << " and";
					css << " and";
				}

				ss << " time<julianday('" << st << "', 'utc', '+1 day')";
				css << " time<julianday('" << st << "', 'utc', '+1 day')";
			}

			if(IsDlgButtonChecked(hwnd, IDC_PROTOCOL)==BST_CHECKED) {
				int protocol;
				switch(ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PROTOCOLLIST))) {
					case 0: protocol=IPPROTO_ICMP; break;
					case 1: protocol=IPPROTO_IGMP; break;
					case 2: protocol=IPPROTO_GGP; break;
					case 4: protocol=IPPROTO_TCP; break;
					case 5: protocol=IPPROTO_PUP; break;
					case 6: protocol=IPPROTO_UDP; break;
					case 7: protocol=IPPROTO_IDP; break;
					case 10: protocol=IPPROTO_ND; break;
					default: __assume(0);
				}

				if(first) {
					ss << " where";
					css << " where";
					first=false;
				}
				else {
					ss << " and";
					css << " and";
				}

				ss << " protocol=" << protocol;
				css << " protocol=" << protocol;
			}

			ss << (first?" where":" and") << " nameid=t_names.id;";
			css << ";";

			query=ss.str();
			cquery=css.str();
		}

		int ret;

		try {
			lock.enter();
			ret=g_con.executeint(cquery);
			cmd=boost::shared_ptr<sqlite3_command>(new sqlite3_command(g_con, query));
			reader=cmd->executereader();
		}
		catch(exception &ex) {
			ExceptionBox(hwnd, ex, __FILE__, __LINE__);
			return 0;
		}

		return ret;
	}

	bool Process() {
		try {
			if(reader.read()) {
				string name=reader.getstring(1);
				if(name=="n/a") name.clear();

				const char *protocol;
				switch(reader.getint(6)) {
					case IPPROTO_ICMP: protocol="ICMP"; break;
					case IPPROTO_IGMP: protocol="IGMP"; break;
					case IPPROTO_GGP: protocol="Gateway^2"; break;
					case IPPROTO_TCP: protocol="TCP"; break;
					case IPPROTO_PUP: protocol="PUP"; break;
					case IPPROTO_UDP: protocol="UDP"; break;
					case IPPROTO_IDP: protocol="XNS IDP"; break;
					case IPPROTO_ND: protocol="NetDisk"; break;
					default: protocol="Unknown"; break;
				}

				fs << reader.getstring(0) << ';'
					<< name << ';';

				FormatIp(fs, (unsigned int)reader.getint(2), (unsigned short)reader.getint(3));
				fs << ';';

				FormatIp(fs, (unsigned int)reader.getint(4), (unsigned short)reader.getint(5));
				fs << ';'
					<< protocol << ';'
					<< (reader.getint(7)?"Blocked":"Allowed") << endl;

				return true;
			}
			else return false;
		}
		catch(exception &ex) {
			ExceptionBox(hwnd, ex, __FILE__, __LINE__);
			return false;
		}
	}
};

static void ExportHistory_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_FROM:
			EnableWindow(GetDlgItem(hwnd, IDC_FROMDATE), IsDlgButtonChecked(hwnd, IDC_FROM)==BST_CHECKED);
			break;
		case IDC_TO:
			EnableWindow(GetDlgItem(hwnd, IDC_TODATE), IsDlgButtonChecked(hwnd, IDC_TO)==BST_CHECKED);
			break;
		case IDC_PROTOCOL:
			EnableWindow(GetDlgItem(hwnd, IDC_PROTOCOLLIST), IsDlgButtonChecked(hwnd, IDC_PROTOCOL)==BST_CHECKED);
			break;
		case IDC_ACTION:
			EnableWindow(GetDlgItem(hwnd, IDC_ACTIONLIST), IsDlgButtonChecked(hwnd, IDC_ACTION)==BST_CHECKED);
			break;
		case IDC_BROWSE: {
			TCHAR file[MAX_PATH]={0};

			OPENFILENAME ofn={0};
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=hwnd;
			ofn.lpstrFilter=_T("Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
			ofn.lpstrDefExt=_T("txt");
			ofn.lpstrFile=file;
			ofn.nMaxFile=MAX_PATH;
			ofn.Flags=OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;

			if(GetSaveFileName(&ofn)) {
				path p=path::relative_to(path::base_dir(), file);
				SetDlgItemText(hwnd, IDC_FILE, p.c_str());

				EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
			}
		} break;
		case IDOK: {
			tstring str=LoadString(IDS_EXPORTING);

			try {
				ExportFuncs funcs(hwnd);

				if(funcs.IsOpen()) {
					LoadingData data;

					data.Title=str.c_str();
					data.InitFunc=boost::bind(&ExportFuncs::Init, &funcs);
					data.ProcessFunc=boost::bind(&ExportFuncs::Process, &funcs);

					DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOADING), hwnd, Loading_DlgProc, (LPARAM)&data);

					EndDialog(hwnd, IDOK);
				}
				else {
					tstring file=GetDlgItemText(hwnd, IDC_FILE);
					tstring text=boost::str(tformat(LoadString(IDS_FILEOPENERRTEXT))%file);
					MessageBox(hwnd, text, IDS_FILEOPENERR, MB_ICONERROR|MB_OK);

					SetFocus(GetDlgItem(hwnd, IDC_BROWSE));
				}
			}
			catch(exception &ex) {
				ExceptionBox(hwnd, ex, __FILE__, __LINE__);
			}
		} break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
	}
}

static LPCTSTR const g_protocols[]={
	_T("ICMP"),
	_T("IGMP"),
	_T("Gateway^2"),
	_T("IPv4"),
	_T("TCP"),
	_T("PUP"),
	_T("UDP"),
	_T("XNS IDP"),
	_T("IPSec ESP"),
	_T("IPSec AH"),
	_T("NetDisk")
};
static const size_t g_numprotocols=sizeof(g_protocols)/sizeof(LPCTSTR);

static BOOL ExportHistory_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	g_hExportHistoryDlg = hwnd;

	HWND protocol=GetDlgItem(hwnd, IDC_PROTOCOLLIST);
	for(size_t i=0; i<g_numprotocols; i++)
		ComboBox_AddString(protocol, g_protocols[i]);
	ComboBox_SetCurSel(protocol, 4);

	HWND action=GetDlgItem(hwnd, IDC_ACTIONLIST);
	ComboBox_AddString(action, LoadString(IDS_BLOCKED).c_str());
	ComboBox_AddString(action, LoadString(IDS_ALLOWED).c_str());
	ComboBox_SetCurSel(action, 0);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	return TRUE;
}

INT_PTR CALLBACK ExportHistory_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_COMMAND, ExportHistory_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, ExportHistory_OnInitDialog);
			HANDLE_MSG(hwnd, WM_CLOSE, ExportHistory_OnClose);
			HANDLE_MSG(hwnd, WM_DESTROY, ExportHistory_OnDestroy);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(hwnd);
				return 1;
			default: return 0;
		}
	}
	catch(exception &ex) {
		UncaughtExceptionBox(hwnd, ex, __FILE__, __LINE__);
		return 0;
	}
	catch(...) {
		UncaughtExceptionBox(hwnd, __FILE__, __LINE__);
		return 0;
	}
}
