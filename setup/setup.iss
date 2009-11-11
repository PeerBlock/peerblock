; Inno Setup v5.3.5
;
; PeerBlock modifications copyright (C) 2009 PeerBlock, LLC
;
; Requirements:
; *Inno Setup QuickStart Pack
;   http://www.jrsoftware.org/isdl.php#qsp

#include "../pb/versioninfo_setup.h"
#define app_version str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "." + str(PB_VER_BUGFIX) + "." + str(PB_VER_BUILDNUM)

; Uncomment one of the #define simple_app_version and comment all other
; E.g. 1.0+
#define simple_app_version str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "+"
; E.g. 1.0.1
;#define simple_app_version str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "." + str(PB_VER_BUGFIX)
; E.g. 1.0.1+
;#define simple_app_version str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "." + str(PB_VER_BUGFIX) + "+"

#define installer_build_date GetDateTimeString('mmm, d yyyy', '', '')


[Setup]
AppID={{015C5B35-B678-451C-9AEE-821E8D69621C}
AppName=PeerBlock
AppVersion={#= app_version}
AppVerName=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
AppPublisher=PeerBlock, LLC
AppCopyright=Copyright © 2009, PeerBlock, LLC
AppPublisherURL=http://www.peerblock.com/
AppSupportURL=http://www.peerblock.com/
AppUpdatesURL=http://www.peerblock.com/
AppContact=http://www.peerblock.com/
VersionInfoCompany=PeerBlock, LLC
VersionInfoCopyright=Copyright © 2009, PeerBlock, LLC
VersionInfoProductName=PeerBlock
VersionInfoProductVersion={#= app_version}
VersionInfoVersion={#= app_version}
VersionInfoDescription=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM}) Setup
VersionInfoTextVersion={#= app_version}
DefaultDirName={pf}\PeerBlock
DefaultGroupName=PeerBlock
LicenseFile=..\license.txt
InfoBeforeFile=readme_before.rtf
OutputDir=..\Distribution
OutputBaseFilename=PeerBlock-Setup_v{#= simple_app_version}_r{#= PB_VER_BUILDNUM}
Compression=lzma/ultra64
InternalCompressLevel=ultra64
SolidCompression=yes
MinVersion=0,5.0.2195
UninstallDisplayName=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
UninstallDisplayIcon={app}\peerblock.exe
AppReadmeFile={app}\readme.rtf
WizardImageFile=WizModernImage.bmp
WizardSmallImageFile=WizModernSmallImage.bmp
SetupIconFile=pg2.ico
DirExistsWarning=No
EnableDirDoesntExistWarning=no
AllowNoIcons=yes
ShowTasksTreeLines=yes
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
WizardImageStretch=no
PrivilegesRequired=admin
DisableDirPage=auto
DisableProgramGroupPage=auto
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
AppMutex=Global\PeerBlock


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
BeveledLabel=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM}) built on {#= installer_build_date}


[CustomMessages]
; tsk=Task, msg=Message
; English
en.msg_DeleteListsSettings=Do you also want to delete PeerBlock settings and lists?%nIf you plan on reinstalling PeerBlock you might not want to delete them.
en.msg_SetupIsRunningWarning=PeerBlock Setup is already running!
en.run_visit_website=Visit PeerBlock's Website
en.tsk_other=Other tasks:
en.tsk_remove_startup=Remove PeerBlock from Windows startup
en.tsk_reset_settings=Reset PeerBlock's settings and delete all custom lists
en.tsk_startup=Startup options:
en.tsk_startup_descr=Start PeerBlock on system startup
en.tsk_uninstall_pg2=Uninstall PeerGuardian2 after PeerBlock's installation
en.tsk_use_PG2_settings=Use PeerGuardian2 settings and custom lists


[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.01; Flags: unchecked
Name: startup_task; Description: {cm:tsk_startup_descr}; GroupDescription: {cm:tsk_startup}; Check: StartupCheck() AND NOT StartupCheckOld(); Flags: unchecked
Name: startup_task; Description: {cm:tsk_startup_descr}; GroupDescription: {cm:tsk_startup}; Check: StartupCheck() AND StartupCheckOld()
Name: remove_startup_task; Description: {cm:tsk_remove_startup}; GroupDescription: {cm:tsk_startup}; Check: NOT StartupCheck(); Flags: unchecked
Name: reset_settings; Description: {cm:tsk_reset_settings}; GroupDescription: {cm:tsk_other}; Check: SettingsExist(); Flags: unchecked
Name: uninstall_pg2; Description: {cm:tsk_uninstall_pg2}; GroupDescription: {cm:tsk_other}; Check: IsPG2Installed(); Flags: unchecked
Name: use_pg2_settings; Description: {cm:tsk_use_PG2_settings}; GroupDescription: {cm:tsk_other}; Check: FileExists(ExpandConstant('{code:GetPG2Path}\pg2.conf')) AND NOT SettingsExist()


[Files]
; Win2k files
Source: ..\win32\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: ..\win32\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: Is2k

; WinXP x64 files
Source: ..\x64\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: ..\x64\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsXP64

; Vista files
Source: ..\win32\release (Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: ..\win32\release (Vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista

; Vista x64 files
Source: ..\x64\release (Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: ..\x64\release (Vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista64

; Copy PG2 settings and custom lists only if PG2 is installed and the user has choosed to do so
Source: {code:GetPG2Path}\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPG2Path}\*.p2p; DestDir: {app}; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPG2Path}\lists\*.p2b; DestDir: {app}\lists; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPG2Path}\lists\*.p2p; DestDir: {app}\lists; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall

Source: ..\license.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\setup\readme.rtf; DestDir: {app}; Flags: ignoreversion


[Icons]
Name: {group}\PeerBlock; Filename: {app}\peerblock.exe; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
Name: {group}\Uninstall PeerBlock; Filename: {app}\unins000.exe; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 1; Comment: {cm:UninstallProgram,PeerBlock}
Name: {group}\Help and Support\Forums; Filename: http://forums.peerblock.com/; WorkingDir: {app}
Name: {group}\Help and Support\Homepage; Filename: http://www.peerblock.com/; WorkingDir: {app}
Name: {group}\Help and Support\ReadMe; Filename: {app}\readme.rtf; WorkingDir: {app}; Comment: PeerBlock's ReadMe
Name: {group}\Help and Support\User Manual; Filename: http://www.peerblock.com/userguide; WorkingDir: {app}; Comment: PeerBlock's User Manual
Name: {userdesktop}\PeerBlock; Filename: {app}\peerblock.exe; Tasks: desktopicon; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock; Filename: {app}\peerblock.exe; Tasks: quicklaunchicon; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})


[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: PeerBlock; ValueData: {app}\peerblock.exe; Tasks: startup_task; Flags: uninsdeletevalue
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: PeerBlock; Tasks: reset_settings remove_startup_task; Flags: deletevalue uninsdeletevalue


[Run]
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: http://www.peerblock.com/; Description: {cm:run_visit_website}; Flags: nowait postinstall skipifsilent shellexec runascurrentuser unchecked


[InstallDelete]
; During installation, delete old files in install folder
Name: {app}\license.txt; Type: files
Name: {app}\peerblock.url; Type: files

; ...also, delete Manual files
Name: {app}\manual\index.htm; Type: files
Name: {app}\manual\FAQ\index.htm; Type: files
Name: {app}\manual\FAQ; Type: dirifempty
Name: {app}\manual\how_to_use\installation.htm; Type: files
Name: {app}\manual\how_to_use\quick_start_wizard.htm; Type: files
Name: {app}\manual\how_to_use\selecting_appropriate_lists.htm; Type: files
Name: {app}\manual\how_to_use\using_lists.htm; Type: files
Name: {app}\manual\how_to_use; Type: dirifempty
Name: {app}\manual\images\*.png; Type: files
Name: {app}\manual\images; Type: dirifempty
Name: {app}\manual\introduction; Type: dirifempty
Name: {app}\manual\settings\index.htm; Type: files
Name: {app}\manual\settings; Type: dirifempty
Name: {app}\manual\what_you_can_do; Type: dirifempty
Name: {app}\manual; Type: dirifempty

; ...and finally, delete old start menu entries
Name: {group}\License.lnk; Type: files
Name: {group}\PeerBlock on the Web.url; Type: files
Name: {group}\Help and Support\License.lnk; Type: files
Name: {group}\Help and Support\PeerBlock on the Web.url; Type: files
Name: {group}\ReadMe.lnk; Type: files
Name: {group}\Uninstall.lnk; Type: files

Name: {app}\cache.p2b; Type: files; Tasks: reset_settings
Name: {app}\history.db; Type: files; Tasks: reset_settings
Name: {app}\peerblock.conf; Type: files; Tasks: reset_settings
Name: {app}\peerblock.dmp; Type: files; Tasks: reset_settings
Name: {app}\peerblock.log; Type: files; Tasks: reset_settings
Name: {app}\*.p2p; Type: files; Tasks: reset_settings
Name: {app}\lists\*.list; Type: files; Tasks: reset_settings
Name: {app}\lists\*.p2b; Type: files; Tasks: reset_settings
Name: {app}\lists\*.p2p; Type: files; Tasks: reset_settings
Name: {app}\lists; Type: dirifempty; Tasks: reset_settings

Name: {userdesktop}\PeerBlock.lnk; Type: files; Tasks: NOT desktopicon
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock.lnk; Type: files; Tasks: NOT quicklaunchicon


[Code]
// Global variables
var
  PG2PathKeyName, PG2Path: String;
  SetResetSettings: Boolean;

// Create a mutex for the installer
const installer_mutex_name = 'peerblock_setup_mutex';


// Check if PeerBlock is configured to run on startup,
// in order to control startup choice within the installer
function StartupCheck(): Boolean;
begin
  Result := True;
  if RegValueExists(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerBlock') then
  Result := False;
end;


// Check if PeerBlock is configured to run on startup,
// looking for the old registry value "PeerGuardian"
function StartupCheckOld(): Boolean;
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


// Get PeerGuardian's installation path
function GetPG2Path(S: String): String;
var
  PG2PathValueName: String;
begin
    PG2Path := '';
    PG2PathKeyName := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\PeerGuardian_is1';
    PG2PathValueName := 'Inno Setup: App Path';

    if not RegQueryStringValue(HKLM, PG2PathKeyName, PG2PathValueName, PG2Path) then
    RegQueryStringValue(HKCU, PG2PathKeyName, PG2PathValueName, PG2Path);
	Result := PG2Path;
end;


// Check if PeerGuardian is installed
function IsPG2Installed(): Boolean;
begin
  Result := False;
  if RegKeyExists(HKLM, PG2PathKeyName) or RegKeyExists(HKCU, PG2PathKeyName) then
  Result := True;
end;


// Function to uninstall PeerGuardian
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
    if RegQueryStringValue(HKLM, PG2PathKeyName, 'UninstallString', sUnInstallString) then begin
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


// Functions to check Windows versions
function Is2k: Boolean;
var
  ver: TWindowsVersion;
begin
  GetWindowsVersionEx(ver);
  Result := UsingWinNT and (ver.Major < 6) and not Is64BitInstallMode;
end;


function IsXP64: Boolean;
var
  ver: TWindowsVersion;
begin
  GetWindowsVersionEx(ver);
  Result := UsingWinNT and (ver.Major < 6) and Is64BitInstallMode;
end;


function IsVista: Boolean;
var
  ver: TWindowsVersion;
begin
  GetWindowsVersionEx(ver);
  Result := UsingWinNT and (ver.Major >= 6) and not Is64BitInstallMode;
end;


function IsVista64: Boolean;
var
  ver: TWindowsVersion;
begin
  GetWindowsVersionEx(ver);
  Result := UsingWinNT and (ver.Major >= 6) and Is64BitInstallMode;
end;


// Always have the "Reset Settings and lists" task disabled
procedure CurPageChanged(CurPageID: Integer);
var
  i: Integer;
begin
  if not SetResetSettings and (CurPageID = wpSelectTasks) then begin
    i := WizardForm.TasksList.Items.IndexOf(ExpandConstant('{cm:tsk_reset_settings}'));
    if(i <> -1) then begin
      WizardForm.TasksList.Checked[i] := False;
    end;
    SetResetSettings := True;
  end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
  Wnd: HWND;
begin
  if CurStep = ssPostInstall then begin
  // Delete old PeerBlock's startup registry entry and uninstall PeerGuardian if the task is selected
    if StartupCheckOld then begin
      RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerGuardian');
    end;
    // Find PeerGuardian's WindowName and kill the process before uninstalling it
    if IsTaskSelected('uninstall_pg2') then begin
      Wnd := FindWindowByWindowName('PeerGuardian 2');
      if Wnd <> 0 then
      PostMessage(Wnd, 18, 0, 0); // WM_QUIT
      UninstallPG;
    end;
  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
// When uninstalling, ask user if they want to delete PeerBlock's logs and settings
  if CurUninstallStep = usUninstall then begin
    if fileExists(ExpandConstant('{app}\peerblock.conf')) then begin
      if MsgBox(ExpandConstant('{cm:msg_DeleteListsSettings}'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then begin
        DelTree(ExpandConstant('{app}\lists\*.list'), False, True, False);
        DelTree(ExpandConstant('{app}\lists\*.p2b'), False, True, False);
        DelTree(ExpandConstant('{app}\lists\*.p2p'), False, True, False);
        RemoveDir(ExpandConstant('{app}\lists\'))
        DeleteFile(ExpandConstant('{app}\peerblock.conf'));
      end;
    end;
    DelTree(ExpandConstant('{app}\archives\*.log'), False, True, False);
    RemoveDir(ExpandConstant('{app}\archives\'))
    DeleteFile(ExpandConstant('{app}\cache.p2b'));
    DeleteFile(ExpandConstant('{app}\history.db'));
    DeleteFile(ExpandConstant('{app}\peerblock.dmp'));
    DeleteFile(ExpandConstant('{app}\peerblock.log'));
  end;
end;


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer.
  // If it's already running display a message and stop installation
  Result := True;
  if CheckForMutexes(installer_mutex_name) then begin
    if not WizardSilent() then
      MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK);
      Result := False;
  end else begin
    CreateMutex(installer_mutex_name);
  end;
end;


function InitializeUninstall(): Boolean;
begin
  Result := True;
  if CheckForMutexes(installer_mutex_name) then begin
    if not WizardSilent() then
      MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK);
    Result := False;
  end else begin
    CreateMutex(installer_mutex_name);
  end;
end;