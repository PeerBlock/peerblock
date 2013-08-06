(*
;  PeerBlock copyright (C) 2009-2013 PeerBlock, LLC
;
;  This software is provided 'as-is', without any express or implied
;  warranty.  In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Permission is granted to anyone to use this software for any purpose,
;  including commercial applications, and to alter it and redistribute it
;  freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not
;     claim that you wrote the original software. If you use this software
;     in a product, an acknowledgment in the product documentation would be
;     appreciated but is not required.
;  2. Altered source versions must be plainly marked as such, and must not be
;     misrepresented as being the original software.
;  3. This notice may not be removed or altered from any source distribution.
;
;  $Id$
*)


[Code]
///////////////////////////////////////
//  Global variables and constants   //
///////////////////////////////////////

var
  sPGPath: String;
const
  PGUninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\PeerGuardian_is1';


///////////////////////////////////////
//  Custom functions and procedures   /
///////////////////////////////////////

#if defined(sse_required) || defined(sse2_required)
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';
#endif


// Get PeerGuardian's installation path
function GetPGPath(Default: String): String;
begin
  sPGPath := '';

  if not RegQueryStringValue(HKLM, PGUninstallKey, 'Inno Setup: App Path', sPGPath) then
    RegQueryStringValue(HKCU, PGUninstallKey, 'Inno Setup: App Path', sPGPath);
    Result := sPGPath;
end;


// Check if PeerGuardian is installed
function IsPGInstalled(): Boolean;
begin
  if RegKeyExists(HKLM, PGUninstallKey) or RegKeyExists(HKCU, PGUninstallKey) then begin
    Log('Custom Code: Found PG2 uninstall registry key');
    Result := True;
  end else
    Result := False;
end;


function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


#if defined(sse_required)
function IsSSESupported(): Boolean;
begin
  // PF_XMMI_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(6);
end;

#elif defined(sse2_required)

function IsSSE2Supported(): Boolean;
begin
  // PF_XMMI64_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(10);
end;

#endif


function CustomListsExist(): Boolean;
var
  FindRec: TFindRec;
begin
  if FindFirst(ExpandConstant('{app}\lists\*.p2b'), FindRec) or FileExists(ExpandConstant('{app}\lists\*.p2p')) then begin
    Log('Custom Code: Custom lists exist');
    Result := True;
    FindClose(FindRec);
  end else
    Result := False;
end;


function ListsExist(): Boolean;
var
  FindRec: TFindRec;
begin
  if FindFirst(ExpandConstant('{app}\lists\*.list'), FindRec) then begin
    Log('Custom Code: Lists exist');
    Result := True;
    FindClose(FindRec);
  end else
    Result := False;
end;


function LogsExist(): Boolean;
var
  FindRec: TFindRec;
begin
  if FindFirst(ExpandConstant('{app}\archives\*.log'), FindRec) or FileExists(ExpandConstant('{app}\peerblock.log')) then begin
    Log('Custom Code: Logs exist');
    Result := True;
    FindClose(FindRec);
  end else
    Result := False;
end;


function MiscFilesExist(): Boolean;
begin
  if FileExists(ExpandConstant('{app}\cache.p2b')) or FileExists(ExpandConstant('{app}\history.db')) then begin
    Log('Custom Code: Misc files exist');
    Result := True;
  end else
    Result := False;
end;


// Check if PeerBlock is configured to run on startup,
// looking for the old registry value "PeerGuardian"
function OldStartupCheck(): Boolean;
var
  svalue: String;
begin
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerGuardian', svalue) and
  (svalue = (ExpandConstant('{app}\peerblock.exe'))) then begin
    Log('Custom Code: Old Startup entry was found');
    Result := True;
  end else
    Result := False;
end;


// Check if PeerBlock's settings exist
function SettingsExist(): Boolean;
begin
  if FileExists(ExpandConstant('{app}\peerblock.conf')) then begin
    Log('Custom Code: Settings exist');
    Result := True;
  end else
    Result := False;
end;


// Check if PeerBlock is configured to run on startup
function StartupCheck(): Boolean;
var
  svalue: String;
begin
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerBlock', svalue) and
  (svalue = (ExpandConstant('{app}\peerblock.exe'))) then begin
    Log('Custom Code: PeerBlock is configured to run on startup');
    Result := True;
  end else
    Result := False;
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


// Find PeerGuardian's WindowName, kill the process and then uninstall it
procedure KillAndUninstallPG();
var
  Wnd: HWND;
begin
  Wnd := FindWindowByWindowName('PeerGuardian 2');
  if Wnd <> 0 then begin
    Log('Custom Code: Trying to kill PG2');
    PostMessage(Wnd, 18, 0, 0); // WM_QUIT
    Log('Custom Code: Trying to uninstall PG2');
    UninstallPG();
  end;
end;


procedure RemoveCustomLists();
begin
  DelTree(ExpandConstant('{app}\lists\*.p2b'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.p2p'), False, True, False);
  RemoveDir(ExpandConstant('{app}\lists\'));
end;


procedure RemoveLists();
begin
  DelTree(ExpandConstant('{app}\lists\*.list'), False, True, False);
  RemoveDir(ExpandConstant('{app}\lists\'));
end;


procedure RemoveLogs();
begin
  DelTree(ExpandConstant('{app}\archives\*.log'), False, True, False);
  DeleteFile(ExpandConstant('{app}\peerblock.log'));
  RemoveDir(ExpandConstant('{app}\archives\'));
end;


procedure RemoveMiscFiles();
begin
  DelTree(ExpandConstant('{app}\lists\*.list.failed'), False, True, False);
  DelTree(ExpandConstant('{app}\lists\*.list.tmp'), False, True, False);
  RemoveDir(ExpandConstant('{app}\lists\'));
  DeleteFile(ExpandConstant('{app}\cache.p2b'));
  DeleteFile(ExpandConstant('{app}\history.db'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.bak.failed'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.bak.tmp'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.failed'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.tmp'));
  DeleteFile(ExpandConstant('{app}\peerblock.dmp'));
  DeleteFile(ExpandConstant('{app}\pg2.conf.failed'));
end;


procedure RemoveSettings();
begin
  DeleteFile(ExpandConstant('{app}\peerblock.conf'));
  DeleteFile(ExpandConstant('{app}\peerblock.conf.bak'));
end;
