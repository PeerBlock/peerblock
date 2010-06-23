/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC
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

#include "stdafx.h"
#include "../pbfilter/filter.h"
#include "wfp.hpp"

static const wchar_t* PBFILTER_NAME = L"pbfilter";
static const wchar_t* PBFILTER_PATH = L"pbfilter.sys";

pbfilter::pbfilter() {
	std::wstring p = L"\\??\\" + (path::base_dir() / PBFILTER_PATH).file_str();

	m_filter.removable = true; // so it can be removed if there's a path mismatch.
	m_filter.load(PBFILTER_NAME, p.c_str());
	if(m_filter.isrunning()) {
		m_filter.stop();
	}
	m_filter.start();
	m_filter.removable = false; // don't remove it afterward though.

	install_callout();
	start_thread();
}

pbfilter::~pbfilter() {
	stop_thread();
	remove_callout();
	m_filter.removable = true; // let's delete the driver-service wrapper
	m_filter.close();
}

void pbfilter::install_callout() {
	FWPM_SESSION0 ses = {0};
	ses.displayData.name = L"PeerBlock Dynamic Session";
	ses.displayData.description = L"PeerBlock firewall callout.";
	ses.flags = FWPM_SESSION_FLAG_DYNAMIC;

	FWPM_CALLOUT0 ocallout4 = {0};
	ocallout4.calloutKey = PBWFP_CONNECT_CALLOUT_V4;
	ocallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
	ocallout4.displayData.name = L"PeerBlock IPv4 Flow Outbound Callout";
	ocallout4.displayData.description = L"PeerBlock IPv4 Flow Outbound Callout";

	FWPM_CALLOUT0 icallout4 = {0};
	icallout4.calloutKey = PBWFP_ACCEPT_CALLOUT_V4;
	icallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
	icallout4.displayData.name = L"PeerBlock IPv4 Flow Inbound Callout";
	icallout4.displayData.description = L"PeerBlock IPv4 Flow Inbound Callout";

	FWPM_CALLOUT0 ocallout6 = {0};
	ocallout6.calloutKey = PBWFP_CONNECT_CALLOUT_V6;
	ocallout6.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
	ocallout6.displayData.name = L"PeerBlock IPv6 Flow Outbound Callout";
	ocallout6.displayData.description = L"PeerBlock IPv6 Flow Outbound Callout";

	FWPM_CALLOUT0 icallout6 = {0};
	icallout6.calloutKey = PBWFP_ACCEPT_CALLOUT_V6;
	icallout6.applicableLayer = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
	icallout6.displayData.name = L"PeerBlock IPv6 Flow Inbound Callout";
	icallout6.displayData.description = L"PeerBlock IPv6 Flow Inbound Callout";

	FWPM_SUBLAYER0 monitor = {0};
	monitor.subLayerKey = PB_MONITOR_SUBLAYER;
	monitor.displayData.name = L"PeerBlock Sub layer";
	monitor.displayData.description = L"PeerBlock Sub layer";

	FWPM_FILTER0 ofilter4 = {0};
	ofilter4.filterKey = PBWFP_CONNECT_FILTER_V4;
	ofilter4.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
	ofilter4.displayData.name = L"PeerBlock Flow Outbound Filter for IPv4.";
	ofilter4.displayData.description = L"Sets up flow outbound filter for all IPv4 traffic, so we can filter the blocked ip-addresses out.";
	ofilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN; // The callout may return block or permit.
	ofilter4.action.calloutKey = PBWFP_CONNECT_CALLOUT_V4;
	ofilter4.subLayerKey = PB_MONITOR_SUBLAYER;
	ofilter4.weight.type = FWP_EMPTY; // auto-weight.

	FWPM_FILTER0 ifilter4 = {0};
	ifilter4.filterKey = PBWFP_ACCEPT_FILTER_V4;
	ifilter4.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
	ifilter4.displayData.name = L"PeerBlock Flow Inbound Filter for IPv4.";
	ifilter4.displayData.description = L"Sets up flow inbound filter for all IPv4 traffic, so we can filter the blocked ip-addresses out.";
	ifilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN; // The callout may return block or permit.
	ifilter4.action.calloutKey = PBWFP_ACCEPT_CALLOUT_V4;
	ifilter4.subLayerKey = PB_MONITOR_SUBLAYER;
	ifilter4.weight.type = FWP_EMPTY; // auto-weight.

	FWPM_FILTER0 ofilter6 = {0};
	ofilter6.filterKey = PBWFP_CONNECT_FILTER_V6;
	ofilter6.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
	ofilter6.displayData.name = L"PeerBlock Flow Outbound Filter for IPv6.";
	ofilter6.displayData.description = L"Sets up flow outbound filter for all IPv6 traffic, so we can filter the blocked ip-addresses out.";
	ofilter6.action.type = FWP_ACTION_CALLOUT_UNKNOWN; // The callout may return block or permit.
	ofilter6.action.calloutKey = PBWFP_CONNECT_CALLOUT_V6;
	ofilter6.subLayerKey = PB_MONITOR_SUBLAYER;
	ofilter6.weight.type = FWP_EMPTY; // auto-weight.

	FWPM_FILTER0 ifilter6 = {0};
	ifilter6.filterKey = PBWFP_ACCEPT_FILTER_V6;
	ifilter6.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
	ifilter6.displayData.name = L"PeerBlock Flow Inbound Filter for IPv6.";
	ifilter6.displayData.description = L"Sets up flow inbound filter for all IPv6 traffic, so we can filter the blocked ip-addresses out.";
	ifilter6.action.type = FWP_ACTION_CALLOUT_UNKNOWN; // The callout may return block or permit.
	ifilter6.action.calloutKey = PBWFP_ACCEPT_CALLOUT_V6;
	ifilter6.subLayerKey = PB_MONITOR_SUBLAYER;
	ifilter6.weight.type = FWP_EMPTY; // auto-weight.

	//wfp_session session(ses);

	m_session.open(ses);
	m_session.begin();

	m_session.add(ocallout4);
	m_session.add(icallout4);
	m_session.add(ocallout6);
	m_session.add(icallout6);
	m_session.add(monitor);
	m_session.add(ofilter4);
	m_session.add(ifilter4);
	m_session.add(ofilter6);
	m_session.add(ifilter6);

	m_session.commit();
}

void pbfilter::remove_callout() {
	/*FWPM_SESSION0 ses = {0};
	ses.displayData.name = L"PeerGuardian 2 Non-Dynamic Session";
	ses.displayData.description = L"PeerGuardian 2 is removing the firewall callout.";

	wfp_session session(ses);

	m_session.begin();

	m_session.remove_filter(PBWFP_CONNECT_FILTER_V4);
	m_session.remove_filter(PBWFP_ACCEPT_FILTER_V4);
	m_session.remove_filter(PBWFP_CONNECT_FILTER_V6);
	m_session.remove_filter(PBWFP_ACCEPT_FILTER_V6);
	m_session.remove_sublayer(PB_MONITOR_SUBLAYER);
	m_session.remove_callout(PBWFP_CONNECT_CALLOUT_V4);
	m_session.remove_callout(PBWFP_ACCEPT_CALLOUT_V4);
	m_session.remove_callout(PBWFP_CONNECT_CALLOUT_V6);
	m_session.remove_callout(PBWFP_ACCEPT_CALLOUT_V6);

	m_session.commit();*/
	m_session.close();
}
