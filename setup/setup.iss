[Setup]
AppName=PeerBlock
AppVerName=PeerBlock 0.9.1 (r61)
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
OutputBaseFilename=PeerBock-Setup_v0.9.1.r61
Compression=lzma/ultra64
SolidCompression=true
;New set minimum to Win2k/NT5.0
MinVersion=5.0.2195,5.0.2195
AppVersion=0.9.1.61
VersionInfoVersion=0.9.0.61
AppMutex=Global\PeerBlock
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
InternalCompressLevel=ultra64
Encryption=false
SetupLogging=true
;Feel Free to edit this \/ PS I prefur to keep my real identity out of the spotlight ;-)
AppCopyright=DisCoStu / MarKSidE
;Set privileges
PrivilegesRequired=none
;Well Da!
AppID={{015C5B35-B678-451C-9AEE-821E8D69621C}
UninstallDisplayName=PeerBlock
VersionInfoCompany=PeerBlock
VersionInfoCopyright=PeerBlock
VersionInfoProductName=PeerBlock
VersionInfoProductVersion=0.9.1.61
;Prevents binary mixup "Need to enable md5 check before this should be enabled"
MergeDuplicateFiles=false
;New setup image BMP's
WizardImageFile=setup\WizModernImage.bmp
WizardSmallImageFile=setup\WizModernSmallImage.bmp
WizardImageBackColor=clBlack

[Tasks]
Name: startupicon; Description: &Start with Windows; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: quickicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked


;ToDo
;Need to implement uninstall checkbox for peerguardian2 as a choice. err Just run Exc Uninstall anyway.  - Done
;Implement an if argument if pg2.conf doesnt exist. - Null Revised and - Done...
;Add option for copy of pg2.conf ... This would be easier... - Copied and renamed success - Done...
;Need to add start in paths - Added start in paths for the shotcuts to minimise external executible operation such as placement of the log and dmp files. - Done
;I do not think this^^ would effect the apps funtion to not have them linked to the path but for suspicions sake meh.
;Make run in none admin mode - Changes made to the installers stuctures should now run in non admin mode under Win2000 - Vista x64 - Done
;Installer Needs app ID - Added App ID. - Done
;Need restiction for lower versions of Windows set @ NT4.0 the app doesn't work on 4.0 - Added - Done
;No more errors as far as I can see.
[Files]
; Win2k files
Source: win32\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: win32\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external; Check: Is2k
;Source: win32\release\; DestDir: {app}; Flags: ignoreversion; Check: Is2k
; WinXP x64 files
Source: x64\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: x64\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external; Check: IsXP64
;Source: x64\release\; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
; Vista files
Source: win32\release (vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: win32\release (vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external; Check: IsVista
;Source: win32\release (vista)\; DestDir: {app}; Flags: ignoreversion; Check: IsVista
; Vista x64 files
Source: x64\release (vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: x64\release (vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external; Check: IsVista64
;Source: x64\release (vista)\; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: license.txt; DestDir: {app}; Flags: ignoreversion
Source: readme.txt; DestDir: {app}; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

;[Components]
[INI]
Filename: {app}\peerblock.url; Section: InternetShortcut; Key: URL; String: http://www.peerblock.com/

[Icons]
Name: {group}\PeerBlock; Filename: {app}\peerblock.exe; WorkingDir: {app}; IconIndex: 0
Name: {group}\Uninstall; Filename: {app}\unins000.exe; WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,PeerBlock}; Filename: {app}\peerblock.url; WorkingDir: {app}
Name: {userdesktop}\PeerBlock; Filename: {app}\peerblock.exe; Tasks: desktopicon; WorkingDir: {app}; IconIndex: 0
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock; Filename: {app}\peerblock.exe; Tasks: quickicon; WorkingDir: {app}; IconIndex: 0

[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: PeerBlock; ValueData: {app}\peerblock.exe; Flags: uninsdeletevalue; Tasks: startupicon

[Run]
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; Flags: nowait postinstall skipifsilent
Filename: {pf}\peerguardian2\unins000.exe; Description: {cm:LaunchProgram,PeerGuardian2 Uninstall}; Flags: nowait postinstall skipifsilent skipifdoesntexist unchecked


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
