/*
 *   Copyright (C) 2011,2012,2013 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2018,2020 by Thomas A. Early N7TAE
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
#include <vector>
#include <sstream>
#include <iterator>

#include "RemoteHandler.h"
#include "DExtraHandler.h"
#include "DStarDefines.h"
#include "DCSHandler.h"
#include "Utils.h"

bool CRemoteHandler::open(const std::string &password, const unsigned short port, const bool isIPV6)
{
	return m_tlsserver.OpenSocket(password, isIPV6 ? "::" : "0.0.0.0", port);
}

bool CRemoteHandler::process()
{
	std::string command;
	if (m_tlsserver.GetCommand(command))
		return false;	// nothing to do...
	// parse the command into words
	std::stringstream ss(command);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	std::vector<std::string> cwords(begin, end);

	if (0 == cwords.size())
		return false;

	if (0 == cwords[0].compare("halt"))
		return true;

	ReplaceChar(cwords[0], '_', ' ');
	cwords[0].resize(8, ' ');
	CGroupHandler *group = CGroupHandler::findGroup(cwords[0]);
	if (NULL == group) {
		char emsg[128];
		snprintf(emsg, 128, "Smart Group [%s] not found", cwords[0].c_str());
		m_tlsserver.Write(emsg);
		m_tlsserver.CloseClient();
	} else {
		// we have a valid smart group in cwords[0]
		if      (cwords.size() > 1 && 0 == cwords[1].compare("list")) {
			sendGroup(group);
		}
		else if (cwords.size() > 2 && 0 == cwords[1].compare("link")) {
			ReplaceChar(cwords[2], '_', ' ');
			cwords[2].resize(8, ' ');
			printf("Remote control user has linked \"%s\" to \"%s\"\n", cwords[0].c_str(), cwords[2].c_str());
			link(group, cwords[2]);
		}
		else if (cwords.size() > 1 && 0 == cwords[1].compare("unlink")) {
			printf("Remote control user has unlinked \"%s\"\n", cwords[0].c_str());
			unlink(group);
		}
		else if (cwords.size() > 2 && 0 == cwords[1].compare("drop")) {
			ReplaceChar(cwords[2], '_', ' ');
			cwords[2].resize(8, ' ');
			printf("Remote control user has logged off \"%s\" from \"%s\"\n", cwords[2].c_str(), cwords[0].c_str());
			logoff(group, cwords[2]);
		}
		else {
			printf("The command \"%s\" is bad\n", command.c_str());
		}
		m_tlsserver.CloseClient();
	}
	return false;
}


void CRemoteHandler::sendGroup(CGroupHandler *group)
{
	CRemoteGroup *data = group->getInfo();
	if (data != NULL) {
		char msg[128];
		snprintf(msg, 128, "Subscribe    = %s", data->getCallsign().c_str());
		m_tlsserver.Write(msg);
		snprintf(msg, 128, "Unsubscribe  = %s", data->getLogoff().c_str());
		m_tlsserver.Write(msg);
		snprintf(msg, 128, "Module       = %s", data->getRepeater().c_str());
		m_tlsserver.Write(msg);
		snprintf(msg, 128, "Description  = %s", data->getInfoText().c_str());
		m_tlsserver.Write(msg);
		snprintf(msg, 128, "Reflector    = %s", data->getReflector().c_str());
		m_tlsserver.Write(msg);
		switch (data->getLinkStatus()) {
			case LS_LINKING_DCS:
			case LS_LINKING_DEXTRA:
				m_tlsserver.Write("Link Status  = Linking");
				break;
			case LS_LINKED_DCS:
			case LS_LINKED_DEXTRA:
				m_tlsserver.Write("Link Status  = Linked");
				break;
			default:
				m_tlsserver.Write("Link Status  = Unlinked");
				break;
		}
		snprintf(msg, 128, "User Timeout = %u min", data->getUserTimeout());
		m_tlsserver.Write(msg);
		for (uint32_t i=0; i<data->getUserCount(); i++) {
			CRemoteUser *user = data->getUser(i);
			snprintf(msg, 128, "    User = %s, timer = %u min, timeout = %u min", user->getCallsign().c_str(), user->getTimer()/60U, user->getTimeout()/60U);
			m_tlsserver.Write(msg);
		}
	}
	delete data;
}

void CRemoteHandler::link(CGroupHandler *group, const std::string &reflector)
{
	char msg[128];
	CRemoteGroup *data = group->getInfo();
	if (group->remoteLink(reflector))
		snprintf(msg, 128, "Smart Group %s linked to %s", data->getCallsign().c_str(), reflector.c_str());
	else
		snprintf(msg, 128, "Failed to link Smart Group %s to %s", data->getCallsign().c_str(), reflector.c_str());
	m_tlsserver.Write(msg);
	delete data;
}

void CRemoteHandler::unlink(CGroupHandler *group)
{
	char msg[128];
	CRemoteGroup *data = group->getInfo();
	switch (group->getLinkType()) {
		case LT_DEXTRA:
			CDExtraHandler::unlink(group, data->getReflector(), false);
			snprintf(msg, 128, "Smart Group %s unlinked from %s", data->getCallsign().c_str(), data->getReflector().c_str());
			break;
		case LT_DCS:
			CDCSHandler::unlink(group, data->getReflector(), false);
			snprintf(msg, 128, "Smart Group %s unlinked from %s", data->getCallsign().c_str(), data->getReflector().c_str());
			break;
		default:
			snprintf(msg, 128, "Smart Group %s is already unlinked", data->getCallsign().c_str());
			m_tlsserver.Write(msg);
			delete data;
			return;
	}
	delete data;
	group->setLinkType(LT_NONE);
	group->clearReflector();
    m_tlsserver.Write(msg);
}

void CRemoteHandler::logoff(CGroupHandler *group, const std::string &user)
{
	char msg[128];
	CRemoteGroup *data = group->getInfo();
	if (group->logoff(user))
		snprintf(msg, 128, "Logging off %s from Smart Group %s", user.c_str(), data->getCallsign().c_str());
	else
		snprintf(msg, 128, "Could not logoff %s from Smart Group %s", user.c_str(), data->getCallsign().c_str());
	m_tlsserver.Write(msg);
	delete data;
}
