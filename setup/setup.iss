; ISTool v5.3.0/Inno Setup v5.3.4, script by XhmikosR
;
; Requirements:
; *Inno Setup QuickStart Pack:
;   http://www.jrsoftware.org/isdl.php#qsp


#define getversionpath "..\Win32\Release (Vista)"
#define input_path ".."
#define app_version	GetFileVersion(AddBackslash(getversionpath) + "\peerblock.exe")

#define VerMajor
#define VerMinor
#define VerRevision
#define VerBuild

#expr ParseVersion(AddBackslash(getversionpath) + "\peerblock.exe", VerMajor, VerMinor, VerRevision, VerBuild)
#define app_version str(VerMajor) + "." + str(VerMinor) + "." + str(VerRevision) + "." + str(VerBuild)
#define simple_app_version str(VerMajor) + "." + str(VerMinor) + "." + str(VerRevision)
#define installer_build_date GetDateTimeString('dd/mm/yyyy', '.', '')


[Setup]
AppID={{015C5B35-B678-451C-9AEE-821E8D69621C}
AppName=PeerBlock
AppVersion={#= app_version}
AppVerName=PeerBlock {#= simple_app_version} (r{#= VerBuild})
AppPublisher=PeerBlock Project
AppCopyright=Mark Bulas
AppPublisherURL=http://www.peerblock.com/
AppSupportURL=http://www.peerblock.com/
AppUpdatesURL=http://www.peerblock.com/
VersionInfoCompany=PeerBlock
VersionInfoCopyright=PeerBlock Project
VersionInfoProductName=PeerBlock
VersionInfoProductVersion={#= app_version}
VersionInfoVersion={#= app_version}
VersionInfoDescription=PeerBlock {#= simple_app_version} (r{#= VerBuild}) Setup
VersionInfoTextVersion={#= app_version}
DefaultDirName={pf}\PeerBlock
DefaultGroupName=PeerBlock
LicenseFile=license.txt
InfoBeforeFile=readme.rtf
OutputDir=.
OutputBaseFilename=PeerBlock-Setup_v{#= simple_app_version}.r{#= VerBuild}
Compression=lzma/ultra64
InternalCompressLevel=ultra64
SolidCompression=True
MinVersion=5.0.2195,5.0.2195
UninstallDisplayName=PeerBlock {#= simple_app_version} (r{#= VerBuild})
UninstallDisplayIcon={app}\peerblock.exe
AppReadmeFile={app}\readme.rtf
WizardImageFile=WizModernImage.bmp
WizardSmallImageFile=WizModernSmallImage.bmp
WizardImageBackColor=clBlack
BackColor=clGray
DirExistsWarning=No
EnableDirDoesntExistWarning=False
AllowNoIcons=True
ShowTasksTreeLines=True
AlwaysShowDirOnReadyPage=True
AlwaysShowGroupOnReadyPage=True
WizardImageStretch=False
PrivilegesRequired=Admin
DisableDirPage=Yes
DisableProgramGroupPage=Yes
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
AppMutex=Global\PeerGuardian2


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
BeveledLabel=PeerBlock {#= simple_app_version} (r{#= VerBuild}) built on {#= installer_build_date}


[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: startup_task; Description: Start PeerBlock on system startup; GroupDescription: Startup options:; Check: StartupCheck(); Flags: unchecked
Name: remove_startup_task; Description: Remove PeerBlock from Windows startup; GroupDescription: Other tasks:; Check: NOT StartupCheck(); Flags: unchecked
Name: reset_settings; Description: Reset PeerBlock's settings; GroupDescription: Other tasks:; Check: SettingsExistCheck(); Flags: unchecked


[Files]
; Win2k files
Source: {#= input_path}\win32\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: {#= input_path}\win32\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: Is2k
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: Is2k

; WinXP x64 files
Source: {#= input_path}\x64\release\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: {#= input_path}\x64\release\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsXP64
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: IsXP64

; Vista files
Source: {#= input_path}\win32\release (Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: {#= input_path}\win32\release (Vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: IsVista

; Vista x64 files
Source: {#= input_path}\x64\release (Vista)\peerblock.exe; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: {#= input_path}\x64\release (Vista)\pbfilter.sys; DestDir: {app}; Flags: ignoreversion; Check: IsVista64
Source: {pf}\PeerGuardian2\pg2.conf; DestDir: {app}; DestName: peerblock.conf; Flags: skipifsourcedoesntexist external onlyifdoesntexist; Check: IsVista64

Source: {#= input_path}\setup\license.txt; DestDir: {app}; Flags: ignoreversion
Source: {#= input_path}\setup\readme.rtf; DestDir: {app}; Flags: ignoreversion


[Icons]
Name: {group}\PeerBlock; Filename: {app}\peerblock.exe; WorkingDir: {app}; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= VerBuild})
Name: {group}\Uninstall; Filename: {app}\unins000.exe; WorkingDir: {app}; Comment: {cm:UninstallProgram,PeerBlock}
Name: {group}\{cm:ProgramOnTheWeb,PeerBlock}; Filename: http://www.peerblock.com/; WorkingDir: {app}
Name: {group}\License; Filename: {app}\license.txt; WorkingDir: {app}
Name: {group}\ReadMe; Filename: {app}\readme.rtf; WorkingDir: {app}
Name: {userdesktop}\PeerBlock; Filename: {app}\peerblock.exe; Tasks: desktopicon; WorkingDir: {app}; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= VerBuild})
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\PeerBlock; Filename: {app}\peerblock.exe; Tasks: quicklaunchicon; WorkingDir: {app}; IconIndex: 0; Comment: PeerBlock {#= simple_app_version} (r{#= VerBuild})


[Registry]
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: PeerBlock; ValueData: """{app}\peerblock.exe"""; Tasks: startup_task; Flags: uninsdeletevalue
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: PeerBlock; Tasks: reset_settings remove_startup_task; Flags: deletevalue uninsdeletevalue


[Run]
Filename: {app}\peerblock.exe; Description: {cm:LaunchProgram,PeerBlock}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: http://www.peerblock.com/; Description: Visit PeerBlock's Website; Flags: nowait postinstall skipifsilent shellexec runascurrentuser unchecked


[InstallDelete]
Name: {app}\peerblock.url; Type: files
Name: {app}\readme.rtf; Type: files
Name: {app}\license.txt; Type: files
Name: {app}\peerblock.conf; Type: files; Tasks: reset_settings
Name: {app}\cache.p2b; Type: files; Tasks: reset_settings
Name: {app}\history.db; Type: files; Tasks: reset_settings
Name: {app}\peerblock.log; Type: files; Tasks: reset_settings



[Code]
// Create a mutex for the installer
const installer_mutex_name = 'peerblock_setup_mutex';


// Check if PeerBlock is configured to run on startup in order to control
// startup choice within the installer
function StartupCheck(): Boolean;
begin
  Result := True;
  if RegValueExists(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'PeerBlock') then
  Result := False;
end;


// Check if PeerBlock's settings exist
function SettingsExistCheck(): Boolean;
begin
  Result := False;
  if FileExists(ExpandConstant('{app}\peerblock.conf')) then
  Result := True;
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


Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling ask user to delete PeerBlock's logs and settings
   if CurUninstallStep = usUninstall then begin
   if fileExists(ExpandConstant('{app}\peerblock.conf')) then begin
    if MsgBox(ExpandConstant('Do you also want to delete PeerBlock logs and settings? If you plan on reinstalling PeerBlock you do not have to delete them.'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then begin
      DelTree(ExpandConstant('{app}\lists\'), True, True, True);
      DeleteFile(ExpandConstant('{app}\peerblock.conf'));
    end;
   end;
   DeleteFile(ExpandConstant('{app}\peerblock.log'));
   DeleteFile(ExpandConstant('{app}\cache.p2b'));
   DeleteFile(ExpandConstant('{app}\history.db'));
  end;
end;


function InitializeSetup(): Boolean;
begin
	// Create a mutex for the installer and if it's already running then expose a message and stop installation
	Result := True;
	if CheckForMutexes(installer_mutex_name) then begin
		if not WizardSilent() then
			MsgBox(ExpandConstant('PeerBlock Setup is already running!'), mbError, MB_OK);
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
			MsgBox(ExpandConstant('PeerBlock Setup is already running!'), mbError, MB_OK);
		Result := False;
		end
		else begin
		CreateMutex(installer_mutex_name);
	end;
end;
