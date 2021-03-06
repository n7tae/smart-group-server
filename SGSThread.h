/*
 *   Copyright (C) 2010-2015 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2019 by Thomas A. Early N7TAE
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
#include <atomic>

#include "DExtraProtocolHandlerPool.h"		// DEXTRA_LINK
#include "DCSProtocolHandlerPool.h"			// DCS_LINK
#include "G2ProtocolHandler.h"
#include "RemoteHandler.h"
#include "IRCDDB.h"
#include "Timer.h"
#include "Defs.h"

class CSGSThread {
public:
	CSGSThread(unsigned int countDExtra, unsigned int countDCS);

	static void SignalCatch(const int signum);
//	bool init();
	void run();

	~CSGSThread();

	void setCallsign(const std::string& callsign);

	void addGroup(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, unsigned int userTimeout, bool listen_only, bool showlink, const std::string &reflector);

	void setRemote(bool enabled, const std::string& password, unsigned short port, bool is_ipv6);
	void setIRC(const unsigned int i, CIRCDDB* irc);

private:
	unsigned int m_countDExtra;
	unsigned int m_countDCS;
	static std::atomic<bool> m_killed;
	bool		m_stopped;
	std::string	m_callsign;
	std::string	m_address;

	CG2ProtocolHandler *m_g2Handler[2];
	CIRCDDB            *m_irc[2];
	bool				m_logEnabled;
	CTimer				m_statusTimer;
	IRCDDB_STATUS		m_lastStatus;
	bool				m_remoteEnabled;
	std::string			m_remotePassword;
	unsigned short		m_remotePort;
	bool				m_remoteIPV6;
	CRemoteHandler     *m_remote;

	void processIrcDDB(const int i);
	void processG2(const int i);
	void loadReflectors(const std::string fname, DSTAR_PROTOCOL dstarProtocol);

	void processDExtra(CDExtraProtocolHandlerPool *dextraPool);
	void processDCS(CDCSProtocolHandlerPool *dcsPool);
};
