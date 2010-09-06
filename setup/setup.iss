;
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
;
; Requirements:
; *Inno Setup QuickStart Pack v5.3.10(+): http://www.jrsoftware.org/isdl.php#qsp


#define dummy Exec("versioninfo_setup.bat","","",1,SW_HIDE)
#include "versioninfo_setup.h"

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
AppCopyright=Copyright © 2009-2010, PeerBlock, LLC
AppPublisherURL=http://www.peerblock.com/
AppSupportURL=http://www.peerblock.com/support
AppUpdatesURL=http://www.peerblock.com/releases
AppContact=http://www.peerblock.com/
VersionInfoCompany=PeerBlock, LLC
VersionInfoCopyright=Copyright © 2009-2010, PeerBlock, LLC
VersionInfoProductName=PeerBlock
VersionInfoProductVersion={#= app_version}
VersionInfoVersion={#= app_version}
VersionInfoDescription=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM}) Setup
VersionInfoTextVersion={#= app_version}
DefaultDirName={pf}\PeerBlock
DefaultGroupName=PeerBlock
LicenseFile=..\license.txt
InfoBeforeFile=readme_before.rtf
OutputDir=.
OutputBaseFilename=PeerBlock-Setup_v{#= simple_app_version}_r{#= PB_VER_BUILDNUM}
Compression=lzma2/max
InternalCompressLevel=max
SolidCompression=yes
MinVersion=0,5.0.2195
UninstallDisplayName=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
UninstallDisplayIcon={app}\peerblock.exe
AppReadmeFile={app}\readme.rtf
WizardImageFile=icons\WizModernImage.bmp
WizardSmallImageFile=icons\WizModernSmallImage.bmp
SetupIconFile=icons\pb.ico
DirExistsWarning=no
EnableDirDoesntExistWarning=no
AllowNoIcons=yes
ShowTasksTreeLines=yes
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
PrivilegesRequired=admin
DisableDirPage=auto
DisableProgramGroupPage=auto
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
AppMutex=Global\PeerBlock

#if GetEnv("PB_CERT")
SignTool=Standard
#endif


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
BeveledLabel=PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM}) built on {#= installer_build_date}


[CustomMessages]
; tsk=Task, msg=Message
; English
en.msg_DeleteListsSettings=Do you also want to delete PeerBlock's settings and lists?%nIf you plan on reinstalling PeerBlock you might not want to delete them.
en.msg_SetupIsRunningWarning=PeerBlock Setup is already running!
en.msg_servicemanager=The service manager is not available
en.msg_servicemanager2=Only NT based systems support services
en.run_visit_website=Visit PeerBlock's Website
en.tsk_other=Other tasks:
en.tsk_remove_startup=Remove PeerBlock from Windows startup
en.tsk_reset_settings=Reset PeerBlock's settings and delete all custom lists
en.tsk_startup=Startup options:
en.tsk_startup_descr=Start PeerBlock on system startup
en.tsk_uninstall_pg=Uninstall PeerGuardian after PeerBlock's installation
en.tsk_use_PG_settings=Use PeerGuardian's settings and custom lists


[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.01; Flags: unchecked
Name: startup_task; Description: {cm:tsk_startup_descr}; GroupDescription: {cm:tsk_startup}; Check: NOT StartupCheck(); Flags: checkedonce unchecked
Name: remove_startup_task; Description: {cm:tsk_remove_startup}; GroupDescription: {cm:tsk_startup}; Check: StartupCheck(); Flags: checkedonce unchecked
Name: reset_settings; Description: {cm:tsk_reset_settings}; GroupDescription: {cm:tsk_other}; Check: SettingsExist(); Flags: checkedonce unchecked
Name: uninstall_pg; Description: {cm:tsk_uninstall_pg}; GroupDescription: {cm:tsk_other}; Check: IsPGInstalled(); Flags: checkedonce unchecked
Name: use_pg_settings; Description: {cm:tsk_use_PG_settings}; GroupDescription: {cm:tsk_other}; Check: FileExists(ExpandConstant('{code:GetPGPath}\pg2.conf')) AND NOT SettingsExist()


[Files]
; Win2K files
Source: ..\Win32\Release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: Is2K()
Source: ..\Win32\Release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: Is2K()

; WinXP x64 files
Source: ..\x64\Release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsXP64()
Source: ..\x64\Release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsXP64()

; Vista files
Source: ..\Win32\Release (Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista()
Source: ..\Win32\Release (Vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista()

; Vista x64 files
Source: ..\x64\Release (Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista64()
Source: ..\x64\Release (Vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista64()

; Copy PG settings and custom lists only if PG is installed and the user has choosed to do so
Source: {code:GetPGPath}\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPGPath}\*.p2p; DestDir: {app}; Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPGPath}\lists\*.p2b; DestDir: {app}\lists; Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPGPath}\lists\*.p2p; DestDir: {app}\lists; Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall

Source: ..\license.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\setup\readme.rtf; DestDir: {app}; Flags: ignoreversion


[Icons]
Name: {group}\PeerBlock; Filename: {app}\peerblock.exe; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
Name: {group}\Uninstall PeerBlock; Filename: {uninstallexe}; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 1; Comment: {cm:UninstallProgram,PeerBlock}
Name: {group}\Help and Support\Forums; Filename: http://forums.peerblock.com/
Name: {group}\Help and Support\Homepage; Filename: http://www.peerblock.com/
Name: {group}\Help and Support\ReadMe; Filename: {app}\readme.rtf; WorkingDir: {app}; Comment: PeerBlock's ReadMe
Name: {group}\Help and Support\User Manual; Filename: http://www.peerblock.com/userguide
Name: {userdesktop}\PeerBlock; Filename: {app}\peerblock.exe; Tasks: desktopicon; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock; Filename: {app}\peerblock.exe; Tasks: quicklaunchicon; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= PB_VER_BUILDNUM})


[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: PeerBlock; ValueData: {app}\peerblock.exe; Tasks: startup_task; Flags: uninsdeletevalue
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: PeerBlock; Tasks: reset_settings remove_startup_task; Flags: deletevalue uninsdeletevalue


[Run]
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; WorkingDir: {app}; Flags: nowait postinstall skipifsilent runascurrentuser
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

; While we are at it, delete any shortcut which is not selected
Name: {userdesktop}\PeerBlock.lnk; Type: files; Check: NOT IsTaskSelected('desktopicon') AND IsUpdate()
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock.lnk; Type: files; Check: NOT IsTaskSelected('quicklaunchicon') AND IsUpdate()


[Code]
// Include custom installer code
#include 'setup_custom_code.iss'
#include 'setup_services.iss'

///////////////////////////////////////////
//  Inno Setup functions and procedures  //
///////////////////////////////////////////

function InitializeSetup(): Boolean;
begin
  Result := True;
  // Create a mutex for the installer.
  // If it's already running display a message and stop installation
  if CheckForMutexes(installer_mutex_name) then begin
    if not WizardSilent() then
      MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK);
      Result := False;
  end else begin
    CreateMutex(installer_mutex_name);

    is_update := RegKeyExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{015C5B35-B678-451C-9AEE-821E8D69621C}_is1');

  end;
end;


function InitializeUninstall(): Boolean;
begin
  Result := True;
  // Create a mutex for the installer.
  // If it's already running display a message and stop installation
  if CheckForMutexes(installer_mutex_name) then begin
    if not WizardSilent() then
      MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK);
      Result := False;
  end else begin
    CreateMutex(installer_mutex_name);
  end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    if IsServiceRunning('pbfilter') then begin
      StopService('pbfilter');
    end;
    RemoveService('pbfilter');
  end;
  if CurStep = ssPostInstall then begin
    // Delete the old PeerBlock's startup registry value
    if OldStartupCheck then begin
      RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerGuardian');
    end;
    if IsTaskSelected('uninstall_pg') then begin
      KillAndUninstallPG;
    end;
    if IsTaskSelected('reset_settings') then begin
      RemoveUserFiles;
      RemoveMiscFiles;
    end;
  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then begin
    // When uninstalling, ask the user if they want to delete PeerBlock's logs and settings
    if fileExists(ExpandConstant('{app}\peerblock.conf')) then begin
      if MsgBox(ExpandConstant('{cm:msg_DeleteListsSettings}'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then begin
        RemoveUserFiles;
      end;
    end;
    StopService('pbfilter');
    RemoveService('pbfilter');
    // Always delete the rest of PeerBlock's files
    RemoveMiscFiles;
    RemoveDir(ExpandConstant('{app}'));
    RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerBlock');
  end;
end;

