(*
;	PeerBlock modifications copyright (C) 2009-2010 PeerBlock, LLC
;
;	This software is provided 'as-is', without any express or implied
;	warranty.  In no event will the authors be held liable for any damages
;	arising from the use of this software.
;
;	Permission is granted to anyone to use this software for any purpose,
;	including commercial applications, and to alter it and redistribute it
;	freely, subject to the following restrictions:
;
;	1. The origin of this software must not be misrepresented; you must not
;		claim that you wrote the original software. If you use this software
;		in a product, an acknowledgment in the product documentation would be
;		appreciated but is not required.
;	2. Altered source versions must be plainly marked as such, and must not be
;		misrepresented as being the original software.
;	3. This notice may not be removed or altered from any source distribution.
;
*)

[Code]
////////////////////////////////////////
//   Global variables and constants   //
////////////////////////////////////////

var
  PGPath: String;
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


procedure RemoveUserFiles();
begin
  DelTree(ExpandConstant('{app}\archives\*.log'), False, True, False);
  RemoveDir(ExpandConstant('{app}\archives\'));
  DelTree(ExpandConstant('{app}\lists\*.list'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.list.tmp'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.list.failed'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.p2b'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.p2p'), False, True, False);
  RemoveDir(ExpandConstant('{app}\lists\'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.bak'));
end;


procedure RemoveMiscFiles();
begin
  DeleteFile(ExpandConstant('{app}\cache.p2b'));
  DeleteFile(ExpandConstant('{app}\history.db'));
  DeleteFile(ExpandConstant('{app}\peerblock.dmp'));
  DeleteFile(ExpandConstant('{app}\peerblock.log'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.tmp'));
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
