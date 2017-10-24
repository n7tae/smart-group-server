/*
 *   Copyright (C) 2011 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017 by Thomas A. Early N7TAE
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

#include "RemoteStarNetGroup.h"

CRemoteStarNetGroup::CRemoteStarNetGroup(const std::string& callsign, const std::string& logoff, uint32_t timer, uint32_t timeout) :
m_callsign(callsign),
m_logoff(logoff),
m_timer(timer),
m_timeout(timeout),
m_users()
{
	if (logoff.compare("        "))
		logoff.empty();
}

CRemoteStarNetGroup::~CRemoteStarNetGroup()
{
	while (m_users.size()) {
		delete m_users.back();
		m_users.pop_back();
	}
}

void CRemoteStarNetGroup::addUser(const std::string& callsign, uint32_t timer, uint32_t timeout)
{
	CRemoteStarNetUser *user = new CRemoteStarNetUser(callsign, timer, timeout);

	m_users.push_back(user);
}

std::string CRemoteStarNetGroup::getCallsign() const
{
	return m_callsign;
}

std::string CRemoteStarNetGroup::getLogoff() const
{
	return m_logoff;
}

uint32_t CRemoteStarNetGroup::getTimer() const
{
	return m_timer;
}

uint32_t CRemoteStarNetGroup::getTimeout() const
{
	return m_timeout;
}

uint32_t CRemoteStarNetGroup::getUserCount() const
{
	return m_users.size();
}

CRemoteStarNetUser *CRemoteStarNetGroup::getUser(uint32_t n) const
{
	return m_users[n];
}
