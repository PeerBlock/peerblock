; Inno Setup v5.3.4
;
; PeerBlock modifications copyright (C) 2009 PeerBlock, LLC
;
; Requirements:
; *Inno Setup QuickStart Pack
;   http://www.jrsoftware.org/isdl.php#qsp

#include "../pg2/versioninfo_setup.h"
#define app_version str(PB_VER_A) + "." + str(PB_VER_B) + "." + str(PB_VER_C) + "." + str(PB_BLDNUM)
#define simple_app_version str(PB_VER_A) + "." + str(PB_VER_B) + "." + str(PB_VER_C)
#define installer_build_date GetDateTimeString('dd/mm/yyyy', '.', '')


[Setup]
AppID={{015C5B35-B678-451C-9AEE-821E8D69621C}
AppName=PeerBlock
AppVersion={#= app_version}
AppVerName=PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM})
AppPublisher=PeerBlock, LLC
AppCopyright=Copyright © 2009, PeerBlock, LLC
AppPublisherURL=http://www.peerblock.com/
AppSupportURL=http://www.peerblock.com/
AppUpdatesURL=http://www.peerblock.com/
VersionInfoCompany=PeerBlock, LLC
VersionInfoCopyright=Copyright © 2009, PeerBlock, LLC
VersionInfoProductName=PeerBlock
VersionInfoProductVersion={#= app_version}
VersionInfoVersion={#= app_version}
VersionInfoDescription=PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM}) Setup
VersionInfoTextVersion={#= app_version}
DefaultDirName={pf}\PeerBlock
DefaultGroupName=PeerBlock
LicenseFile=..\license.txt
InfoBeforeFile=readme_before.rtf
OutputDir=..\Distribution
OutputBaseFilename=PeerBlock-Setup_v{#= simple_app_version}.r{#= PB_BLDNUM}
Compression=lzma/ultra64
InternalCompressLevel=ultra64
SolidCompression=True
MinVersion=0,5.0.2195
UninstallDisplayName=PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM})
UninstallDisplayIcon={app}\peerblock.exe
AppReadmeFile={app}\readme.rtf
WizardImageFile=WizModernImage.bmp
WizardSmallImageFile=WizModernSmallImage.bmp
DirExistsWarning=No
EnableDirDoesntExistWarning=False
AllowNoIcons=True
ShowTasksTreeLines=True
AlwaysShowDirOnReadyPage=True
AlwaysShowGroupOnReadyPage=True
WizardImageStretch=False
PrivilegesRequired=Admin
DisableDirPage=Auto
DisableProgramGroupPage=Auto
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
AppMutex=Global\PeerBlock


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
BeveledLabel=PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM}) built on {#= installer_build_date}


[CustomMessages]
; tsk=Task, msg=Message
; English
en.msg_SetupIsRunningWarning=PeerBlock Setup is already running!
en.msg_DeleteListsSettings=Do you also want to delete PeerBlock settings and lists?%nIf you plan on reinstalling PeerBlock you might not want to delete them.
en.tsk_other=Other tasks:
en.tsk_remove_startup=Remove PeerBlock from Windows startup
en.tsk_reset_settings=Reset PeerBlock's settings
en.tsk_startup_descr=Start PeerBlock on system startup
en.tsk_startup=Startup options:
en.tsk_use_PG2_settings=Use PeerGuardian2 settings and custom lists
en.run_visit_website=Visit PeerBlock's Website


[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.01; Flags: unchecked
Name: startup_task; Description: {cm:tsk_startup_descr}; GroupDescription: {cm:tsk_startup}; Check: StartupCheck() AND NOT StartupCheckOld(); Flags: unchecked
Name: startup_task; Description: {cm:tsk_startup_descr}; GroupDescription: {cm:tsk_startup}; Check: StartupCheck() AND StartupCheckOld()
Name: remove_startup_task; Description: {cm:tsk_remove_startup}; GroupDescription: {cm:tsk_startup}; Check: NOT StartupCheck(); Flags: unchecked
Name: reset_settings; Description: {cm:tsk_reset_settings}; GroupDescription: {cm:tsk_other}; Check: SettingsExist(); Flags: unchecked
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

;Copy PG2 settings and custom lists only if PG2 is installed and the user has choosed to do so
Source: {code:GetPG2Path}\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPG2Path}\*.p2p; DestDir: {app}; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPG2Path}\lists\*.p2b; DestDir: {app}\lists; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPG2Path}\lists\*.p2p; DestDir: {app}\lists; Tasks: use_pg2_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall

Source: ..\license.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\setup\readme.rtf; DestDir: {app}; Flags: ignoreversion
Source: ..\manual\*; DestDir: {app}\manual; Flags: ignoreversion recursesubdirs createallsubdirs


[Icons]
Name: {group}\PeerBlock; Filename: {app}\peerblock.exe; WorkingDir: {app}; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM})
Name: {group}\Uninstall PeerBlock; Filename: {app}\unins000.exe; WorkingDir: {app}; Comment: {cm:UninstallProgram,PeerBlock}
Name: {group}\Help and Support\Forums; Filename: http://forums.peerblock.com/; WorkingDir: {app}
Name: {group}\Help and Support\Homepage; Filename: http://www.peerblock.com/; WorkingDir: {app}
Name: {group}\Help and Support\License; Filename: {app}\license.txt; WorkingDir: {app}; Comment: PeerBlock's License
Name: {group}\Help and Support\ReadMe; Filename: {app}\readme.rtf; WorkingDir: {app}; Comment: PeerBlock's ReadMe
Name: {group}\Help and Support\User Manual; Filename: {app}\manual\index.htm; WorkingDir: {app}\manual; Comment: PeerBlock's User Manual
Name: {userdesktop}\PeerBlock; Filename: {app}\peerblock.exe; Tasks: desktopicon; WorkingDir: {app}; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM})
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock; Filename: {app}\peerblock.exe; Tasks: quicklaunchicon; WorkingDir: {app}; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_BLDNUM})


[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: PeerBlock; ValueData: {app}\peerblock.exe; Tasks: startup_task; Flags: uninsdeletevalue
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: PeerBlock; Tasks: reset_settings remove_startup_task; Flags: deletevalue uninsdeletevalue


[Run]
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: http://www.peerblock.com/; Description: {cm:run_visit_website}; Flags: nowait postinstall skipifsilent shellexec runascurrentuser unchecked


[InstallDelete]
// During installation, delete older files in install folder: 
Name: {app}\peerblock.url; Type: files
Name: {app}\readme.rtf; Type: files
Name: {app}\license.txt; Type: files

Name: {app}\peerblock.conf; Type: files; Tasks: reset_settings
Name: {app}\cache.p2b; Type: files; Tasks: reset_settings
Name: {app}\history.db; Type: files; Tasks: reset_settings
Name: {app}\peerblock.log; Type: files; Tasks: reset_settings
Name: {app}\peerblock.dmp; Type: files; Tasks: reset_settings

Name: {userdesktop}\PeerBlock.lnk; Type: files; Check: NOT IsTaskSelected('desktopicon')
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock.lnk; Type: files; Check: NOT IsTaskSelected('quicklaunchicon')

// During installation, delete older start menu entries: 
Name: {group}\License.lnk; Type: files
Name: {group}\PeerBlock on the Web.url; Type: files
Name: {group}\Help and Support\PeerBlock on the Web.url; Type: files
Name: {group}\ReadMe.lnk; Type: files
Name: {group}\Uninstall.lnk; Type: files


[UninstallDelete]
Name: {app}\manual; Type: filesandordirs


[Code]
var
  PG2PathKeyName, PG2Path, PG2PathValueName: String;


// Create a mutex for the installer
const installer_mutex_name = 'peerblock_setup_mutex';


// Check if PeerBlock is configured to run on startup, in order to control
// startup choice within the installer
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
begin
	PG2Path := '';
    PG2PathKeyName := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\PeerGuardian_is1';
    PG2PathValueName := 'Inno Setup: App Path';

    if not RegQueryStringValue(HKLM, PG2PathKeyName, PG2PathValueName, PG2Path) then
    RegQueryStringValue(HKCU, PG2PathKeyName, PG2PathValueName, PG2Path);
	Result := PG2Path;
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


// Delete old startup registry entry while installing
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then begin
    if StartupCheckOld then begin
	  RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerGuardian');
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
	// Create a mutex for the installer. If it's already running display a message and stop installation
	Result := True;
	if CheckForMutexes(installer_mutex_name) then begin
		if not WizardSilent() then
			MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK);
			Result := False;
		end
		else begin
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
		end
		else begin
		CreateMutex(installer_mutex_name);
	end;
end;
