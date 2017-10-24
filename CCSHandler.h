/*
 *   Copyright (C) 2013 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <string>
#include <cstdio>
#include <mutex>
#include <map>
#include <netinet/in.h>

#include "CCSProtocolHandler.h"
#include "DStarDefines.h"
#include "HeaderLogger.h"
#include "ConnectData.h"
#include "CCSCallback.h"
#include "AMBEData.h"
#include "PollData.h"
#include "Timer.h"
#include "Defs.h"

enum CCS_STATUS {
	CS_DISABLED,
	CS_CONNECTING,
	CS_CONNECTED,
	CS_ACTIVE
};

class CCCSHandler {
public:
	CCCSHandler(ICCSCallback* handler, const std::string& callsign, unsigned int delay, double latitude, double longitude, double frequency, double offset, const std::string& description1, const std::string& description2, const std::string& url, unsigned int localPort);
	~CCCSHandler();

	bool connect();

	void writeHeard(CHeaderData& header);
	void writeHeader(CHeaderData& header);
	void writeAMBE(CAMBEData& data);

	void startLink(const std::string& dtmf, const std::string& user, const std::string& type);
	void stopLink(const std::string& user = std::string(""), const std::string& type = std::string(""));

	void unlink(const std::string& callsign);

	void setReflector(const std::string& callsign = std::string(""));

	CCS_STATUS getStatus() const;

	static void disconnect();

	static void initialise(unsigned int count);

	static void process();

	static void clock(unsigned int ms);

	static void setHeaderLogger(CHeaderLogger* logger);

	static void setLocalAddress(const std::string& address);

	static void setHost(const std::string& host);

	static bool stateChange();
	static void writeStatus(FILE *file);

	static void getInfo(ICCSCallback* handler, CRemoteRepeaterData& data);

	static std::string getIncoming(const std::string& callsign);

	static void finalise();

protected:
	void clockInt(unsigned int ms);

	void processInt();

	void disconnectInt();

private:
	static CCCSHandler**  m_handlers;
	static unsigned int   m_count;

	static std::string    m_localAddress;
	static CHeaderLogger* m_headerLogger;

	static std::string    m_ccsHost;

	static std::map <std::string, std::string> m_cache;
	static std::mutex     m_mutex;

	static bool           m_stateChange;

	ICCSCallback*       m_handler;
	std::string         m_callsign;
	std::string         m_reflector;
	double              m_latitude;
	double              m_longitude;
	double              m_frequency;
	double              m_offset;
	std::string         m_description1;
	std::string         m_description2;
	std::string         m_url;
	in_addr             m_ccsAddress;
	CCCSProtocolHandler m_protocol;
	CCS_STATUS          m_state;
	std::string         m_local;
	CTimer              m_announceTimer;
	CTimer              m_inactivityTimer;
	CTimer              m_pollInactivityTimer;
	CTimer              m_pollTimer;
	CTimer              m_waitTimer;
	CTimer              m_tryTimer;
	unsigned int        m_tryCount;
	unsigned int        m_id;
	unsigned int        m_seqNo;
	time_t              m_time;
	DIRECTION           m_direction;
	std::string         m_yourCall;
	std::string         m_myCall1;
	std::string         m_myCall2;
	std::string         m_rptCall1;

	void process(CAMBEData& header);
	void process(CPollData& data);
	void process(CConnectData& connect);
	void process(CCCSData& data);

	unsigned int calcBackoff();

	static void        addToCache(const std::string& dtmf, const std::string& callsign);
	static std::string findInCache(const std::string& dtmf);
};

