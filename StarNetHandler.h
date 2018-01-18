/*
 *   Copyright (C) 2011-2014 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017,2018 by Thomas A. Early N7TAE
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

#include "RemoteStarNetGroup.h"
#include "G2ProtocolHandler.h"
#include "ReflectorCallback.h"		// DEXTRA_LINK || DCS_LINK
#include "RepeaterCallback.h"
#include "TextCollector.h"
#include "CacheManager.h"
#include "DStarDefines.h"
#include "HeaderData.h"
#include "AMBEData.h"
#include "IRCDDB.h"
#include "Timer.h"


class CStarNetUser {
public:
	CStarNetUser(const std::string& callsign, unsigned int timeout);
	~CStarNetUser();

	void reset();

	bool clock(unsigned int ms);
	bool hasExpired();

	std::string getCallsign() const;
	CTimer getTimer() const;

private:
	std::string m_callsign;
	CTimer m_timer;
};

class CStarNetId {
public:
	CStarNetId(unsigned int id, unsigned int timeout, CStarNetUser* user);
	~CStarNetId();

	unsigned int getId() const;

	void reset();

	void setLogin();
	void setInfo();
	void setLogoff();
	void setEnd();

	bool clock(unsigned int ms);
	bool hasExpired();

	bool isLogin() const;
	bool isInfo() const;
	bool isLogoff() const;
	bool isEnd() const;

	CStarNetUser* getUser() const;

	CTextCollector& getTextCollector();

private:
	unsigned int   m_id;
	CTimer         m_timer;
	bool           m_login;
	bool           m_info;
	bool           m_logoff;
	bool           m_end;
	CStarNetUser*  m_user;
	CTextCollector m_textCollector;
};

class CStarNetRepeater {
public:
	std::string        m_destination;
	std::string        m_repeater;
	std::string        m_gateway;
	in_addr            m_address;
	IRepeaterCallback* m_local;
};

class CStarNetHandler : public IReflectorCallback {
public:
	static void add(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, const std::string &permanent,
										unsigned int userTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string & eflector);
	static void setG2Handler(CG2ProtocolHandler *handler);
	static void setIRC(CIRCDDB *irc);
	static void setCache(CCacheManager *cache);
	static void setGateway(const std::string &gateway);
	static void link();

	static std::list<std::string> listStarNets();

	static CStarNetHandler *findStarNet(const std::string &callsign);
	static CStarNetHandler *findStarNet(const CHeaderData &header);
	static CStarNetHandler *findStarNet(const CAMBEData &data);

	static void finalise();

	static void clock(unsigned int ms);

	void process(CHeaderData &header);
	void process(CAMBEData &data);
	bool remoteLink(const std::string &reflector);
	void updateReflectorInfo();
	DSTAR_LINKTYPE getLinkType();

	CRemoteStarNetGroup *getInfo() const;

	bool logoff(const std::string& callsign);

	virtual bool process(CHeaderData &header, DIRECTION direction, AUDIO_SOURCE source);
	virtual bool process(CAMBEData &data, DIRECTION direction, AUDIO_SOURCE source);

	virtual void linkUp(DSTAR_PROTOCOL protocol, const std::string &callsign);
	virtual void linkRefused(DSTAR_PROTOCOL protocol, const std::string &callsign);
	virtual bool linkFailed(DSTAR_PROTOCOL protocol, const std::string &callsign, bool isRecoverable);

	virtual bool singleHeader();

protected:
	CStarNetHandler(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, const std::string &permanent,
												unsigned int userTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string &reflector);
	virtual ~CStarNetHandler();

	bool linkInt();
	void clockInt(unsigned int ms);

private:
	static std::list<CStarNetHandler *> m_starNets;

	static CG2ProtocolHandler *m_g2Handler;
	static CIRCDDB            *m_irc;
	static CCacheManager      *m_cache;
	static std::string         m_gateway;

	static std::string         m_name;

	// Group info
	std::string    m_groupCallsign;
	std::string    m_offCallsign;
	std::string    m_shortCallsign;
	std::string    m_repeater;
	std::string    m_infoText;
	std::set<std::string>  m_permanent;
	std::string    m_linkReflector;
	std::string    m_linkGateway;
	LINK_STATUS    m_linkStatus;
	LINK_STATUS    m_oldlinkStatus;
	CTimer         m_linkTimer;
	DSTAR_LINKTYPE m_linkType;

	unsigned int   m_id;
	CTimer         m_announceTimer;
	unsigned int   m_userTimeout;
	STARNET_CALLSIGN_SWITCH  m_callsignSwitch;
	bool                     m_txMsgSwitch;

	std::map<unsigned int, CStarNetId *>      m_ids;
	std::map<std::string, CStarNetUser *>     m_users;
	std::map<std::string, CStarNetRepeater *> m_repeaters;

	void sendFromText(const std::string &text) const;
	void sendToRepeaters(CHeaderData &header) const;
	void sendToRepeaters(CAMBEData &data) const;
	void sendAck(const CUserData &user, const std::string &text) const;
	void logoffUser(const std::string channel, const std::string user);
};
