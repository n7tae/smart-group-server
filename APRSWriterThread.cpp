/*
 *   Copyright (C) 2010-2014 by Jonathan Naylor G4KLX
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

#include <cassert>
#include <unistd.h>
#include <cstring>

#include "APRSWriterThread.h"
#include "DStarDefines.h"
#include "Utils.h"
#include "Defs.h"

// #define	DUMP_TX

const unsigned int APRS_TIMEOUT = 10U;

CAPRSWriterThread::CAPRSWriterThread(const std::string& callsign, const std::string& address, const std::string& hostname, unsigned int port) :
m_username(callsign),
m_ssid(callsign),
m_socket(hostname, port, address),
m_exit(false),
m_connected(false),
m_APRSReadCallback(NULL),
m_filter(""),
m_clientName("ircDDBGateway")
{
	assert(callsign.size());
	assert(hostname.size());
	assert(port > 0U);

	m_username.at(LONG_CALLSIGN_LENGTH - 1U) = ' ';
	CUtils::Trim(m_username);
	CUtils::ToUpper(m_username);

	m_ssid = m_ssid.substr(LONG_CALLSIGN_LENGTH - 1U, 1);
}

CAPRSWriterThread::CAPRSWriterThread(const std::string& callsign, const std::string& address, const std::string& hostname, unsigned int port, const std::string& filter, const std::string& clientName) :
m_username(callsign),
m_ssid(callsign),
m_socket(hostname, port, address),
m_exit(false),
m_connected(false),
m_APRSReadCallback(NULL),
m_filter(filter),
m_clientName(clientName)
{
	assert(callsign.size());
	assert(hostname.size());
	assert(port > 0U);

	m_username.at(LONG_CALLSIGN_LENGTH - 1U) = ' ';
	CUtils::Trim(m_username);
	CUtils::ToUpper(m_username);

	m_ssid = m_ssid.substr(LONG_CALLSIGN_LENGTH - 1U, 1);
}

CAPRSWriterThread::~CAPRSWriterThread()
{
	m_username.clear();
}

bool CAPRSWriterThread::start()
{
	m_future = std::async(std::launch::async, &CAPRSWriterThread::Entry, this);

	return true;
}

bool CAPRSWriterThread::Entry()
{
	CUtils::lprint("Starting the APRS Writer thread");

	m_connected = connect();

	try {
		while (!m_exit) {
			if (!m_connected) {
				m_connected = connect();

				if (!m_connected){
					CUtils::lprint("Reconnect attempt to the APRS server has failed");
					sleep(10);		// 10 secs
				}
			}

			if (m_connected) {
				if(m_queue.size()){
					char* p = m_queue.front();

					std::string text(p);
					CUtils::lprint("APRS ==> %s", text.c_str());

					::strcat(p, "\r\n");

					bool ret = m_socket.write((unsigned char*)p, ::strlen(p));
					if (!ret) {
						m_connected = false;
						m_socket.close();
						CUtils::lprint("Connection to the APRS thread has failed");
					}

					delete[] p;
					m_queue.pop();
				}
				{
					std::string line;
					int length = m_socket.readLine(line, APRS_TIMEOUT);

					/*if (length == 0)
						CUtils::lprint("No response from the APRS server after %u seconds", APRS_TIMEOUT);*/

					if (length < 0) {
						m_connected = false;
						m_socket.close();
						CUtils::lprint("Error when reading from the APRS server");
					}

					if(length > 0 && line[0] != '#'//check if we have something and if that something is an APRS frame
					    && m_APRSReadCallback != NULL)//do we have someone wanting an APRS Frame?
					{	
						//CUtils::lprint("Received APRS Frame : ") + line);
						m_APRSReadCallback(std::string(line));
					}
				}

			}
		}

		if (m_connected)
			m_socket.close();

		while (m_queue.size()) {
			delete[] m_queue.front();
			m_queue.pop();
		}
	}
	catch (std::exception& e) {
		std::string message(e.what());
		CUtils::lprint("Exception raised in the APRS Writer thread - \"%s\"", message.c_str());
	}
	catch (...) {
		CUtils::lprint("Unknown exception raised in the APRS Writer thread");
	}

	CUtils::lprint("Stopping the APRS Writer thread");

	return NULL;
}

void CAPRSWriterThread::setReadAPRSCallback(ReadAPRSFrameCallback cb)
{
	m_APRSReadCallback = cb;
}

void CAPRSWriterThread::write(const char* data)
{
	assert(data != NULL);

	if (!m_connected)
		return;

	unsigned int len = ::strlen(data);

	char* p = new char[len + 5U];
	::strcpy(p, data);

	m_queue.push(p);
}

bool CAPRSWriterThread::isConnected() const
{
	return m_connected;
}

void CAPRSWriterThread::stop()
{
	m_exit = true;

	m_future.get();
}

bool CAPRSWriterThread::connect()
{
	unsigned int password = getAPRSPassword(m_username);

	bool ret = m_socket.open();
	if (!ret)
		return false;

	//wait for lgin banner
	int length;
	std::string serverResponse("");
	length = m_socket.readLine(serverResponse, APRS_TIMEOUT);
	if (length == 0) {
		CUtils::lprint("No reply from the APRS server after %u seconds", APRS_TIMEOUT);
		m_socket.close();
		return false;
	}
	CUtils::lprint("Received login banner : %s", serverResponse.c_str());

	std::string filter;
	if (m_filter.size() > 0)
		filter = std::string(" filter ") + m_filter;
	char cstr[128];
	snprintf(cstr, 128, "user %s-%s pass %u vers %s%s\n", m_username.c_str(), m_ssid.c_str(), password, m_clientName.size() ? m_clientName.c_str() : "ircDDBGateway", filter.c_str());
	ret = m_socket.writeLine(std::string(cstr));
	if (!ret) {
		m_socket.close();
		return false;
	}
	

	length = m_socket.readLine(serverResponse, APRS_TIMEOUT);
	if (length == 0) {
		CUtils::lprint("No reply from the APRS server after %u seconds", APRS_TIMEOUT);
		m_socket.close();
		return false;
	}
	if (length < 0) {
		CUtils::lprint("Error when reading from the APRS server");
		m_socket.close();
		return false;
	}

	CUtils::lprint("Response from APRS server: %s", serverResponse.c_str());

	CUtils::lprint("Connected to the APRS server");

	return true;
}

unsigned int CAPRSWriterThread::getAPRSPassword(std::string callsign) const
{
	unsigned int len = callsign.size();

	uint16_t hash = 0x73E2U;

	for (unsigned int i = 0U; i < len; i += 2U) {
		hash ^= (char)callsign.at(i) << 8;
		if(i + 1 < len)
			hash ^= (char)callsign.at(i + 1);
	}

	return hash & 0x7FFFU;
}
