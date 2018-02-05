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

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include "SlowDataEncoder.h"
#include "RepeaterHandler.h"
#include "StarNetHandler.h"
#include "DExtraHandler.h"		// DEXTRA_LINK
#include "DCSHandler.h"			// DCS_LINK
#include "Utils.h"

const unsigned int MESSAGE_DELAY = 4U;

// define static members
CG2ProtocolHandler *CStarNetHandler::m_g2Handler = NULL;
CIRCDDB            *CStarNetHandler::m_irc = NULL;
CCacheManager      *CStarNetHandler::m_cache = NULL;
std::string         CStarNetHandler::m_gateway;
std::list<CStarNetHandler *> CStarNetHandler::m_starNets;


CStarNetUser::CStarNetUser(const std::string &callsign, unsigned int timeout) :
m_callsign(callsign),
m_timer(1000U, timeout)
{
	m_timer.start();
}

CStarNetUser::~CStarNetUser()
{
}

bool CStarNetUser::clock(unsigned int ms)
{
	m_timer.clock(ms);

	return m_timer.isRunning() && m_timer.hasExpired();
}

bool CStarNetUser::hasExpired()
{
	return m_timer.isRunning() && m_timer.hasExpired();
}

void CStarNetUser::reset()
{
	m_timer.start();
}

std::string CStarNetUser::getCallsign() const
{
	return m_callsign;
}

CTimer CStarNetUser::getTimer() const
{
	return m_timer;
}

CStarNetId::CStarNetId(unsigned int id, unsigned int timeout, CStarNetUser *user) :
m_id(id),
m_timer(1000U, timeout),
m_login(false),
m_info(false),
m_logoff(false),
m_end(false),
m_user(user),
m_textCollector()
{
	assert(user != NULL);

	m_timer.start();
}

CStarNetId::~CStarNetId()
{
}

unsigned int CStarNetId::getId() const
{
	return m_id;
}

void CStarNetId::reset()
{
	m_timer.start();
}

void CStarNetId::setLogin()
{
	m_login = true;
}

void CStarNetId::setInfo()
{
	if (!m_login && !m_logoff)
		m_info = true;
}

void CStarNetId::setLogoff()
{
	if (!m_login && !m_info)
		m_logoff = true;
}

void CStarNetId::setEnd()
{
	m_end = true;
}

bool CStarNetId::clock(unsigned int ms)
{
	m_timer.clock(ms);

	return m_timer.isRunning() && m_timer.hasExpired();
}

bool CStarNetId::hasExpired()
{
	return m_timer.isRunning() && m_timer.hasExpired();
}

bool CStarNetId::isLogin() const
{
	return m_login;
}

bool CStarNetId::isInfo() const
{
	return m_info;
}

bool CStarNetId::isLogoff() const
{
	return m_logoff;
}

bool CStarNetId::isEnd() const
{
	return m_end;
}

CStarNetUser* CStarNetId::getUser() const
{
	return m_user;
}

CTextCollector& CStarNetId::getTextCollector()
{
	return m_textCollector;
}

void CStarNetHandler::add(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, const std::string &permanent,
														unsigned int userTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string &reflector)
{
	CStarNetHandler *starNet = new CStarNetHandler(callsign, logoff, repeater, infoText, permanent, userTimeout, callsignSwitch, txMsgSwitch, reflector);

	if (starNet)
		m_starNets.push_back(starNet);
	else
		printf("Cannot allocate StarNet group with callsign %s\n", callsign.c_str());
}

void CStarNetHandler::setG2Handler(CG2ProtocolHandler *handler)
{
	assert(handler != NULL);

	m_g2Handler = handler;
}

void CStarNetHandler::setIRC(CIRCDDB *irc)
{
	assert(irc != NULL);

	m_irc = irc;
}

void CStarNetHandler::setCache(CCacheManager *cache)
{
	assert(cache != NULL);

	m_cache = cache;
}

void CStarNetHandler::setGateway(const std::string &gateway)
{
	m_gateway = gateway;
}

CStarNetHandler *CStarNetHandler::findStarNet(const std::string &callsign)
{
	for (auto it=m_starNets.begin(); it!=m_starNets.end(); it++) {
		if (0 == (*it)->m_groupCallsign.compare(callsign))
			return *it;
	}
	return NULL;
}

CStarNetHandler *CStarNetHandler::findStarNet(const CHeaderData &header)
{
	std::string your = header.getYourCall();

	for (auto it=m_starNets.begin(); it!=m_starNets.end(); it++) {
		if (0 == (*it)->m_groupCallsign.compare(your))
			return *it;
		if (0 == (*it)->m_offCallsign.compare(your))
			return *it;
	}
	return NULL;
}

CStarNetHandler *CStarNetHandler::findStarNet(const CAMBEData &data)
{
	unsigned int id = data.getId();

	for (auto it=m_starNets.begin(); it!=m_starNets.end(); it++) {
		if ((*it)->m_id == id)
			return *it;
	}
	return NULL;
}

std::list<std::string> CStarNetHandler::listStarNets()
{
	std::list<std::string> starNets;

	for (auto it=m_starNets.begin(); it!=m_starNets.end(); it++)
		starNets.push_back((*it)->m_groupCallsign);

	return starNets;
}

CRemoteStarNetGroup *CStarNetHandler::getInfo() const
{
	CRemoteStarNetGroup *data = new CRemoteStarNetGroup(m_groupCallsign, m_offCallsign, m_repeater, m_infoText, m_linkReflector, m_linkStatus, m_userTimeout);

	for (auto it=m_users.begin(); it!=m_users.end(); ++it) {
		CStarNetUser* user = it->second;
		data->addUser(user->getCallsign(), user->getTimer().getTimer(), user->getTimer().getTimeout());
	}

	return data;
}

void CStarNetHandler::finalise()
{
	while (m_starNets.size()) {
		delete m_starNets.front();
		m_starNets.pop_front();
	}
}

void CStarNetHandler::clock(unsigned int ms)
{
	for (auto it=m_starNets.begin(); it!=m_starNets.end(); it++)
		(*it)->clockInt(ms);
}

void CStarNetHandler::link()
{
	for (auto it=m_starNets.begin(); it!=m_starNets.end(); it++)
		(*it)->linkInt();
}

CStarNetHandler::CStarNetHandler(const std::string &callsign, const std::string &logoff, const std::string &repeater, const std::string &infoText, const std::string &permanent,
																unsigned int userTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string &reflector) :
m_groupCallsign(callsign),
m_offCallsign(logoff),
m_shortCallsign("SMRT"),
m_repeater(repeater),
m_infoText(infoText),
m_permanent(),
m_linkReflector(reflector),
m_linkGateway(),
m_linkStatus(LS_NONE),
m_oldlinkStatus(LS_INIT),
m_linkTimer(1000U, NETWORK_TIMEOUT),
m_id(0x00U),
m_announceTimer(1000U, 2U * 60U),		// 2 minutes
m_userTimeout(userTimeout),
m_callsignSwitch(callsignSwitch),
m_txMsgSwitch(txMsgSwitch),
m_ids(),
m_users(),
m_repeaters()
{
	m_announceTimer.start();

	// set link type
	if (m_linkReflector.size())
		m_linkType = (0 == m_linkReflector.compare(0, 3, "XRF")) ? LT_DEXTRA : LT_DCS;
	else
		m_linkType = LT_NONE;

	// Create the short version of the Smart Group callsign
	if (0 == m_groupCallsign.compare(0, 3, "SGS")) {
		if (' ' == m_groupCallsign[7])
			m_shortCallsign = std::string("S") + m_groupCallsign.substr(3, 3);
		else
			m_shortCallsign = m_groupCallsign.substr(3, 3) + m_groupCallsign[7];
	}
	if (permanent.size() < 4)
		return;
	char *buf = (char *)calloc(permanent.size() + 1, 1);
	if (buf) {
		strcpy(buf, permanent.c_str());
		char *token = strtok(buf, ",");
		while (token) {
			if (strlen(token)) {
				std::string newcall(token);
				if (newcall.size() > 3) {
					CUtils::ToUpper(newcall);
					newcall.resize(LONG_CALLSIGN_LENGTH, ' ');
					m_permanent.insert(newcall);
					token = strtok(NULL, ",");
				}
			}
		}
		free(buf);
	}
}

CStarNetHandler::~CStarNetHandler()
{
	for (auto it = m_ids.begin(); it != m_ids.end(); it++)
		delete it->second;
	m_ids.empty();

	for (auto it = m_users.begin(); it != m_users.end(); ++it)
		delete it->second;
	m_users.empty();

	for (auto it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
		delete it->second;
	m_repeaters.empty();
	m_permanent.erase(m_permanent.begin(), m_permanent.end());
}

void CStarNetHandler::process(CHeaderData &header)
{
	std::string my   = header.getMyCall1();
	std::string your = header.getYourCall();
//printf("CStarNetHandler::Process(CHeaderData) my=%s ur=%s\n", my.c_str(), your.c_str());
	unsigned int id = header.getId();

	CStarNetUser* user = m_users[my];
	bool islogin = false;

	// Ensure that this user is in the cache
	CUserData* userData = m_cache->findUser(my);
	if (userData == NULL)
		m_irc->findUser(my);

	if (0 == your.compare(m_groupCallsign)) {
		// This is a normal message for logging in/relaying
		if (user == NULL) {
			// This is a new user, add them to the list
			printf("Adding %s to Smart Group %s\n", my.c_str(), your.c_str());
			logUser(LU_ON, your, my);	// inform Quadnet
			user = new CStarNetUser(my, m_userTimeout * 60U);
			m_users[my] = user;

			CStarNetId* tx = new CStarNetId(id, MESSAGE_DELAY, user);
			tx->setLogin();
			m_ids[id] = tx;
			islogin = true;
		} else {
			user->reset();

			// Check that it isn't a duplicate header
			CStarNetId* tx = m_ids[id];
			if (tx != NULL) {
				delete userData;
				return;
			}
			printf("Updating %s on Smart Group %s\n", my.c_str(), your.c_str());
			logUser(LU_ON, your, my);	// this will be an update
			m_ids[id] = new CStarNetId(id, MESSAGE_DELAY, user);
		}
	} else {
		delete userData;
		userData = NULL;

		// This is a logoff message
		if (user == NULL)				// Not a known user, ignore
			return;

		printf("Removing %s from Smart Group %s\n", user->getCallsign().c_str(), m_groupCallsign.c_str());
		logUser(LU_OFF, m_groupCallsign, my);	// inform Quadnet
		// Remove the user from the user list
		m_users.erase(my);

		CStarNetId* tx = new CStarNetId(id, MESSAGE_DELAY, user);
		tx->setLogoff();
		m_ids[id] = tx;

		return;
	}

	if (m_id != 0x00U) {
		delete userData;
		return;
	}

	m_id = id;

	// Change the Your callsign to CQCQCQ
	header.setCQCQCQ();

	header.setFlag1(0x00);
	header.setFlag2(0x00);
	header.setFlag3(0x00);

	if (LT_DEXTRA == m_linkType) {
		if (!islogin) {
			header.setRepeaters(m_linkGateway, m_linkReflector);
			CDExtraHandler::writeHeader(this, header, DIR_OUTGOING);
		}
	} else if (LT_DCS == m_linkType) {
		if (!islogin) {
			header.setRepeaters(m_linkGateway, m_linkReflector);
			CDCSHandler::writeHeader(this, header, DIR_OUTGOING);
		}
	}

	// Get the home repeater of the user
	std::string exclude;
	if (userData != NULL) {
		exclude = userData->getRepeater();
		delete userData;
		userData = NULL;
	}

	// Build new repeater list
	for (std::map<std::string, CStarNetUser *>::const_iterator it = m_users.begin(); it != m_users.end(); ++it) {
		CStarNetUser* user = it->second;
		if (user != NULL) {
			// Find the user in the cache
			CUserData* userData = m_cache->findUser(user->getCallsign());

			if (userData) {
				// Check for the excluded repeater
				if (userData->getRepeater().compare(exclude)) {
					// Find the users repeater in the repeater list, add it otherwise
					CStarNetRepeater* repeater = m_repeaters[userData->getRepeater()];
					if (repeater == NULL) {
						// Add a new repeater entry
						repeater = new CStarNetRepeater;
						repeater->m_destination = std::string("/") + userData->getRepeater().substr(0, 6) + userData->getRepeater().back();
						repeater->m_repeater    = userData->getRepeater();
						repeater->m_gateway     = userData->getGateway();
						repeater->m_address     = userData->getAddress();
						repeater->m_local       = CRepeaterHandler::findDVRepeater(userData->getRepeater());
						m_repeaters[userData->getRepeater()] = repeater;
					}
				}

				delete userData;
				userData = NULL;
			}
		}
	}

	switch (m_callsignSwitch) {
		case SCS_GROUP_CALLSIGN:
			header.setMyCall1(m_groupCallsign);
			header.setMyCall2("SMRT");
			break;
		case SCS_USER_CALLSIGN:
			header.setMyCall1(my);
			header.setMyCall2(m_shortCallsign);
			break;
		default:
			break;
	}
	if (!islogin)
		sendToRepeaters(header);

	if (m_txMsgSwitch)
		sendFromText(my);
}

void CStarNetHandler::process(CAMBEData &data)
{
	unsigned int id = data.getId();

	CStarNetId* tx = m_ids[id];
	if (tx == NULL)
		return;

	tx->reset();

	CStarNetUser* user = tx->getUser();
	user->reset();

	// If we've just logged in, the LOGOFF and INFO commands are disabled
	if (!tx->isLogin()) {
		// If we've already found some slow data, then don't look again
		if (!tx->isLogoff() && !tx->isInfo()) {
			tx->getTextCollector().writeData(data);
			bool hasText = tx->getTextCollector().hasData();
			if (hasText) {
				std::string text = tx->getTextCollector().getData();
				std::string TEMP(text.substr(0,6));
				CUtils::ToUpper(TEMP);
				if (0 == TEMP.compare("LOGOFF")) {
					printf("Removing %s from StarNet group %s, logged off\n", user->getCallsign().c_str(), m_groupCallsign.c_str());

					tx->setLogoff();

					// Ensure that this user is in the cache in time for the logoff ack
					CUserData* cacheUser = m_cache->findUser(user->getCallsign());
					if (cacheUser == NULL)
						m_irc->findUser(user->getCallsign());

					delete cacheUser;
					cacheUser = NULL;
				}
				TEMP = text.substr(0, 4);
				CUtils::ToUpper(TEMP);
				if (0 == TEMP.compare("INFO")) {
					tx->setInfo();

					// Ensure that this user is in the cache in time for the info text
					CUserData* cacheUser = m_cache->findUser(user->getCallsign());
					if (cacheUser == NULL)
						m_irc->findUser(user->getCallsign());

					delete cacheUser;
					cacheUser = NULL;
				}
			}
		}
	}

	if (id == m_id && !tx->isLogin()) {
		if (LT_DEXTRA == m_linkType)
			CDExtraHandler::writeAMBE(this, data, DIR_OUTGOING);
		else if (LT_DCS == m_linkType)
			CDCSHandler::writeAMBE(this, data, DIR_OUTGOING);
		sendToRepeaters(data);
	}

	if (data.isEnd()) {
		if (id == m_id) {
			// Clear the repeater list if we're the relayed id
			for (std::map<std::string, CStarNetRepeater *>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
				delete it->second;
			m_repeaters.clear();
			m_id = 0x00U;
		}

		if (tx->isLogin()) {
			tx->reset();
			tx->setEnd();
		} else if (tx->isLogoff()) {
			m_users.erase(user->getCallsign());
			tx->reset();
			tx->setEnd();
		} else if (tx->isInfo()) {
			tx->reset();
			tx->setEnd();
		} else {
			m_ids.erase(tx->getId());
			delete tx;
		}
	}
}

bool CStarNetHandler::logoff(const std::string &callsign)
{
	if (0 == callsign.compare("ALL     ")) {
		for (std::map<std::string, CStarNetUser *>::iterator it = m_users.begin(); it != m_users.end(); ++it) {
			CStarNetUser* user = it->second;
			if (user) {
				printf("Removing %s from Smart Group %s, logged off by remote control\n", user->getCallsign().c_str(), m_groupCallsign.c_str());
				logUser(LU_OFF, m_groupCallsign, user->getCallsign());	// inform Quadnet
				delete user;
			}
		}

		for (std::map<unsigned int, CStarNetId *>::iterator it = m_ids.begin(); it != m_ids.end(); ++it)
			delete it->second;

		for (std::map<std::string, CStarNetRepeater *>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
			delete it->second;

		m_users.clear();
		m_ids.clear();
		m_repeaters.clear();

		m_id = 0x00U;

		return true;
	} else {
		CStarNetUser* user = m_users[callsign];
		if (user == NULL) {
			printf("Invalid callsign asked to logoff");
			return false;
		}
		printf("Removing %s from Smart Group %s, logged off by remote control\n", user->getCallsign().c_str(), m_groupCallsign.c_str());
		logUser(LU_OFF, m_groupCallsign, user->getCallsign());	// inform Quadnet

		// Find any associated id structure associated with this use, and the logged off user is the
		// currently relayed one, remove his id.
		for (std::map<unsigned int, CStarNetId *>::iterator it = m_ids.begin(); it != m_ids.end(); ++it) {
			CStarNetId* id = it->second;
			if (id != NULL && id->getUser() == user) {
				if (id->getId() == m_id)
					m_id = 0x00U;

				m_ids.erase(it);
				delete id;
				break;
			}
		}

		m_users.erase(callsign);
		delete user;

		// Check to see if we have any users left
		unsigned int count = m_users.size();

		// If none then clear all the data structures
		if (count == 0U) {
			for (std::map<unsigned int, CStarNetId *>::iterator it = m_ids.begin(); it != m_ids.end(); ++it)
				delete it->second;
			for (std::map<std::string, CStarNetRepeater *>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
				delete it->second;

			m_ids.clear();
			m_repeaters.clear();

			m_id = 0x00U;
		}

		return true;
	}
}

bool CStarNetHandler::process(CHeaderData &header, DIRECTION, AUDIO_SOURCE)
{
	if (m_id != 0x00U)
		return false;

	std::string my = header.getMyCall1();
	m_id = header.getId();

	m_linkTimer.start();

	// Change the Your callsign to CQCQCQ
	header.setCQCQCQ();

	header.setFlag1(0x00);
	header.setFlag2(0x00);
	header.setFlag3(0x00);

	// Build new repeater list
	for (std::map<std::string, CStarNetUser *>::const_iterator it = m_users.begin(); it != m_users.end(); ++it) {
		CStarNetUser* user = it->second;
		if (user) {
			// Find the user in the cache
			CUserData* userData = m_cache->findUser(user->getCallsign());

			if (userData) {
				// Find the users repeater in the repeater list, add it otherwise
				CStarNetRepeater* repeater = m_repeaters[userData->getRepeater()];
				if (repeater == NULL) {
					// Add a new repeater entry
					repeater = new CStarNetRepeater;
					repeater->m_destination = std::string("/") + userData->getRepeater().substr(0, 6) + userData->getRepeater().back();
					repeater->m_repeater    = userData->getRepeater();
					repeater->m_gateway     = userData->getGateway();
					repeater->m_address     = userData->getAddress();
					repeater->m_local       = CRepeaterHandler::findDVRepeater(userData->getRepeater());
					m_repeaters[userData->getRepeater()] = repeater;
				}

				delete userData;
				userData = NULL;
			}
		}
	}

	switch (m_callsignSwitch) {
		case SCS_GROUP_CALLSIGN:
			header.setMyCall1(m_groupCallsign);
			header.setMyCall2("SMRT");
			break;
		case SCS_USER_CALLSIGN:
			header.setMyCall1(my);
			header.setMyCall2(m_shortCallsign);
			break;
		default:
			break;
	}

	CStarNetId *tx = m_ids[m_id];
	if (tx) {
		if (!tx->isLogin())
			sendToRepeaters(header);
	} else
		sendToRepeaters(header);

	if (m_txMsgSwitch)
		sendFromText(my);

	return true;
}

bool CStarNetHandler::process(CAMBEData &data, DIRECTION, AUDIO_SOURCE)
{
	unsigned int id = data.getId();
	if (id != m_id)
		return false;

	m_linkTimer.start();

	CStarNetId *tx = m_ids[id];
	if (tx) {
		if (!tx->isLogin())
			sendToRepeaters(data);
	} else
		sendToRepeaters(data);

	if (data.isEnd()) {
		m_linkTimer.stop();
		m_id = 0x00U;

		// Clear the repeater list
		for (std::map<std::string, CStarNetRepeater *>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
			delete it->second;
		m_repeaters.clear();
	}

	return true;
}

bool CStarNetHandler::remoteLink(const std::string &reflector)
{
	if (LT_NONE != m_linkType)
		return false;

	if (LONG_CALLSIGN_LENGTH != reflector.size())
		return false;
	if (0 == reflector.compare(0, 3, "XRF"))
		m_linkType = LT_DEXTRA;
	else if (0 == reflector.compare(0, 3, "DCS"))
		m_linkType = LT_DCS;
	else
		return false;

	m_linkReflector.assign(reflector);
	return linkInt();
}

bool CStarNetHandler::linkInt()
{
	if (LT_NONE == m_linkType)
		return false;

	printf("Linking %s to %s reflector %s\n", m_repeater.c_str(), (LT_DEXTRA==m_linkType)?"DExtra":"DCS", m_linkReflector.c_str());

	// Find the repeater to link to
	CRepeaterData* data = m_cache->findRepeater(m_linkReflector);
	if (data == NULL) {
		printf("Cannot find the reflector in the cache, not linking\n");
		return false;
	}

	m_linkGateway = data->getGateway();
	bool rtv = true;
	switch (m_linkType) {
		case LT_DEXTRA:
			m_linkStatus  = LS_LINKING_DEXTRA;
			CDExtraHandler::link(this, m_repeater, m_linkReflector, data->getAddress());
			break;
		case LT_DCS:
			m_linkStatus  = LS_LINKING_DCS;
			CDCSHandler::link(this, m_repeater, m_linkReflector, data->getAddress());
			break;
		default:
			rtv = false;
			break;
	}
	delete data;
	return rtv;
}

void CStarNetHandler::clockInt(unsigned int ms)
{
	m_linkTimer.clock(ms);
	if (m_linkTimer.isRunning() && m_linkTimer.hasExpired()) {
		m_linkTimer.stop();
		m_id = 0x00U;

		// Clear the repeater list
		for (std::map<std::string, CStarNetRepeater *>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
			delete it->second;
		m_repeaters.clear();
	}
	m_announceTimer.clock(ms);
	if (m_announceTimer.hasExpired()) {
		m_irc->sendHeardWithTXMsg(m_groupCallsign, "    ", "CQCQCQ  ", m_repeater, m_gateway, 0x00U, 0x00U, 0x00U, std::string(""), m_infoText);
		if (m_offCallsign.size() && m_offCallsign.compare("        "))
			m_irc->sendHeardWithTXMsg(m_offCallsign, "    ", "CQCQCQ  ", m_repeater, m_gateway, 0x00U, 0x00U, 0x00U, std::string(""), m_infoText);
		m_announceTimer.start(60U * 60U);		// 1 hour

	}
	if (m_oldlinkStatus!=m_linkStatus && 7==m_irc->getConnectionState()) {
		updateReflectorInfo();
		m_oldlinkStatus = m_linkStatus;
	}

	// For each incoming id
	for (std::map<unsigned int, CStarNetId *>::iterator it = m_ids.begin(); it != m_ids.end(); ++it) {
		CStarNetId* tx = it->second;

		if (tx != NULL && tx->clock(ms)) {
			std::string callsign = tx->getUser()->getCallsign();

			if (tx->isEnd()) {
				CUserData* user = m_cache->findUser(callsign);
				if (user) {
					if (tx->isLogin()) {
						sendAck(*user, "Logged in");
					} else if (tx->isInfo()) {
						sendAck(*user, m_infoText);
					} else if (tx->isLogoff()) {
						sendAck(*user, "Logged off");
					}

					delete user;
					user = NULL;
				} else {
					printf("Cannot find %s in the cache", callsign.c_str());
				}

				delete tx;
				m_ids.erase(it);

				// The iterator is now invalid, so we'll find the next expiry on the next clock tick with a
				// new iterator
				break;
			} else {
				if (tx->getId() == m_id) {
					// Clear the repeater list if we're the relayed id
					for (std::map<std::string, CStarNetRepeater *>::iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it)
						delete it->second;
					m_repeaters.clear();
					m_id = 0x00U;
				}

				if (tx->isLogin()) {
					tx->reset();
					tx->setEnd();
				} else if (tx->isLogoff()) {
					m_users.erase(callsign);
					tx->reset();
					tx->setEnd();
				} else if (tx->isInfo()) {
					tx->reset();
					tx->setEnd();
				} else {
					delete tx;
					m_ids.erase(it);
					// The iterator is now invalid, so we'll find the next expiry on the next clock tick with a
					// new iterator
					break;
				}
			}
		}
	}

	// Individual user expiry, but not for the permanent entries
	for (std::map<std::string, CStarNetUser *>::iterator it = m_users.begin(); it != m_users.end(); ++it) {
		CStarNetUser* user = it->second;
		if (user && m_permanent.find(user->getCallsign()) == m_permanent.end())
			user->clock(ms);
	}

	// Don't do timeouts when relaying audio
	if (m_id != 0x00U)
		return;

	// Individual user expiry
	for (std::map<std::string, CStarNetUser *>::iterator it = m_users.begin(); it != m_users.end(); ++it) {
		CStarNetUser* user = it->second;
		if (user && user->hasExpired()) {
			printf("Removing %s from StarNet group %s, user timeout\n", user->getCallsign().c_str(), m_groupCallsign.c_str());

			logUser(LU_OFF, m_groupCallsign, user->getCallsign());	// inform QuadNet
			delete user;
			m_users.erase(it);
			// The iterator is now invalid, so we'll find the next expiry on the next clock tick with a
			// new iterator
			break;
		}
	}
}

void CStarNetHandler::updateReflectorInfo()
{
	std::string subcommand("REFLECTOR");
	std::vector<std::string> parms;
	std::string callsign(m_groupCallsign);
	CUtils::ReplaceChar(callsign, ' ', '_');
	parms.push_back(callsign);
	std::string reflector(m_linkReflector);
	if (reflector.size() < 8)
		reflector.assign("________");
	else
		CUtils::ReplaceChar(reflector, ' ', '_');
	parms.push_back(reflector);
	switch (m_linkStatus) {
		case LS_LINKING_DCS:
		case LS_LINKING_DEXTRA:
		case LS_PENDING_IRCDDB:
			parms.push_back(std::string("LINKING"));
			break;
		case LS_LINKED_DCS:
		case LS_LINKED_DEXTRA:
			parms.push_back(std::string("LINKED"));
			break;
		case LS_NONE:
			parms.push_back(std::string("UNLINKED"));
			break;
		default:
			parms.push_back(std::string("FAILED"));
			break;
	}
	parms.push_back(std::to_string(m_userTimeout));
	std::string info(m_infoText);
	info.resize(20, '_');
	CUtils::ReplaceChar(info, ' ', '_');
	parms.push_back(info);

	m_irc->sendSGSInfo(subcommand, parms);
}

void CStarNetHandler::logUser(LOGUSER lu, const std::string channel, const std::string user)
{
	std::string cmd(LU_OFF==lu ? "LOGOFF" : "LOGON");
	std::string chn(channel);
	std::string usr(user);
	CUtils::ReplaceChar(chn, ' ', '_');
	CUtils::ReplaceChar(usr, ' ', '_');
	std::vector<std::string> parms;
	parms.push_back(chn);
	parms.push_back(usr);
	m_irc->sendSGSInfo(cmd, parms);
}

void CStarNetHandler::sendToRepeaters(CHeaderData& header) const
{
	for (std::map<std::string, CStarNetRepeater *>::const_iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
		CStarNetRepeater* repeater = it->second;
		if (repeater != NULL) {
			header.setYourCall(repeater->m_destination);
			header.setDestination(repeater->m_address, G2_DV_PORT);
			header.setRepeaters(repeater->m_gateway, repeater->m_repeater);
			if (repeater->m_local != NULL)
				repeater->m_local->process(header, DIR_INCOMING, AS_G2);
			else
				m_g2Handler->writeHeader(header);
		}
	}
}

void CStarNetHandler::sendToRepeaters(CAMBEData &data) const
{
	for (std::map<std::string, CStarNetRepeater *>::const_iterator it = m_repeaters.begin(); it != m_repeaters.end(); ++it) {
		CStarNetRepeater* repeater = it->second;
		if (repeater != NULL) {
			data.setDestination(repeater->m_address, G2_DV_PORT);
			if (repeater->m_local != NULL)
				repeater->m_local->process(data, DIR_INCOMING, AS_G2);
			else
				m_g2Handler->writeAMBE(data);
		}
	}
}

void CStarNetHandler::sendFromText(const std::string &my) const
{
	std::string text;
	switch (m_callsignSwitch) {
		case SCS_GROUP_CALLSIGN:
			text = std::string("FROM %") + my;
			break;
		case SCS_USER_CALLSIGN:
			text = std::string("VIA SMARTGP ") + m_groupCallsign;
			break;
		default:
			break;
	}

	CSlowDataEncoder slowData;
	slowData.setTextData(text);

	CAMBEData data;
	data.setId(m_id);

	unsigned char buffer[DV_FRAME_LENGTH_BYTES];
	::memcpy(buffer + 0U, NULL_AMBE_DATA_BYTES, VOICE_FRAME_LENGTH_BYTES);

	for (unsigned int i = 0U; i < 21U; i++) {
		if (i == 0U) {
			// The first AMBE packet is a sync
			::memcpy(buffer + VOICE_FRAME_LENGTH_BYTES, DATA_SYNC_BYTES, DATA_FRAME_LENGTH_BYTES);
			data.setData(buffer, DV_FRAME_LENGTH_BYTES);
			data.setSeq(i);
		} else {
			// The packets containing the text data
			unsigned char slowDataBuffer[DATA_FRAME_LENGTH_BYTES];
			slowData.getTextData(slowDataBuffer);
			::memcpy(buffer + VOICE_FRAME_LENGTH_BYTES, slowDataBuffer, DATA_FRAME_LENGTH_BYTES);
			data.setData(buffer, DV_FRAME_LENGTH_BYTES);
			data.setSeq(i);
		}

		sendToRepeaters(data);
	}
}

void CStarNetHandler::sendAck(const CUserData &user, const std::string &text) const
{
	unsigned int id = CHeaderData::createId();

	CHeaderData header(m_groupCallsign, "    ", user.getUser(), user.getGateway(), user.getRepeater());
	header.setDestination(user.getAddress(), G2_DV_PORT);
	header.setId(id);
	m_g2Handler->writeHeader(header);

	CSlowDataEncoder slowData;
	slowData.setTextData(text);

	CAMBEData data;
	data.setId(id);
	data.setDestination(user.getAddress(), G2_DV_PORT);

	unsigned char buffer[DV_FRAME_MAX_LENGTH_BYTES];
	::memcpy(buffer + 0U, NULL_AMBE_DATA_BYTES, VOICE_FRAME_LENGTH_BYTES);

	for (unsigned int i = 0U; i < 20U; i++) {
		if (i == 0U) {
			// The first AMBE packet is a sync
			::memcpy(buffer + VOICE_FRAME_LENGTH_BYTES, DATA_SYNC_BYTES, DATA_FRAME_LENGTH_BYTES);
			data.setData(buffer, DV_FRAME_LENGTH_BYTES);
			data.setSeq(i);
		} else if (i == 19U) {
			// The last packet of the ack
			::memcpy(buffer + VOICE_FRAME_LENGTH_BYTES, END_PATTERN_BYTES, END_PATTERN_LENGTH_BYTES);
			data.setData(buffer, DV_FRAME_MAX_LENGTH_BYTES);
			data.setSeq(i);
			data.setEnd(true);
		} else {
			// The packets containing the text data
			unsigned char slowDataBuffer[DATA_FRAME_LENGTH_BYTES];
			slowData.getTextData(slowDataBuffer);
			::memcpy(buffer + VOICE_FRAME_LENGTH_BYTES, slowDataBuffer, DATA_FRAME_LENGTH_BYTES);
			data.setData(buffer, DV_FRAME_LENGTH_BYTES);
			data.setSeq(i);
		}

		m_g2Handler->writeAMBE(data);
	}
}

void CStarNetHandler::linkUp(DSTAR_PROTOCOL, const std::string &callsign)
{
	printf("%s link to %s established\n", (LT_DEXTRA==m_linkType)?"DExtra":"DCS", callsign.c_str());

	m_linkStatus = (LT_DEXTRA == m_linkType) ? LS_LINKED_DEXTRA : LS_LINKED_DCS;
}

bool CStarNetHandler::linkFailed(DSTAR_PROTOCOL, const std::string &callsign, bool isRecoverable)
{
	if (!isRecoverable) {
		if (m_linkStatus != LS_NONE) {
			printf("%s link to %s has failed\n", (LT_DEXTRA==m_linkType)?"DExtra":"DCS", callsign.c_str());
			m_linkStatus = LS_NONE;
		}

		return false;
	}

	if (m_linkStatus == LS_LINKING_DEXTRA || m_linkStatus == LS_LINKED_DEXTRA || m_linkStatus == LS_LINKING_DCS || m_linkStatus == LS_LINKED_DCS) {
		printf("%s link to %s has failed, relinking\n", (LT_DEXTRA==m_linkType)?"DExtra":"DCS", callsign.c_str());
		m_linkStatus = (LT_DEXTRA == m_linkType) ? LS_LINKING_DEXTRA : LS_LINKING_DCS;
		return true;
	}

	return false;
}

void CStarNetHandler::linkRefused(DSTAR_PROTOCOL, const std::string &callsign)
{
	if (m_linkStatus != LS_NONE) {
		printf("%s link to %s was refused\n", (LT_DEXTRA==m_linkType)?"DExtra":"DCS", callsign.c_str());
		m_linkStatus = LS_NONE;
	}
}

bool CStarNetHandler::singleHeader()
{
	return true;
}

DSTAR_LINKTYPE CStarNetHandler::getLinkType()
{
	return m_linkType;
}

void CStarNetHandler::setLinkType(DSTAR_LINKTYPE linkType)
{
	m_linkType = linkType;
}

void CStarNetHandler::clearReflector()
{
	m_linkReflector.clear();
}
