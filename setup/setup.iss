[Setup]
AppName=PeerBlock
AppVerName=PeerBlock 0.9.0 (r52)
AppPublisher=PeerBlock Project
AppPublisherURL=http://www.peerblock.com/
AppSupportURL=http://www.peerblock.com/
AppUpdatesURL=http://www.peerblock.com/
DefaultDirName={pf}\PeerBlock
DefaultGroupName=PeerBlock
SourceDir=../
LicenseFile=setup/license.txt
InfoBeforeFile=setup/readme.rtf
OutputDir=setup
OutputBaseFilename=PeerBock-Setup_v0.9.0.r52
Compression=lzma/ultra
SolidCompression=yes
MinVersion=4.0,4.0
AppVersion=0.9.0.52
VersionInfoVersion=0.9.0.52
AppMutex=Global\PeerBlock
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64

[Tasks]
Name: startupicon; Description: &Start with Windows; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: quickicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked


;ToDo
;Need to implement uninstall checkbox for peerguardian2 as a choice. err Just run Exc Uninstall anyway.  - Done
;Implement an if argument if pg2.conf doesnt exist. - Null Revised and - Done...        \
;Add option for copy of pg2.conf ... This would be easier... - Copied and renamed success - Done...
;No more errors as far as I can see.
[Files]
; Win2k files
Source: win32\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: win32\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: "{pf}\PeerGuardian2\pg2.conf"; DestDir: "{app}"; DestName: "peerblock.conf"; Flags: skipifsourcedoesntexist external; Check: Is2k
;Source: win32\release\; DestDir: {app}; Flags: ignoreversion; Check: Is2k
; WinXP x64 files
Source: x64\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: x64\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: "{pf}\PeerGuardian2\pg2.conf"; DestDir: "{app}"; DestName: "peerblock.conf"; Flags: skipifsourcedoesntexist external; Check: IsXP64
;Source: x64\release\; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
; Vista files
Source: win32\release (vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: win32\release (vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: "{pf}\PeerGuardian2\pg2.conf"; DestDir: "{app}"; DestName: "peerblock.conf"; Flags: skipifsourcedoesntexist external; Check: IsVista
;Source: win32\release (vista)\; DestDir: {app}; Flags: ignoreversion; Check: IsVista
; Vista x64 files
Source: x64\release (vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: x64\release (vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: "{pf}\PeerGuardian2\pg2.conf"; DestDir: "{app}"; DestName: "peerblock.conf"; Flags: skipifsourcedoesntexist external; Check: IsVista64
;Source: x64\release (vista)\; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: license.txt; DestDir: {app}; Flags: ignoreversion
Source: readme.txt; DestDir: {app}; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

;[Components]

[INI]
Filename: {app}\peerblock.url; Section: InternetShortcut; Key: URL; String: http://www.peerblock.com/

[Icons]
Name: {group}\PeerBlock; Filename: {app}\peerblock.exe
Name: {group}\Uninstall; Filename: {app}\unins000.exe
Name: {group}\{cm:ProgramOnTheWeb,PeerBlock}; Filename: {app}\peerblock.url
Name: {userdesktop}\PeerBlock; Filename: {app}\peerblock.exe; Tasks: desktopicon
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock; Filename: {app}\peerblock.exe; Tasks: quickicon

[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: PeerBlock; ValueData: {app}\peerblock.exe; Flags: uninsdeletevalue; Tasks: startupicon

[Run]
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; Flags: nowait postinstall skipifsilent
Filename: {pf}\peerguardian2\unins000.exe; Description: {cm:LaunchProgram,PeerGuardian2 Uninstall}; Flags: nowait postinstall skipifsilent


[UninstallDelete]
Type: files; Name: {app}\peerblock.url

[Code]
function Is9x: Boolean;
begin
  Result := not UsingWinNT;
end;

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
