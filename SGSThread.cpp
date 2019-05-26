/*
 *   Copyright (C) 2010-2013,2015 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2019 by Thomas Early N7TAE
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

#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pwd.h>
#include <ctime>
#include <fstream>
#include <cstring>
#include <cassert>

#include "SGSThread.h"
#include "GroupHandler.h"
#include "DExtraHandler.h"			// DEXTRA LINK
#include "DCSHandler.h"				// DCS LINK
#include "HeaderData.h"
#include "G2Handler.h"
#include "AMBEData.h"
#include "Utils.h"

const unsigned int REMOTE_DUMMY_PORT = 65015U;

CSGSThread::CSGSThread(unsigned int countDExtra, unsigned int countDCS) :
m_countDExtra(countDExtra),
m_countDCS(countDCS),
m_killed(false),
m_stopped(true),
m_callsign(),
m_address(),
m_cache(),
m_logEnabled(false),
m_statusTimer(1000U, 1U),		// 1 second
m_lastStatus(IS_DISCONNECTED),
m_remoteEnabled(false),
m_remotePassword(),
m_remotePort(0U),
m_remote(NULL)
{
	m_g2Handler[0] = m_g2Handler[1] = NULL;
	m_irc[0] = m_irc[1] = NULL;
	CHeaderData::initialise();
	printf("SGSThread created. DExtra channels: %d, DCS Channels: %d\n", countDExtra, countDCS);
}

CSGSThread::~CSGSThread()
{
	CHeaderData::finalise();
	CGroupHandler::finalise();
	CDExtraHandler::finalise();
	CDCSHandler::finalise();

	printf("SGSThread destroyed\n");
}

void CSGSThread::run()
{
	int family[2] = { AF_UNSPEC, AF_UNSPEC };
	for (int i=0; m_irc[i] && i<2; i++) {
		while (AF_UNSPEC == family[i]) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			family[i] = m_irc[i]->GetFamily();
		}
		printf("IRC Server %d family is %s\n", i, ((AF_INET==family[i]) ? "IPV4" : ((AF_INET6==family[i]) ? "IPV6" : "Family UNSPECIFIED")));
		if (AF_INET6 == family[i])
			m_g2Handler[i] = new CG2ProtocolHandler(family[i], G2_IPV6_PORT);
		else
			m_g2Handler[i] = new CG2ProtocolHandler(family[i], G2_DV_PORT);
		
		bool ret = m_g2Handler[i]->open();
		if (!ret) {
			printf("Could not open the G2 protocol handler\n");
			delete m_g2Handler[i];
			m_g2Handler[i] = NULL;
		}
	}

	if (m_irc[1] && family[0] == family[1]) {
		fprintf(stderr, "Each irc server must be from a different IP family\n");
		m_killed = true;
	}

	if (m_irc[1] && AF_INET6==family[1]) {
		fprintf(stderr, "For a two irc server system, the first must be IPV6\n");
		m_killed = true;
	}

	// Wait here until we have the essentials to run
	while (!m_killed && 0==m_callsign.size())
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	if (m_killed) {
		for (int i=0; i<2; i++) {
			if (m_g2Handler[i]) {
				m_g2Handler[i]->close();
				delete m_g2Handler[i];
			}
			if (m_irc[i]) {
				m_irc[i]->close();
				delete m_irc[i];
			}
		}
		return;
	}
	m_stopped = false;

	printf("Starting the Smart Group Server thread\n");

	loadReflectors(DEXTRA_HOSTS_FILE_NAME, DP_DEXTRA);
	loadReflectors(DCS_HOSTS_FILE_NAME, DP_DCS);
	CDExtraProtocolHandlerPool dextraPool(DEXTRA_PORT);
	CDCSProtocolHandlerPool dcsPool(DCS_PORT);

	CDExtraHandler::setCallsign(m_callsign);
	CDExtraHandler::setDExtraProtocolHandlerPool(&dextraPool);
	CDCSHandler::setDCSProtocolHandlerPool(&dcsPool);
	CDCSHandler::setGatewayType(GT_SMARTGROUP);

	CGroupHandler::setCache(&m_cache);
	CGroupHandler::setGateway(m_callsign);
	CGroupHandler::setG2Handler(m_g2Handler[0], m_g2Handler[1]);
	CGroupHandler::setIRC(m_irc[0], m_irc[1]);
	if (m_countDExtra || m_countDCS)
		CGroupHandler::link();

	if (m_remoteEnabled && m_remotePassword.size() && m_remotePort > 0U) {
		m_remote = new CRemoteHandler(m_remotePassword, m_remotePort, m_remoteIPV6);
		bool res = m_remote->open();
		if (!res) {
			delete m_remote;
			m_remote = NULL;
		}
	}

	time_t start;
	time(&start);

	m_statusTimer.start();

	try {
		while (!m_killed) {
			processIrcDDB(0);
			processG2(0);
			if (m_irc[1]) {
				processIrcDDB(1);
				processG2(1);
			}
			processDExtra(&dextraPool);
			processDCS(&dcsPool);
			if (m_remote != NULL)
				m_remote->process();

			time_t now;
			time(&now);
			unsigned long ms = (unsigned long)(1000.0 * difftime(now, start));
			time(&start);

			m_statusTimer.clock(ms);
			CGroupHandler::clock(ms);
			CDExtraHandler::clock(ms);
			CDCSHandler::clock(ms);

			std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PER_TIC_MS));
		}
	}
	catch (std::exception& e) {
		printf("Exception raised - \"%s\"\n", e.what());
	}
	catch (...) {
		printf("Unknown exception raised\n");
	}

	printf("Stopping the Smart Group Server thread\n");

	// Unlink from all reflectors
	CDExtraHandler::unlink();
	dextraPool.close();

	// Unlink from all reflectors
	CDCSHandler::unlink();
	dcsPool.close();

	m_g2Handler[0]->close();
	delete m_g2Handler[0];
	if (m_g2Handler[1]) {
		m_g2Handler[1]->close();
		delete m_g2Handler[1];
	}

	if (m_irc[0]) {
		m_irc[0]->close();
		delete m_irc[0];
	}

	if (m_irc[1]) {
		m_irc[1]->close();
		delete m_irc[1];
	}

	if (m_remote != NULL) {
		m_remote->close();
		delete m_remote;
	}
}

void CSGSThread::kill()
{
	m_killed = true;
}

void CSGSThread::setCallsign(const std::string &callsign)
{
	if (!m_stopped)
		return;

	m_callsign = callsign;
}

// void CSGSThread::setAddress(const std::string &address)
// {
// 	m_address = address;
// }

void CSGSThread::addGroup(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText,
	unsigned int userTimeout, CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, bool listen_only, const std::string &reflector)
{
	CGroupHandler::add(callsign, logoff, repeater, infoText, userTimeout, callsignSwitch, txMsgSwitch, listen_only, reflector);
}

void CSGSThread::setIRC(const unsigned int i, CIRCDDB* irc)
{
	assert(irc != NULL);

	m_irc[i] = irc;
}

void CSGSThread::setRemote(bool enabled, const std::string& password, unsigned short port, bool is_ipv6)
{
	if (enabled) {
		m_remoteEnabled  = true;
		m_remotePassword = password;
		m_remotePort     = port;
		m_remoteIPV6     = is_ipv6;
	} else {
		m_remoteEnabled  = false;
		m_remotePassword = password;
		m_remotePort     = REMOTE_DUMMY_PORT;
		m_remoteIPV6     = false;
	}
}

void CSGSThread::processIrcDDB(const int i)
{
	// Once per second
	if (m_statusTimer.hasExpired()) {
		int status = m_irc[i]->getConnectionState();
		switch (status) {
			case 0:
			case 10:
				if (m_lastStatus != IS_DISCONNECTED) {
					printf("Disconnected from ircDDB\n");
					m_lastStatus = IS_DISCONNECTED;
				}
				break;
			case 7:
				if (m_lastStatus != IS_CONNECTED) {
					printf("Connected to ircDDB\n");
					m_lastStatus = IS_CONNECTED;
				}
				break;
			default:
				if (m_lastStatus != IS_CONNECTING) {
					printf("Connecting to ircDDB\n");
					m_lastStatus = IS_CONNECTING;
				}
				break;
		}

		m_statusTimer.start();
	}

	// Process all incoming ircDDB messages, updating the caches
	while (true) {
		IRCDDB_RESPONSE_TYPE type = m_irc[i]->getMessageType();

		switch (type) {
			case IDRT_NONE:
				return;

			case IDRT_USER: {
					std::string user, repeater, gateway, address, timestamp;
					bool res = m_irc[i]->receiveUser(user, repeater, gateway, address, timestamp);
					if (!res)
						break;

					if (address.size()) {
						//printf("USER[%d]: %s %s %s %s\n", i, user.c_str(), repeater.c_str(), gateway.c_str(), address.c_str());
						m_cache.updateUser(user, repeater, gateway, address, timestamp, DP_DEXTRA, false, false);
					} else {
						printf("USER: %s has no IP address!\n", user.c_str());
					}
				}
				break;

			case IDRT_REPEATER: {
					std::string repeater, gateway, address;
					bool res = m_irc[i]->receiveRepeater(repeater, gateway, address);
					if (!res)
						break;

					if (address.size()) {
						//printf("REPEATER[%d]: %s %s %s\n", i, repeater.c_str(), gateway.c_str(), address.c_str());
						m_cache.updateRepeater(repeater, gateway, address, DP_DEXTRA, false, false);
					} else {
						printf("REPEATER: %s NOT FOUND\n", repeater.c_str());
					}
				}
				break;

			case IDRT_GATEWAY: {
					std::string gateway, address;
					bool res = m_irc[i]->receiveGateway(gateway, address);
					if (!res)
						break;

					CDExtraHandler::gatewayUpdate(gateway, address);

					CDCSHandler::gatewayUpdate(gateway, address);

					if (0 == address.size()) {
						//printf("GATEWAY[%d]: %s %s\n", i, gateway.c_str(), address.c_str());
						m_cache.updateGateway(gateway, address, DP_DEXTRA, false, false);
					} else {
						printf("GATEWAY: %s NOT FOUND\n", gateway.c_str());
					}
				}
				break;
		}
	}
}

void CSGSThread::processDExtra(CDExtraProtocolHandlerPool *dextraPool)
{
	while (true) {
		DEXTRA_TYPE type = dextraPool->read();

		switch (type) {
			case DE_NONE:
				return;

			case DE_POLL: {
					CPollData* poll = dextraPool->newPoll();
					if (poll != NULL) {
						CDExtraHandler::process(*poll);
						delete poll;
					}
				}
				break;

			case DE_CONNECT: {
					CConnectData* connect = dextraPool->newConnect();
					if (connect != NULL) {
						CDExtraHandler::process(*connect);
						delete connect;
					}
				}
				break;

			case DE_HEADER: {
					CHeaderData* header = dextraPool->newHeader();
					if (header != NULL) {
						// printf("DExtra header - My: %s/%s  Your: %s  Rpt1: %s  Rpt2: %s\n", header->getMyCall1().c_str(), header->getMyCall2().c_str(), header->getYourCall().c_str(), header->getRptCall1().c_str(), header->getRptCall2().c_str());
						CDExtraHandler::process(*header);
						delete header;
					}
				}
				break;

			case DE_AMBE: {
					CAMBEData* data = dextraPool->newAMBE();
					if (data != NULL) {
						CDExtraHandler::process(*data);
						delete data;
					}
				}
				break;
		}
	}
}

void CSGSThread::processDCS(CDCSProtocolHandlerPool *dcsPool)
{
	while (true) {
		DCS_TYPE type = dcsPool->read();

		switch (type) {
			case DC_NONE:
				return;

			case DC_POLL: {
					CPollData* poll = dcsPool->readPoll();
					if (poll != NULL) {
						CDCSHandler::process(*poll);
						delete poll;
					}
				}
				break;

			case DC_CONNECT: {
					CConnectData* connect = dcsPool->readConnect();
					if (connect != NULL) {
						CDCSHandler::process(*connect);
						delete connect;
					}
				}
				break;

			case DC_DATA: {
					CAMBEData* data = dcsPool->readData();
					if (data != NULL) {
						// printf("DCS header - My: %s/%s  Your: %s  Rpt1: %s  Rpt2: %s\n", header->getMyCall1().c_str(), header->getMyCall2().c_str(), header->getYourCall().c_str(), header->getRptCall1().c_str(), header->getRptCall2().c_str());
						CDCSHandler::process(*data);
						delete data;
					}
				}
				break;
		}
	}
}

void CSGSThread::processG2(const int i)
{
	while(true) {
		G2_TYPE type = m_g2Handler[i]->read();

		switch (type) {
			case GT_NONE:
				return;

			case GT_HEADER: {
					CHeaderData* header = m_g2Handler[i]->readHeader();
					if (header != NULL) {
//printf("G2 header - My: %s/%s  Your: %s  Rpt1: %s  Rpt2: %s  Flags: %02X %02X %02X\n", header->getMyCall1().c_str(), header->getMyCall2().c_str(), header->getYourCall().c_str(), header->getRptCall1().c_str(), header->getRptCall2().c_str(), header->getFlag1(), header->getFlag2(), header->getFlag3());
						CG2Handler::process(*header);
						delete header;
					}
				}
				break;

			case GT_AMBE: {
					CAMBEData* data = m_g2Handler[i]->readAMBE();
					if (data != NULL) {
						CG2Handler::process(*data);
						delete data;
					}
				}
				break;
		}
	}
}

void CSGSThread::loadReflectors(const std::string fname, DSTAR_PROTOCOL dstarProtocol)
{
	std::string filepath(CFG_DIR);
	filepath += std::string("/") + fname;

	struct stat sbuf;
	if (stat(filepath.c_str(), &sbuf)) {
		printf("%s doesn't exist!\n", filepath.c_str());
		return;
	}

	std::ifstream hostfile;
	hostfile.open(filepath, std::ifstream::in);
	char line[256];
	hostfile.getline(line, 256);
	int count=0, tries=0;
	while (hostfile.good()) {
		const char *space = " \t\r";
		char *first = strtok(line, space);
		if (first) {
			if ('#' != first[0]) {
				tries++;
				char *second = strtok(NULL, space);
				if (second) {
					char *third = strtok(NULL, space);
					if (third && '#'==third[0])
						third = NULL;
					std::string name(first);
					name.resize(7, ' ');
					name.push_back('G');
					struct hostent *he = gethostbyname(second);
					if (he) {
						count++;
						std::string address(inet_ntoa(*(struct in_addr*)(he->h_addr_list[0])));
						m_cache.updateGateway(name, address, dstarProtocol, third?1:0, true);
//						printf("reflector:%s, address:%s lock:%s\n", name.c_str(), address.c_str(), third?"true":"false");
					}
				}
			}
		}
		hostfile.getline(line, 256);
	}

	printf("Loaded %u of %u %s reflectors\n", count, tries, DP_DEXTRA==dstarProtocol?"DExtra":"DCS");
}
