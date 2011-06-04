/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2011 PeerBlock, LLC
	Based on the original work by Tim Leonard

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

*/

#pragma once

#include <stdexcept>
#include <windows.h>
#include "resource.h"

#ifndef INCLUDED_FWPMU
#define INCLUDED_FWPMU
#include <fwpmu.h>
#endif

#include "win32_error.h"

class wfp_session {
public:
	wfp_session() : m_session(NULL) {}
	wfp_session(const FWPM_SESSION0 &session) : m_session(NULL) { open(session); }

	~wfp_session() {
		close();
	}

	void open(const FWPM_SESSION0 &session) {
		if(m_session) throw std::exception("session already open", 0);

		DWORD err = FwpmEngineOpen0(0, RPC_C_AUTHN_WINNT, 0, &session, &m_session);
		if (err == 1753)
			throw peerblock_error(IDS_NEEDSVCSERR, IDS_NEEDSVCSERRTEXT);
		if (err != ERROR_SUCCESS)
			throw win32_error("FwpmEngineOpen0", err);
	}

	void close() {
		if(m_session != NULL) {
			FwpmEngineClose0(m_session);
			m_session = NULL;
		}
	}

	void add(const FWPM_CALLOUT0 &callout) {
		DWORD err = FwpmCalloutAdd0(m_session, &callout, 0, 0);
		if (err != ERROR_SUCCESS && err != FWP_E_ALREADY_EXISTS) throw win32_error("FwpmCalloutAdd0", err);
	}

	void add(const FWPM_SUBLAYER0 &sublayer) {
		DWORD err = FwpmSubLayerAdd0(m_session, &sublayer, 0);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmSubLayerAdd0", err);
	}

	void add(const FWPM_FILTER0 &filter) {
		DWORD err = FwpmFilterAdd0(m_session, &filter, 0, 0);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmFilterAdd0", err);
	}

	void remove_callout(const GUID &guid) {
		DWORD err = FwpmCalloutDeleteByKey0(m_session, &guid);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmCalloutDeleteByKey0", err);
	}

	void remove_sublayer(const GUID &guid) {
		DWORD err = FwpmSubLayerDeleteByKey0(m_session, &guid);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmSubLayerDeleteByKey0", err);
	}

	void remove_filter(const GUID &guid) {
		DWORD err = FwpmFilterDeleteByKey0(m_session, &guid);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmFilterDeleteByKey0", err);
	}

	void begin() {
		DWORD err = FwpmTransactionBegin0(m_session, 0);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmTransactionBegin0", err);
	}

	void rollback() {
		DWORD err = FwpmTransactionAbort0(m_session);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmTransactionAbort0", err);
	}

	void commit() {
		DWORD err = FwpmTransactionCommit0(m_session);
		if(err != ERROR_SUCCESS) throw win32_error("FwpmTransactionCommit0", err);
	}

private:
	HANDLE m_session;
};
