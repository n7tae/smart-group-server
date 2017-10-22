/*
 *   Copyright (C) 2010,2011,2012 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017 by Thomas A. Early
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

#include <future>
#include <string>
#include <queue>

#include "TCPReaderWriterClient.h"

typedef void (*ReadAPRSFrameCallback)(const std::string&);

#ifdef __UNIT_TEST__
	class APRSWriterThreadTests;
#endif

class CAPRSWriterThread {
public:
	CAPRSWriterThread(const std::string& callsign, const std::string& address, const std::string& hostname, unsigned int port);
	CAPRSWriterThread(const std::string& callsign, const std::string& address, const std::string& hostname, unsigned int port, const std::string& filter, const std::string& clientName);
	~CAPRSWriterThread();

	bool start();

	bool isConnected() const;

	void write(const char* data);

	bool Entry();

	void stop();

	void setReadAPRSCallback(ReadAPRSFrameCallback cb);

private:
	std::string            m_username;
	std::string	           m_ssid;
	CTCPReaderWriterClient m_socket;
	bool                   m_exit;
	bool                   m_connected;
	ReadAPRSFrameCallback  m_APRSReadCallback;
	std::string            m_filter;
	std::string            m_clientName;
	std::future<bool>      m_future;
	std::queue<char *>     m_queue;

	bool connect();
	unsigned int getAPRSPassword(std::string username) const;

#ifdef __UNIT_TEST__
	friend APRSWriterThreadTests;
#endif
};

