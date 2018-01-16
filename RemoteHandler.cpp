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
				RECONNECT reconnect;
				m_handler.readLink(callsign, reconnect, reflector);
				if (0 == reflector.size())
					printf("Remote control user has linked \"%s\" to \"None\" with reconnect %s\n", callsign.c_str(), ReconnectText(reconnect));
				else
					printf("Remote control user has linked \"%s\" to \"%s\" with reconnect %s\n", callsign.c_str(), reflector.c_str(), ReconnectText(reconnect));
				link(callsign, reconnect, reflector, true);
			}
			break;
		case RPHT_UNLINK: {
				std::string callsign, reflector;
				PROTOCOL protocol;
				m_handler.readUnlink(callsign, protocol, reflector);
				printf("Remote control user has unlinked \"%s\" from \"%s\"\n", callsign.c_str(), reflector.c_str());
				unlink(callsign, protocol, reflector);
			}
			break;
		case RPHT_LINKSCR: {
				std::string callsign, reflector;
				RECONNECT reconnect;
				m_handler.readLinkScr(callsign, reconnect, reflector);
				if (0 == reflector.size())
					printf("Remote control user has linked \"%s\" to \"None\" with reconnect %s from localhost\n", callsign.c_str(), ReconnectText(reconnect));
				else
					printf("Remote control user has linked \"%s\" to \"%s\" with reconnect %s from localhost\n", callsign.c_str(), reflector.c_str(), ReconnectText(reconnect));
				link(callsign, reconnect, reflector, false);
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
//		CDPlusHandler::getInfo(repeater, *data);
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

void CRemoteHandler::link(const std::string &callsign, RECONNECT reconnect, const std::string &reflector, bool respond)
{
//	CStarNetHandler *smartGroup = CStarNetHandler::findStarNet(callsign);
//	if (0 == reflector.compare(0, 3, "DCS") || 0 == reflector.compare(0, 3, "XRF")) {
//		if (NULL == smartGroup) {
//			m_handler.sendNAK("Invalid smartgroup callsign");
//			return;
//		}
//		DSTAR_LINKTYPE linkType = smartGroup->getLinkType();
//		if ((0==callsign.compare(0, 3, "XRF") && linkType!=LT_DEXTRA) || (0==callsign.compare(0, 3, "DCS") && linkType!=LT_DCS)) {
//			std::string response("Can't link ");
//			response += callsign + " of type ";
//			switch (linkType) {
//				case LT_DEXTRA:
//					response.append("DExtra");
//					break;
//				case LT_DCS:
//					response.append("DCS");
//					break;
//				case LT_NONE:
//					response.append("Unlinked");
//					break;
//			}
//			response += std::string(" to ") + reflector;
//			m_handler.sendNAK(response);
//			return;
//		}
//	}
	CRepeaterHandler *repeater = CRepeaterHandler::findDVRepeater(callsign);
	if (repeater == NULL) {
		m_handler.sendNAK("Invalid repeater callsign");
		return;
	}

	repeater->link(reconnect, reflector);

	if (respond)
	    m_handler.sendACK();
//	if (smartGroup)
//		smartGroup->updateReflectorInfo();	// tell QuadNet
}

void CRemoteHandler::unlink(const std::string &callsign, PROTOCOL protocol, const std::string &reflector)
{
	CRepeaterHandler *repeater = CRepeaterHandler::findDVRepeater(callsign);
	if (repeater == NULL) {
		m_handler.sendNAK("Invalid repeater callsign");
		return;
	}

	repeater->unlink(protocol, reflector);

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

const char *CRemoteHandler::ReconnectText(RECONNECT value)
{
	const char *ret;
	switch (value) {
		case RECONNECT_NEVER:
			ret = "never";
			break;
		case RECONNECT_FIXED:
			ret = "fixed";
			break;
		case RECONNECT_5MINS:
			ret = "5 mins";
			break;
		case RECONNECT_10MINS:
			ret = "10 mins";
			break;
		case RECONNECT_15MINS:
			ret = "15 mins";
			break;
		case RECONNECT_20MINS:
			ret = "20 mins";
			break;
		case RECONNECT_25MINS:
			ret = "25 mins";
			break;
		case RECONNECT_30MINS:
			ret = "30 mins";
			break;
		case RECONNECT_60MINS:
			ret = "60 mins";
			break;
		case RECONNECT_90MINS:
			ret = "90 mins";
			break;
		case RECONNECT_120MINS:
			ret = "120 mins";
			break;
		case RECONNECT_180MINS:
			ret = "180 mins";
			break;
	}
	return ret;
}
