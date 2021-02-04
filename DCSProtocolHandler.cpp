/*
 *   Copyright (C) 2012,2013 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017,2021 by Thomas A. Early N7TAE
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

#include "DCSProtocolHandler.h"
#include "Utils.h"

// #define	DUMP_TX

const unsigned int BUFFER_LENGTH = 2000U;

CDCSProtocolHandler::CDCSProtocolHandler(int family) :
m_family(family),
m_socket(family, 0),
m_type(DC_NONE),
m_buffer(NULL),
m_length(0U),
m_yourAddress(),
m_yourPort(0U)
{
	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CDCSProtocolHandler::~CDCSProtocolHandler()
{
	delete[] m_buffer;
}

bool CDCSProtocolHandler::open()
{
	return m_socket.Open();
}

unsigned short CDCSProtocolHandler::getPort() const
{
	return m_socket.getPort();
}

bool CDCSProtocolHandler::writeData(const CAMBEData& data)
{
	unsigned char buffer[100U];
	unsigned int length = data.getDCSData(buffer, 100U);

#if defined(DUMP_TX)
	dump("Sending Data", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, data.getYourPort(), data.getYourAddress().c_str());
	return m_socket.Write(buffer, length, addr);
}

bool CDCSProtocolHandler::writePoll(const CPollData& poll)
{
	unsigned char buffer[25U];
	unsigned int length = poll.getDCSData(buffer, 25U);

#if defined(DUMP_TX)
	dump("Sending Poll", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, poll.getYourPort(), poll.getYourAddress().c_str());
	return m_socket.Write(buffer, length, addr);
}

bool CDCSProtocolHandler::writeConnect(const CConnectData& connect)
{
	unsigned char buffer[520U];
	unsigned int length = connect.getDCSData(buffer, 520U);

#if defined(DUMP_TX)
	dump("Sending Connect", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, connect.getYourPort(), connect.getYourAddress().c_str());
	return m_socket.Write(buffer, length, addr);
}

DCS_TYPE CDCSProtocolHandler::read()
{
	bool res = true;

	// Loop until we have no more data from the socket or we have data for the higher layers
	while (res)
		res = readPackets();

	return m_type;
}

bool CDCSProtocolHandler::readPackets()
{
	m_type = DC_NONE;

	// No more data?
	CSockAddress addr;
	int length = m_socket.Read(m_buffer, BUFFER_LENGTH, addr);
	if (length <= 0)
		return false;
	m_yourAddress = addr.GetAddress();
	m_yourPort = addr.GetPort();
	if (length <= 0)
		return false;

	m_length = length;

	if (m_buffer[0] == '0' && m_buffer[1] == '0' && m_buffer[2] == '0' && m_buffer[3] == '1') {
		if (m_length == 100U) {
			m_type = DC_DATA;
			return false;
		}
	} else if (m_buffer[0] == 'E' && m_buffer[1] == 'E' && m_buffer[2] == 'E' && m_buffer[3] == 'E') {
		//dump("Status data", m_buffer, m_length);
		return true;
	} else {
		switch (m_length) {
			case 17U:
			case 22U:
				m_type = DC_POLL;
				return false;
			case 14U:
			case 19U:
			case 519U:
				m_type = DC_CONNECT;
				return false;
			case 35U:
				//dump("Status data", m_buffer, m_length);
				return true;
			default:
				break;
		}
	}

	// An unknown type
	//dump("Unknown packet type from DCS", m_buffer, m_length);
	return true;
}

CAMBEData* CDCSProtocolHandler::readData()
{
	if (m_type != DC_DATA)
		return NULL;

	CAMBEData* data = new CAMBEData;

	bool res = data->setDCSData(m_buffer, m_length, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete data;
		return NULL;
	}

	return data;
}

CPollData* CDCSProtocolHandler::readPoll()
{
	if (m_type != DC_POLL)
		return NULL;

	CPollData* poll = new CPollData;

	bool res = poll->setDCSData(m_buffer, m_length, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete poll;
		return NULL;
	}

	return poll;
}

CConnectData* CDCSProtocolHandler::readConnect()
{
	if (m_type != DC_CONNECT)
		return NULL;

	CConnectData* connect = new CConnectData;

	bool res = connect->setDCSData(m_buffer, m_length, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete connect;
		return NULL;
	}

	return connect;
}

void CDCSProtocolHandler::close()
{
	m_socket.Close();
}
