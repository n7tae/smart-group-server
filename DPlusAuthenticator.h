/*
 *   Copyright (C) 2010-2013 by Jonathan Naylor G4KLX
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

#pragma once

#include <string>
#include <future>
#include <netinet/in.h>

#include "TCPReaderWriterClient.h"
#include "CacheManager.h"
#include "Timer.h"

class CDPlusAuthenticator {
public:
	CDPlusAuthenticator(const std::string& loginCallsign, const std::string& gatewayCallsign, const std::string& address, CCacheManager* cache);
	~CDPlusAuthenticator();

	void  start();

	bool Entry();

	void stop();

private:
	std::string    m_loginCallsign;
	std::string    m_gatewayCallsign;
	std::string    m_address;
	CCacheManager* m_cache;
	CTimer         m_timer;
	CTimer         m_pollTimer;
	bool           m_killed;
	std::future<bool> m_future;

	bool poll(const std::string& callsign, const std::string& hostname, unsigned int port, unsigned char id);
	bool authenticate(const std::string& callsign, const std::string& hostname, unsigned int port, unsigned char id, bool writeToCache);
	bool read(CTCPReaderWriterClient& socket, unsigned char* buffer, unsigned int len) const;
};
