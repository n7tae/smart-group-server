// Copyright (c) 2017 by Thomas Early N7TAE

#pragma once

#include <string>

#include "DExtraProtocolHandlerPool.h"		// DEXTRA_LINK
#include "DCSProtocolHandlerPool.h"			// DCS_LINK
#include "G2ProtocolHandler.h"
#include "RemoteHandler.h"
#include "CacheManager.h"
#include "IRCDDB.h"
#include "Timer.h"
#include "Defs.h"

class CSmartServerThread {
public:
	CSmartServerThread(bool nolog, const std::string& logDir);
	virtual ~CSmartServerThread();

	virtual void setCallsign(const std::string& callsign);
	virtual void setAddress(const std::string& address);
	
#if defined(DEXTRA_LINK) || defined(DCS_LINK)
	virtual void addStarNet(const std::string& callsign, const std::string& logoff, const std::string& repeater, const std::string& infoText, const std::string& permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string& reflector);
#else
	virtual void addStarNet(const std::string& callsign, const std::string& logoff, const std::string& repeater, const std::string& infoText, const std::string& permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch);
#endif

	virtual void setRemote(bool enabled, const std::string& password, unsigned int port);
	virtual void setIRC(CIRCDDB* irc);

	virtual void run();
	virtual void kill();

private:
	bool		m_nolog;
	std::string	m_logDir;
	bool		m_killed;
	bool		m_stopped;
	std::string	m_callsign;
	std::string	m_address;
	
#if defined(DEXTRA_LINK)
	CDExtraProtocolHandlerPool*	m_dextraPool;
#endif

#if defined(DCS_LINK)
	CDCSProtocolHandlerPool*	m_dcsPool;
#endif

	CG2ProtocolHandler*	m_g2Handler;
	CIRCDDB*			m_irc;
	CCacheManager 		m_cache;
	bool				m_logEnabled;
	CTimer				m_statusTimer;
	IRCDDB_STATUS		m_lastStatus;
	bool				m_remoteEnabled;
	std::string			m_remotePassword;
	unsigned int		m_remotePort;
	CRemoteHandler*		m_remote;

	void processIrcDDB();
	void processG2();
	
#if defined(DEXTRA_LINK)
	void processDExtra();
	void loadReflectors(const char *fname);
#endif

#if defined(DCS_LINK)
	void processDCS();
	void loadReflectors(const char *fname);
#endif
};

