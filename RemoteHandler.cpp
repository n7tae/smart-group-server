/*
 *   Copyright (C) 2011,2012,2013 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2018 by Thomas A. Early N7TAE
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
#include <cstdlib>
#include <list>

#include "RepeaterHandler.h"
#include "StarNetHandler.h"
#include "RemoteHandler.h"
#include "DExtraHandler.h"
//#include "DPlusHandler.h"
#include "DStarDefines.h"
#include "DCSHandler.h"
#include "Utils.h"

CRemoteHandler::CRemoteHandler(const std::string &password, unsigned int port, const std::string &address) :
m_password(password),
m_handler(port, address),
m_random(0U)
{
	assert(port > 0U);
	assert(password.size());
}

CRemoteHandler::~CRemoteHandler()
{
}

bool CRemoteHandler::open()
{
	return m_handler.open();
}

void CRemoteHandler::process()
{
	RPH_TYPE type = m_handler.readType();
	switch (type) {
		case RPHT_LOGOUT:
			m_handler.setLoggedIn(false);
			printf("Remote control user has logged out\n");
			break;
		case RPHT_LOGIN:
			m_random = (uint32_t)rand();
			m_handler.sendRandom(m_random);
			break;
		case RPHT_HASH: {
				bool valid = m_handler.readHash(m_password, m_random);
				if (valid) {
					printf("Remote control user has logged in\n");
					m_handler.setLoggedIn(true);
					m_handler.sendACK();
				} else {
					printf("Remote control user has failed login authentication\n");
					m_handler.setLoggedIn(false);
					m_handler.sendNAK("Invalid password");
				}
			}
			break;
		case RPHT_CALLSIGNS:
			sendCallsigns();
			break;
		case RPHT_REPEATER: {
				std::string callsign = m_handler.readRepeater();
				sendRepeater(callsign);
			}
			break;
		case RPHT_STARNET: {
				std::string callsign = m_handler.readStarNetGroup();
				sendStarNetGroup(callsign);
			}
			break;
		case RPHT_LINK: {
				std::string callsign, reflector;
				m_handler.readLink(callsign, reflector);
				printf("Remote control user has linked \"%s\" to \"%s\"\n", callsign.c_str(), reflector.c_str());
				link(callsign, reflector);
			}
			break;
		case RPHT_UNLINK: {
				std::string callsign;
				m_handler.readUnlink(callsign);
				printf("Remote control user has unlinked \"%s\"\n", callsign.c_str());
				unlink(callsign);
			}
			break;
		case RPHT_LOGOFF: {
				std::string callsign, user;
				m_handler.readLogoff(callsign, user);
				printf("Remote control user has logged off \"%s\" from \"%s\"\n", user.c_str(), callsign.c_str());
				logoff(callsign, user);
			}
			break;
		default:
			break;
	}
}

void CRemoteHandler::close()
{
	m_handler.close();
}

void CRemoteHandler::sendCallsigns()
{
	std::list<std::string> repeaters = CRepeaterHandler::listDVRepeaters();
	std::list<std::string> starNets  = CStarNetHandler::listStarNets();

	m_handler.sendCallsigns(repeaters, starNets);
}

void CRemoteHandler::sendRepeater(const std::string &callsign)
{
	CRepeaterHandler *repeater = CRepeaterHandler::findDVRepeater(callsign);
	if (repeater == NULL) {
		m_handler.sendNAK("Invalid repeater callsign");
		return;
	}

	CRemoteRepeaterData *data = repeater->getInfo();
	if (data != NULL) {
		CDExtraHandler::getInfo(repeater, *data);
		CDCSHandler::getInfo(repeater, *data);
		CCCSHandler::getInfo(repeater, *data);

		m_handler.sendRepeater(*data);
	}

	delete data;
}

void CRemoteHandler::sendStarNetGroup(const std::string &callsign)
{
	CStarNetHandler *starNet = CStarNetHandler::findStarNet(callsign);
	if (starNet == NULL) {
		m_handler.sendNAK("Invalid STARnet Group callsign");
		return;
	}

	CRemoteStarNetGroup *data = starNet->getInfo();
	if (data != NULL)
		m_handler.sendStarNetGroup(*data);

	delete data;
}

void CRemoteHandler::link(const std::string &callsign, const std::string &reflector)
{
	CStarNetHandler *smartGroup = CStarNetHandler::findStarNet(callsign);
	if (NULL == smartGroup) {
		m_handler.sendNAK(std::string("Invalid Smart Group subscribe call ") + callsign);
		return;
	}

	if (smartGroup->remoteLink(reflector))
		m_handler.sendACK();
	else
		m_handler.sendNAK("link failed");
}

void CRemoteHandler::unlink(const std::string &callsign)
{
	CStarNetHandler *smartGroup = CStarNetHandler::findStarNet(callsign);
	if (NULL == smartGroup) {
		m_handler.sendNAK(std::string("Invalid Smart Group subscribe call ") + callsign);
		return;
	}

	CRemoteStarNetGroup *data = smartGroup->getInfo();
	if (data) {
		switch (smartGroup->getLinkType()) {
			case LT_DEXTRA:
				CDExtraHandler::unlink(smartGroup, data->getReflector(), true);
				break;
			case LT_DCS:
				CDCSHandler::unlink(smartGroup, data->getReflector(), true);
				break;
			default:
				delete data;
				m_handler.sendNAK("alread unlinked");
				return;
		}
		delete data;
	} else {
		m_handler.sendNAK("could not get Smart Group info");
		return;
	}
	smartGroup->setLinkType(LT_NONE);
    m_handler.sendACK();
}

void CRemoteHandler::logoff(const std::string &callsign, const std::string &user)
{
	CStarNetHandler *starNet = CStarNetHandler::findStarNet(callsign);
	if (starNet == NULL) {
		m_handler.sendNAK("Invalid STARnet group callsign");
		return;
	}

	bool res = starNet->logoff(user);
	if (!res)
		m_handler.sendNAK("Invalid STARnet user callsign");
	else
		m_handler.sendACK();
}
