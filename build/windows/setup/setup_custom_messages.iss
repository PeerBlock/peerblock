;  PeerBlock copyright (C) 2009-2011 PeerBlock, LLC
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


[CustomMessages]
; msg=Message, tsk=Task
; English
en.msg_DeleteLogsListsSettings =Do you also want to delete PeerBlock's lists, logs and settings?%n%nIf you plan on reinstalling PeerBlock you might not want to delete them.
en.msg_SetupIsRunningWarning   =PeerBlock Setup is already running!
en.msg_ServiceManager          =The service manager is not available
en.msg_ServiceManager2         =Only NT based systems support services
#if defined(sse_required) || defined(sse2_required)
en.msg_simd_sse                =This build of PeerBlock requires a CPU with SSE extension support.%n%nYour CPU does not have those capabilities.
en.msg_simd_sse2               =This build of PeerBlock requires a CPU with SSE2 extension support.%n%nYour CPU does not have those capabilities.
#endif
en.run_visit_website           =Visit PeerBlock's Website
en.tsk_other                   =Other tasks:
en.tsk_remove_startup          =Remove PeerBlock from Windows startup
en.tsk_reset                   =Delete PeerBlock's files:
en.tsk_delete_misc             =History and misc files
en.tsk_delete_lists            =Default lists
en.tsk_delete_custom_lists     =Custom lists
en.tsk_delete_logs             =Logs
en.tsk_delete_settings         =Settings
en.tsk_startup                 =Startup option:
en.tsk_startup_descr           =Start PeerBlock on system startup
en.tsk_uninstall_pg            =Uninstall PeerGuardian after PeerBlock's installation
en.tsk_use_pg_settings         =Use PeerGuardian's settings and custom lists
