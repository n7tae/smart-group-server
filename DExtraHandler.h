/*
 *   Copyright (C) 2010-2013,2015 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2018 by Thomas A. Early
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

#include <netinet/in.h>
#include <string>
#include <list>

#include "DExtraProtocolHandlerPool.h"
#include "GroupHandler.h"
#include "DStarDefines.h"
#include "CallsignList.h"
#include "ConnectData.h"
#include "HeaderData.h"
#include "AMBEData.h"
#include "PollData.h"
#include "Timer.h"
#include "Defs.h"

enum DEXTRA_STATE {
	DEXTRA_LINKING,
	DEXTRA_LINKED,
	DEXTRA_UNLINKING
};

class CDExtraHandler {
public:
	static void setCallsign(const std::string &callsign);
	static void setDExtraProtocolHandlerPool(CDExtraProtocolHandlerPool *pool);

	static void link(CGroupHandler *handler, const std::string &repeater, const std::string &reflector, const std::string &address);
	static void unlink(CGroupHandler *handler, const std::string &reflector = std::string(""), bool exclude = true);
	static void unlink(CDExtraHandler *reflector);
	static void unlink();

	static void writeHeader(CGroupHandler *handler, CHeaderData &header, DIRECTION direction);
	static void writeAMBE(CGroupHandler *handler, CAMBEData &data, DIRECTION direction);

	static void process(CHeaderData &header);
	static void process(CAMBEData &data);
	static void process(const CPollData &poll);
	static void process(CConnectData &connect);

	static void gatewayUpdate(const std::string &reflector, const std::string &address);
	static void clock(unsigned int ms);

	static void setWhiteList(CCallsignList *list);
	static void setBlackList(CCallsignList *list);

	static void finalise();

	static std::string getIncoming(const std::string &callsign);
	static std::string getDongles();

protected:
	CDExtraHandler(CGroupHandler *handler, const std::string &reflector, const std::string &repeater, CDExtraProtocolHandler *protoHandler, const std::string &address, unsigned short port, DIRECTION direction);
	~CDExtraHandler();

	void processInt(CHeaderData &header);
	void processInt(CAMBEData &data);
	bool processInt(CConnectData &connect, CD_TYPE type);

	void writeHeaderInt(CGroupHandler *handler, CHeaderData &header, DIRECTION direction);
	void writeAMBEInt(CGroupHandler *handler, CAMBEData &data, DIRECTION direction);

	bool clockInt(unsigned int ms);

private:
	static std::list<CDExtraHandler *> m_DExtraHandlers;

	static std::string                 m_callsign;
	static CDExtraProtocolHandlerPool *m_pool;

	static CCallsignList *m_whiteList;
	static CCallsignList *m_blackList;

	std::string             m_reflector;
	std::string             m_repeater;
	CDExtraProtocolHandler *m_handler;
	std::string             m_yourAddress;
	unsigned short          m_yourPort;
	DIRECTION               m_direction;
	DEXTRA_STATE            m_linkState;
	CGroupHandler          *m_destination;
	time_t                  m_time;
	CTimer                  m_pollTimer;
	CTimer                  m_pollInactivityTimer;
	CTimer                  m_tryTimer;
	unsigned int            m_tryCount;
	unsigned int            m_dExtraId;
	unsigned int            m_dExtraSeq;
	CTimer                  m_inactivityTimer;
	CHeaderData            *m_header;

	unsigned int calcBackoff();
};
