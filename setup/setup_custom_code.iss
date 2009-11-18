[Code]
////////////////////////////////////////
//   Global variables and constants   //
////////////////////////////////////////

var
  PGPath: String;
  SetResetTask: Boolean;
  WinVer: TWindowsVersion;
const
  installer_mutex_name = 'peerblock_setup_mutex';
  PGUninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\PeerGuardian_is1';


////////////////////////////////////////
//  Custom functions and procedures   //
////////////////////////////////////////

// Get PeerGuardian's installation path
function GetPGPath(Default: String): String;
var
  PGUninstallValue: String;
begin
  PGPath := '';
  PGUninstallValue := 'Inno Setup: App Path';

  if not RegQueryStringValue(HKLM, PGUninstallKey, PGUninstallValue, PGPath) then
  RegQueryStringValue(HKCU, PGUninstallKey, PGUninstallValue, PGPath);
  Result := PGPath;
end;


// Check if PeerGuardian is installed
function IsPGInstalled(): Boolean;
begin
  Result := False;
  if RegKeyExists(HKLM, PGUninstallKey) or RegKeyExists(HKCU, PGUninstallKey) then
  Result := True;
end;


// Check if PeerBlock is configured to run on startup,
// looking for the old registry value "PeerGuardian"
function OldStartupCheck(): Boolean;
var
  svalue: String;
begin
  Result := False;
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerGuardian', svalue) then begin
    if svalue = (ExpandConstant('{app}\peerblock.exe')) then
    Result := True;
  end;
end;


// Check if PeerBlock's settings exist
function SettingsExist(): Boolean;
begin
  Result := False;
  if FileExists(ExpandConstant('{app}\peerblock.conf')) then
  Result := True;
end;


// Check if PeerBlock is configured to run on startup
function StartupCheck(): Boolean;
begin
  Result := False;
  if RegValueExists(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerBlock') then
  Result := True;
end;


// Function to retrieve PeerGuardian's uninstall string and uninstall it
function UninstallPG(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
// Return Values:
// 0 - no idea
// 1 - can't find the registry key (probably no previous version installed)
// 2 - uninstall string is empty
// 3 - error executing the UnInstallString
// 4 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  sUnInstallString := '';

  // get the uninstall string of the old app
  if RegQueryStringValue(HKLM, PGUninstallKey, 'UninstallString', sUnInstallString) then begin
      if sUnInstallString <> '' then begin
          sUnInstallString := RemoveQuotes(sUnInstallString);
          if Exec(sUnInstallString, '/SILENT /VERYSILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
              Result := 4
          else
              Result := 3;
          end else
              Result := 2;
  end else
      Result := 1;
end;


// Bypass Inno Setup UsePreviousTasks directive only for the "reset_settings",
// "uninstall_pg" and "startup_task" tasks
procedure CheckUncheckTasks();
var
  i, j, k: Integer;
begin
  i := WizardForm.TasksList.Items.IndexOf(ExpandConstant('{cm:tsk_reset_settings}'));
  j := WizardForm.TasksList.Items.IndexOf(ExpandConstant('{cm:tsk_uninstall_pg}'));
  k := WizardForm.TasksList.Items.IndexOf(ExpandConstant('{cm:tsk_startup_descr}'));

  if(i <> -1) then begin
    WizardForm.TasksList.Checked[i] := False;
  end;
  if(j <> -1) then begin
    WizardForm.TasksList.Checked[j] := False;
  end;
  if(k <> -1) and OldStartupCheck then begin
    WizardForm.TasksList.Checked[k] := True;
  end;
end;


procedure RemoveUserFiles();
begin
  DelTree(ExpandConstant('{app}\archives\*.log'), False, True, False);
  RemoveDir(ExpandConstant('{app}\archives\'));
  DelTree(ExpandConstant('{app}\lists\*.list'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.list.tmp'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.p2b'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.p2p'), False, True, False);
  RemoveDir(ExpandConstant('{app}\lists\'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf'));
end;


procedure RemoveMiscFiles();
begin
  DeleteFile(ExpandConstant('{app}\cache.p2b'));
  DeleteFile(ExpandConstant('{app}\history.db'));
  DeleteFile(ExpandConstant('{app}\peerblock.dmp'));
  DeleteFile(ExpandConstant('{app}\peerblock.log'));
end;


// Find PeerGuardian's WindowName, kill the process and then uninstall it
procedure KillAndUninstallPG();
var
  Wnd: HWND;
begin
  Wnd := FindWindowByWindowName('PeerGuardian 2');
  if Wnd <> 0 then begin
    PostMessage(Wnd, 18, 0, 0); // WM_QUIT
  end;
  UninstallPG;
end;


///////////////////////////////////////////
//  Functions to check Windows versions  //
///////////////////////////////////////////

function Is2K(): Boolean;
begin
  GetWindowsVersionEx(WinVer);
  Result := WinVer.NTPlatform and (WinVer.Major < 6) and not Is64BitInstallMode;
end;


function IsXP64(): Boolean;
begin
  GetWindowsVersionEx(WinVer);
  Result := WinVer.NTPlatform and (WinVer.Major < 6) and Is64BitInstallMode;
end;


function IsVista(): Boolean;
begin
  GetWindowsVersionEx(WinVer);
  Result := WinVer.NTPlatform and (WinVer.Major >= 6) and not Is64BitInstallMode;
end;


function IsVista64(): Boolean;
begin
  GetWindowsVersionEx(WinVer);
  Result := WinVer.NTPlatform and (WinVer.Major >= 6) and Is64BitInstallMode;
end;