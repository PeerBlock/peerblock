[Setup]
AppName=PeerBlock
AppVerName=PeerBlock 0.9.1 (r71)
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
;Nice I think I can spell\/
OutputBaseFilename=PeerBlock-Setup_v0.9.1.r71
Compression=lzma/ultra64
SolidCompression=true
;New set minimum to Win2k/NT5.0
MinVersion=5.0.2195,5.0.2195
AppVersion=0.9.1.71
VersionInfoVersion=0.9.0.71
;The Mutex signing in the executible itself is not set to peerblock.
;So the program is still identifying its self as peerguardian2
;It seems the naming of the Mutex durring install is taken from the AppName (See First Line)
;But more needs to be done on the Apps development cycle to rid it
;of all peerguardian2 branding... I will submit a changed source Code
;of revision 61 that will totally rid it of these problems.
AppMutex=Global\PeerGuardian2
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
InternalCompressLevel=ultra64
Encryption=false
SetupLogging=true
;Feel Free to edit this \/ PS I prefur to keep my real identity out of the spotlight ;-)
AppCopyright=DisCoStu / MarKSidE
;Set privileges
PrivilegesRequired=admin
;Well Da!
AppID={{015C5B35-B678-451C-9AEE-821E8D69621C}
UninstallDisplayName=PeerBlock
VersionInfoCompany=PeerBlock
VersionInfoCopyright=PeerBlock
VersionInfoProductName=PeerBlock
VersionInfoProductVersion=0.9.1.71
;File checks are manditory by compiler..
MergeDuplicateFiles=true
;New setup image BMP's
WizardImageFile=setup\WizModernImage.bmp
WizardSmallImageFile=setup\WizModernSmallImage.bmp
WizardImageBackColor=clBlack
UserInfoPage=false
AllowRootDirectory=false
;AlwaysUsePersonalGroup=false
BackColor=clGray
UninstallLogMode=overwrite
TimeStampsInUTC=true

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
;Make run in none admin mode - Changes made to the installers stuctures should now run in non admin mode under Win2000 - Vista x64 - Done - You can't run the program after install if your not running as an admin.
;Installer Needs app ID - Added App ID. - Done
;Need restiction for lower versions of Windows set @ NT4.0 the app doesn't work on 4.0 - Added - Done
;Needs fix for if the program is running durring an upgrade install - Mutex Needs to be labled Global peerguardian2 - Done
;Needs to uninstall Everything icl the directory - Done - Fixed all problems that could occure I hope...
;Fix for under privelaged users running Program being researched.  *** No Fix Found ***
;No more errors as far as I can see.
[Files]

; Win2k files
Source: win32\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: win32\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: Is2k

; WinXP x64 files
Source: x64\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: x64\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: IsXP64

; Vista files
Source: win32\release (vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: win32\release (vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: IsVista

; Vista x64 files
Source: x64\release (vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: x64\release (vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: IsVista64
;Licence & Readme
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
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; Flags: nowait postinstall skipifsilent runascurrentuser
;Filename: {pf}\peerguardian2\unins000.exe; Description: {cm:LaunchProgram,PeerGuardian2 Uninstall}; Flags: nowait postinstall skipifsilent skipifdoesntexist unchecked


[UninstallDelete]
;Solved the problem with the Directory Tree staying inplace with random files
Type: filesandordirs; Name: {app}\lists
Name: {app}\peerblock.*; Type: files
Name: {app}\pbfilter.sys; Type: files
Name: {app}\history.db; Type: files
Name: {app}\cache.p2b; Type: files
Type: dirifempty; Name: {app}
Name: {group}\*.*; Type: files
Type: dirifempty; Name: {group}
Name: {userdesktop}\PeerBlock; Type: files
Name: {userappdata}Microsoft\Internet Explorer\Quick Launch\PeerBlock; Type: files

[Code]
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
