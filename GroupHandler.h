/*
 *   Copyright (C) 2011-2014 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017,2020 by Thomas A. Early N7TAE
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
#include <map>
#include <list>
#include <set>

#include "RemoteGroup.h"
#include "G2ProtocolHandler.h"
#include "RepeaterCallback.h"
#include "CacheManager.h"
#include "DStarDefines.h"
#include "HeaderData.h"
#include "AMBEData.h"
#include "IRCDDB.h"
#include "Timer.h"

enum LOGUSER {
	LU_ON,
	LU_OFF
};

class CSGSUser {
public:
	CSGSUser(const std::string& callsign, unsigned int timeout);
	~CSGSUser();

	void reset();

	bool clock(unsigned int ms);
	bool hasExpired();

	std::string getCallsign() const;
	CTimer getTimer() const;

private:
	std::string m_callsign;
	CTimer m_timer;
};

class CSGSId {
public:
	CSGSId(unsigned int id, unsigned int timeout, CSGSUser* user);
	~CSGSId();

	unsigned int getId() const;

	void reset();

	void setLogin();
	void setInfo();
	void setLogoff();
	void setEnd();

	bool clock(unsigned int ms);
	bool hasExpired();

	bool isLogin() const;
	bool isLogoff() const;
	bool isEnd() const;

	CSGSUser* getUser() const;

private:
	unsigned int   m_id;
	CTimer         m_timer;
	bool           m_login;
	bool           m_logoff;
	bool           m_end;
	CSGSUser      *m_user;
};

class CSGSRepeater {
public:
	std::string dest;
	std::string rptr;
	std::string gate;
	std::string	addr;
};

class CGroupHandler {
public:
	static void add(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, unsigned int userTimeout, bool listenOnly, bool showlink, const std::string & eflector);
	static void setG2Handler(CG2ProtocolHandler *handler0, CG2ProtocolHandler *handler1);
	static void setIRC(CIRCDDB *irc0, CIRCDDB *irc1);
	static void setGateway(const std::string &gateway);
	static void link();

	static std::list<std::string> listGroups();

	static CGroupHandler *findGroup(const std::string &callsign);
	static CGroupHandler *findGroup(const CHeaderData &header);
	static CGroupHandler *findGroup(const CAMBEData &data);

	static void finalise();

	static void clock(unsigned int ms);

    // these two process functions are for the G2Handler
	void process(CHeaderData &header);
	void process(CAMBEData &data);

	bool remoteLink(const std::string &reflector);
	void updateReflectorInfo();
	DSTAR_LINKTYPE getLinkType();
	void setLinkType(DSTAR_LINKTYPE linkType);
	void clearReflector();

	CRemoteGroup *getInfo() const;

	bool LogoffUser(const std::string& callsign);

    // these two process functions are for linked reflectors
	bool process(CHeaderData &header, DIRECTION direction, AUDIO_SOURCE source);
	bool process(CAMBEData &data, DIRECTION direction, AUDIO_SOURCE source);

	void linkUp(DSTAR_PROTOCOL protocol, const std::string &callsign);
	void linkRefused(DSTAR_PROTOCOL protocol, const std::string &callsign);
	bool linkFailed(DSTAR_PROTOCOL protocol, const std::string &callsign, bool isRecoverable);

	bool singleHeader();

protected:
	CGroupHandler(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, unsigned int userTimeout, bool listenOnly, bool showlink, const std::string &reflector);
	~CGroupHandler();

	bool linkInt();
	void clockInt(unsigned int ms);

private:
	static std::list<CGroupHandler *> m_Groups;

	static CG2ProtocolHandler *m_g2Handler[2];
	static CIRCDDB            *m_irc[2];
	static std::string         m_gateway;

	static std::string         m_name;

	// Group info
	std::string    m_groupCallsign;
	std::string    m_offCallsign;
	std::string    m_repeater;
	std::string    m_infoText;
	std::string    m_linkReflector;
	std::string    m_linkGateway;
	LINK_STATUS    m_linkStatus;
	LINK_STATUS    m_oldlinkStatus;
	CTimer         m_linkTimer;
	DSTAR_LINKTYPE m_linkType;
	unsigned int   m_id;
	CTimer         m_announceTimer;
	CTimer         m_pingTimer;
	unsigned int   m_userTimeout;
	bool           m_listenOnly;
	bool           m_showlink;
	std::map<unsigned int, CSGSId *>      m_ids;
	std::map<std::string, CSGSUser *>     m_users;
	std::map<std::string, CSGSRepeater *> m_repeaters;

	void sendFromText();
	void sendToRepeaters(CHeaderData &header) const;
	void sendToRepeaters(CAMBEData &data) const;
	void sendAck(const int index, const std::string &user, const std::string &text) const;
	void logUser(LOGUSER lu, const std::string channel, const std::string user);
};
