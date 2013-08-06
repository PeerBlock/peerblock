(*
;  Original source: http://goo.gl/PTi56
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
*)


[Code]
// Various service related functions

type
  SERVICE_STATUS = record
    dwServiceType             : cardinal;
    dwCurrentState            : cardinal;
    dwControlsAccepted        : cardinal;
    dwWin32ExitCode           : cardinal;
    dwServiceSpecificExitCode : cardinal;
    dwCheckPoint              : cardinal;
    dwWaitHint                : cardinal;
  end;
  HANDLE = cardinal;

const
  SERVICE_QUERY_CONFIG        = $1;
  SERVICE_CHANGE_CONFIG       = $2;
  SERVICE_QUERY_STATUS        = $4;
  SERVICE_START               = $10;
  SERVICE_STOP                = $20;
  SERVICE_ALL_ACCESS          = $f01ff;
  SC_MANAGER_ALL_ACCESS       = $f003f;
  SERVICE_KERNEL_DRIVER       = $1;
  SERVICE_WIN32_OWN_PROCESS   = $10;
  SERVICE_WIN32_SHARE_PROCESS = $20;
  SERVICE_WIN32               = $30;
  SERVICE_INTERACTIVE_PROCESS = $100;
  SERVICE_BOOT_START          = $0;
  SERVICE_SYSTEM_START        = $1;
  SERVICE_AUTO_START          = $2;
  SERVICE_DEMAND_START        = $3;
  SERVICE_DISABLED            = $4;
  SERVICE_DELETE              = $10000;
  SERVICE_CONTROL_STOP        = $1;
  SERVICE_CONTROL_PAUSE       = $2;
  SERVICE_CONTROL_CONTINUE    = $3;
  SERVICE_CONTROL_INTERROGATE = $4;
  SERVICE_STOPPED             = $1;
  SERVICE_START_PENDING       = $2;
  SERVICE_STOP_PENDING        = $3;
  SERVICE_RUNNING             = $4;
  SERVICE_CONTINUE_PENDING    = $5;
  SERVICE_PAUSE_PENDING       = $6;
  SERVICE_PAUSED              = $7;

// #######################################################################################
// nt based service utilities
// #######################################################################################
function OpenSCManager(lpMachineName, lpDatabaseName: AnsiString; dwDesiredAccess: cardinal): HANDLE;
external 'OpenSCManagerA@advapi32.dll stdcall';

function OpenService(hSCManager: HANDLE; lpServiceName: AnsiString; dwDesiredAccess: cardinal): HANDLE;
external 'OpenServiceA@advapi32.dll stdcall';

function CloseServiceHandle(hSCObject: HANDLE): Boolean;
external 'CloseServiceHandle@advapi32.dll stdcall';

function CreateService(hSCManager: HANDLE; lpServiceName, lpDisplayName: AnsiString; dwDesiredAccess,dwServiceType,dwStartType,dwErrorControl: cardinal; lpBinaryPathName,lpLoadOrderGroup: AnsiString; lpdwTagId: cardinal; lpDependencies,lpServiceStartName,lpPassword: AnsiString): cardinal;
external 'CreateServiceA@advapi32.dll stdcall';

function DeleteService(hService: HANDLE): Boolean;
external 'DeleteService@advapi32.dll stdcall';

function StartNTService(hService: HANDLE; dwNumServiceArgs: cardinal; lpServiceArgVectors: cardinal): Boolean;
external 'StartServiceA@advapi32.dll stdcall';

function ControlService(hService: HANDLE; dwControl: cardinal; var ServiceStatus: SERVICE_STATUS): Boolean;
external 'ControlService@advapi32.dll stdcall';

function QueryServiceStatus(hService: HANDLE; var ServiceStatus: SERVICE_STATUS): Boolean;
external 'QueryServiceStatus@advapi32.dll stdcall';

function QueryServiceStatusEx(hService: HANDLE; ServiceStatus: SERVICE_STATUS): Boolean;
external 'QueryServiceStatus@advapi32.dll stdcall';


function OpenServiceManager(): HANDLE;
begin
  Result := OpenSCManager('','ServicesActive',SC_MANAGER_ALL_ACCESS);
  if Result = 0 then
    SuppressibleMsgBox(CustomMessage('msg_ServiceManager'), mbError, MB_OK, MB_OK);
end;


function InstallService(FileName, ServiceName, DisplayName, Description: AnsiString; ServiceType,StartType: cardinal): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := CreateService(hSCM,ServiceName,DisplayName,SERVICE_ALL_ACCESS,ServiceType,StartType,0,FileName,'',0,'','','');
    if hService <> 0 then begin
      Result := True;
      // Win2K & WinXP supports aditional description text for services
      if Description <> '' then
        RegWriteStringValue(HKLM,'System\CurrentControlSet\Services\' + ServiceName,'Description',Description);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function RemoveService(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM,ServiceName,SERVICE_DELETE);
    if hService <> 0 then begin
      Result := DeleteService(hService);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function StartService(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM,ServiceName,SERVICE_START);
    if hService <> 0 then begin
      Result := StartNTService(hService,0,0);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function StopService(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
  Status   : SERVICE_STATUS;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM,ServiceName,SERVICE_STOP);
    if hService <> 0 then begin
      Result := ControlService(hService,SERVICE_CONTROL_STOP,Status);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function IsServiceRunning(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
  Status   : SERVICE_STATUS;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM,ServiceName,SERVICE_QUERY_STATUS);
    if hService <> 0 then begin
      if QueryServiceStatus(hService,Status) then begin
        Result := (Status.dwCurrentState = SERVICE_RUNNING);
      end;
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;
