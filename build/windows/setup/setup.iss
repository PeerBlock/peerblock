;  PeerBlock modifications copyright (C) 2009-2013 PeerBlock, LLC
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
;
; Requirements:
; *Inno Setup: http://www.jrsoftware.org/isdl.php


; Define "VS2010build" or "VS2012build" based on which build you compiled or use the appropriate batch file
;#define VS2010build
;#define VS2012build


#if VER < 0x05040200
  #error Update your Inno Setup version
#endif


#include "..\..\..\src\peerblock\version_parsed.h"

#define app_ver str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "." + str(PB_VER_BUGFIX) + "." + str(PB_VER_BUILDNUM)

; Uncomment one of the #define app_ver_short and comment all other
; E.g. 1.0+
#define app_ver_short str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "+"
; E.g. 1.0.1
;#define app_ver_short str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "." + str(PB_VER_BUGFIX)
; E.g. 1.0.1+
;#define app_ver_short str(PB_VER_MAJOR) + "." + str(PB_VER_MINOR) + "." + str(PB_VER_BUGFIX) + "+"

#define copyright            "Copyright © 2009-2013, PeerBlock, LLC"
#define installer_build_date GetDateTimeString('mmm, d yyyy', '', '')
#define quick_launch         "{userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock"


#if defined(VS2010build)
  #define bindir        = "..\bin10"
  #define sse_required
#elif defined(VS2012build)
  #define bindir        = "..\bin12"
  #define sse_required
#else
  #define bindir        = "..\bin"
#endif


[Setup]
AppID={{015C5B35-B678-451C-9AEE-821E8D69621C}
AppName=PeerBlock
AppVersion={#app_ver}
AppVerName=PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM})
AppPublisher=PeerBlock, LLC
AppCopyright={#copyright}
AppPublisherURL=http://www.peerblock.com/
AppSupportURL=http://www.peerblock.com/support
AppUpdatesURL=http://www.peerblock.com/releases
AppContact=http://www.peerblock.com/
VersionInfoCompany=PeerBlock, LLC
VersionInfoCopyright={#copyright}
VersionInfoDescription=PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM}) Setup
VersionInfoProductName=PeerBlock
VersionInfoProductVersion={#app_ver}
VersionInfoTextVersion={#app_ver}
VersionInfoVersion={#app_ver}
DefaultDirName={pf}\PeerBlock
DefaultGroupName=PeerBlock
LicenseFile=..\..\..\license.txt
InfoBeforeFile=readme_before.rtf
OutputDir=.
#if defined(VS2012build)
OutputBaseFilename=PeerBlock-Setup_v{#app_ver_short}_r{#PB_VER_BUILDNUM}_VS2012
#elif defined(VS2010build)
OutputBaseFilename=PeerBlock-Setup_v{#app_ver_short}_r{#PB_VER_BUILDNUM}_VS2010
#else
OutputBaseFilename=PeerBlock-Setup_v{#app_ver_short}_r{#PB_VER_BUILDNUM}
#endif
Compression=lzma2/max
SolidCompression=yes
#if defined(VS2012build) || defined(VS2010build)
MinVersion=0,5.1.2600sp3
#else
MinVersion=0,5.0
#endif
UninstallDisplayName=PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM})
UninstallDisplayIcon={app}\peerblock.exe
AppReadmeFile={app}\readme.rtf
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=WizardSmallImageFile.bmp
SetupIconFile=..\..\..\src\peerblock\res\pb.ico
AllowNoIcons=yes
ShowTasksTreeLines=yes
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

; Include installer's custom messages
#include "setup_custom_messages.iss"


[Messages]
#if defined(VS2012build)
BeveledLabel=PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM}) [MSVC2012] built on {#installer_build_date}
#elif defined(VS2010build)
BeveledLabel=PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM}) [MSVC2010] built on {#installer_build_date}
#else
BeveledLabel=PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM}) built on {#installer_build_date}
#endif
SetupAppTitle=Setup - PeerBlock
SetupWindowTitle=Setup - PeerBlock


[Tasks]
Name: desktopicon;               Description: {cm:CreateDesktopIcon};       GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon;           Description: {cm:CreateQuickLaunchIcon};   GroupDescription: {cm:AdditionalIcons};                        Flags: unchecked; OnlyBelowVersion: 0,6.01
Name: startup;                   Description: {cm:tsk_startup_descr};       GroupDescription: {cm:tsk_startup}; Check: not StartupCheck(); Flags: checkedonce unchecked
Name: remove_startup;            Description: {cm:tsk_remove_startup};      GroupDescription: {cm:tsk_startup}; Check: StartupCheck();     Flags: checkedonce unchecked
Name: uninstall_pg;              Description: {cm:tsk_uninstall_pg};        GroupDescription: {cm:tsk_other};   Check: IsPGInstalled();    Flags: checkedonce unchecked
Name: use_pg_settings;           Description: {cm:tsk_use_pg_settings};     GroupDescription: {cm:tsk_other};   Check: FileExists(ExpandConstant('{code:GetPGPath}\pg2.conf')) and not IsUpgrade()
Name: delete_lists;              Description: {cm:tsk_delete_lists};        GroupDescription: {cm:tsk_reset};   Check: ListsExist();       Flags: checkablealone checkedonce unchecked
Name: delete_lists\custom_lists; Description: {cm:tsk_delete_custom_lists}; GroupDescription: {cm:tsk_reset};   Check: CustomListsExist(); Flags: checkedonce unchecked dontinheritcheck
Name: delete_misc;               Description: {cm:tsk_delete_misc};         GroupDescription: {cm:tsk_reset};   Check: MiscFilesExist();   Flags: checkedonce unchecked
Name: delete_logs;               Description: {cm:tsk_delete_logs};         GroupDescription: {cm:tsk_reset};   Check: LogsExist();        Flags: checkedonce unchecked
Name: delete_settings;           Description: {cm:tsk_delete_settings};     GroupDescription: {cm:tsk_reset};   Check: SettingsExist();    Flags: checkedonce unchecked


[Files]
; 2K/XP 32bit files
Source: {#bindir}\Win32\Release\peerblock.exe;         DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode(); OnlyBelowVersion: 0,6.0
Source: {#bindir}\Win32\Release\pbfilter.sys;          DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode(); OnlyBelowVersion: 0,6.0

; XP 64bit files
Source: {#bindir}\x64\Release\peerblock.exe;           DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode();     OnlyBelowVersion: 0,6.0
Source: {#bindir}\x64\Release\pbfilter.sys;            DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode();     OnlyBelowVersion: 0,6.0

; Vista/7 32bit files
Source: {#bindir}\Win32\Release_(Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode(); MinVersion: 0,6.0
Source: {#bindir}\Win32\Release_(Vista)\pbfilter.sys;  DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode(); MinVersion: 0,6.0

; Vista/7 64bit files
Source: {#bindir}\x64\Release_(Vista)\peerblock.exe;   DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode();     MinVersion: 0,6.0
Source: {#bindir}\x64\Release_(Vista)\pbfilter.sys;    DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode();     MinVersion: 0,6.0

; Copy PG settings and custom lists only if PG is installed and the user has chosen to do so
Source: {code:GetPGPath}\pg2.conf;                     DestDir: {app};  DestName: peerblock.conf; Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPGPath}\*.p2p;                        DestDir: {app};                            Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPGPath}\lists\*.p2b;                  DestDir: {app}\lists;                      Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall
Source: {code:GetPGPath}\lists\*.p2p;                  DestDir: {app}\lists;                      Tasks: use_pg_settings; Flags: skipifsourcedoesntexist external uninsneveruninstall

Source: ..\..\..\license.txt;                          DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\..\doc\readme.rtf;                       DestDir: {app};                                                    Flags: ignoreversion


[Icons]
Name: {group}\PeerBlock;                    Filename: {app}\peerblock.exe; WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM})
Name: {group}\Uninstall PeerBlock;          Filename: {uninstallexe};      WorkingDir: {app}; IconFilename: {app}\peerblock.exe; IconIndex: 1; Comment: {cm:UninstallProgram,PeerBlock}
Name: {group}\Help and Support\Forums;      Filename: http://forums.peerblock.com/
Name: {group}\Help and Support\Homepage;    Filename: http://www.peerblock.com/
Name: {group}\Help and Support\ReadMe;      Filename: {app}\readme.rtf;    WorkingDir: {app}; Comment: PeerBlock's ReadMe
Name: {group}\Help and Support\User Manual; Filename: http://www.peerblock.com/userguide
Name: {userdesktop}\PeerBlock;              Filename: {app}\peerblock.exe; WorkingDir: {app}; Tasks: desktopicon;     IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM})
Name: {#quick_launch};                      Filename: {app}\peerblock.exe; WorkingDir: {app}; Tasks: quicklaunchicon; IconFilename: {app}\peerblock.exe; IconIndex: 0; Comment: PeerBlock {#app_ver_short} (r{#PB_VER_BUILDNUM})


[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: PeerBlock; ValueType: string; ValueData: {app}\peerblock.exe; Tasks: startup
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: PeerBlock;                                                    Tasks: delete_settings remove_startup;  Flags: deletevalue; Check: not IsTaskSelected('startup')


[Run]
Filename: {app}\peerblock.exe;       Description: {cm:LaunchProgram,PeerBlock}; WorkingDir: {app}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: http://www.peerblock.com/; Description: {cm:run_visit_website};                          Flags: nowait postinstall skipifsilent shellexec unchecked


[InstallDelete]
; During installation, delete old files in install folder
Name: {app}\license.txt;                                       Type: files
Name: {app}\peerblock.url;                                     Type: files

; ...also, delete Manual files
Name: {app}\manual\index.htm;                                  Type: files
Name: {app}\manual\FAQ\index.htm;                              Type: files
Name: {app}\manual\FAQ;                                        Type: dirifempty
Name: {app}\manual\how_to_use\installation.htm;                Type: files
Name: {app}\manual\how_to_use\quick_start_wizard.htm;          Type: files
Name: {app}\manual\how_to_use\selecting_appropriate_lists.htm; Type: files
Name: {app}\manual\how_to_use\using_lists.htm;                 Type: files
Name: {app}\manual\how_to_use;                                 Type: dirifempty
Name: {app}\manual\images\*.png;                               Type: files
Name: {app}\manual\images;                                     Type: dirifempty
Name: {app}\manual\introduction;                               Type: dirifempty
Name: {app}\manual\settings\index.htm;                         Type: files
Name: {app}\manual\settings;                                   Type: dirifempty
Name: {app}\manual\what_you_can_do;                            Type: dirifempty
Name: {app}\manual;                                            Type: dirifempty

; ...and finally, delete old start menu entries
Name: {group}\License.lnk;                                     Type: files
Name: {group}\PeerBlock on the Web.url;                        Type: files
Name: {group}\Help and Support\License.lnk;                    Type: files
Name: {group}\Help and Support\PeerBlock on the Web.url;       Type: files
Name: {group}\ReadMe.lnk;                                      Type: files
Name: {group}\Uninstall.lnk;                                   Type: files

; While we are at it, delete any shortcut which is not selected
Name: {userdesktop}\PeerBlock.lnk;                             Type: files; Check: not IsTaskSelected('desktopicon')     and IsUpgrade()
Name: {#quick_launch}.lnk;                                     Type: files; Check: not IsTaskSelected('quicklaunchicon') and IsUpgrade(); OnlyBelowVersion: 0,6.01


[Code]
// Include custom installer code
#include "setup_custom_code.iss"
#include "setup_services.iss"


///////////////////////////////////////////
//  Inno Setup functions and procedures  //
///////////////////////////////////////////
const
  installer_mutex = 'peerblock_setup_mutex';

function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer.
  // If it's already running display a message and stop the installation
  if CheckForMutexes(installer_mutex) and not WizardSilent() then begin
      Log('Custom Code: Installer is already running');
      SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
      Result := False;
  end
  else begin
    Result := True;
    Log('Custom Code: Creating installer`s mutex');
    CreateMutex(installer_mutex);

#if defined(sse2_required)
    if not IsSSE2Supported() then begin
      Log('Custom Code: Found a non SSE2 capable CPU');
      SuppressibleMsgBox(CustomMessage('msg_simd_sse2'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#elif defined(sse_required)
    if not IsSSESupported() then begin
      Log('Custom Code: Found a non SSE capable CPU');
      SuppressibleMsgBox(CustomMessage('msg_simd_sse'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#endif

  end;
end;


function InitializeUninstall(): Boolean;
begin
  if CheckForMutexes(installer_mutex) then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    CreateMutex(installer_mutex);
    Result := True;
  end;
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if IsUpgrade() then begin
    // Hide the license and the InfoBefore page
    if (PageID = wpLicense) or (PageID = wpInfoBefore) then
      Result := True;
  end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    if IsServiceRunning('pbfilter') then begin
      Log('Custom Code: pbfilter service is running, will attempt to stop it');
      StopService('pbfilter');
    end;
    Log('Custom Code: pbfilter service is not running, will attempt to remove it');
    RemoveService('pbfilter');
  end;

  if CurStep = ssPostInstall then begin
    // Delete the old PeerBlock's startup registry value
    if OldStartupCheck() then begin
      Log('Custom Code: Removing old startup entry');
      RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerGuardian');
    end;

    if IsTaskSelected('uninstall_pg') then begin
      Log('Custom Code: User selected the "uninstall_pg" task, calling KillAndUninstallPG()');
      KillAndUninstallPG;
    end;

    if IsTaskSelected('delete_misc') then begin
      Log('Custom Code: User selected the "delete_misc" task, calling RemoveMiscFiles()');
      RemoveMiscFiles;
    end;

    if IsTaskSelected('delete_lists') then begin
      Log('Custom Code: User selected the "delete_lists" task, calling RemoveLists()');
      RemoveLists;
    end;

    if IsTaskSelected('delete_lists\custom_lists') then begin
      Log('Custom Code: User selected the "delete_lists\custom_lists" task, calling RemoveCustomLists()');
      RemoveCustomLists;
    end;

    if IsTaskSelected('delete_logs') then begin
      Log('Custom Code: User selected the "delete_logs" task, calling RemoveLogs()');
      RemoveLogs;
    end;

    if IsTaskSelected('delete_settings') then begin
      Log('Custom Code: User selected the "delete_setting" task, calling RemoveSettings()');
      RemoveSettings;
    end;

  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then begin
    // When uninstalling, ask the user if they want to delete PeerBlock's logs and settings
    if FileExists(ExpandConstant('{app}\peerblock.conf')) then begin
      if SuppressibleMsgBox(CustomMessage('msg_DeleteLogsListsSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then begin
        RemoveCustomLists();
        RemoveLists();
        RemoveLogs();
        RemoveSettings();
      end;
    end;

    // Always stop and remove pbfilter service just in case
    StopService('pbfilter');
    RemoveService('pbfilter');

    // Always delete the rest of PeerBlock's files
    RemoveMiscFiles();
    RemoveDir(ExpandConstant('{app}'));

    if StartupCheck() then
      // Delete the installed PeerBlock's startup entry when uninstalling
      RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerBlock');

  end;
end;
