/*
 *   Copyright (C) 2010-2013 by Jonathan Naylor G4KLX
 *   Copyright (C) 2017,2020 by Thomas A. Early N7TAE
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

#include <cstring>

#include "DExtraProtocolHandler.h"
#include "Utils.h"

// #define	DUMP_TX

const unsigned int BUFFER_LENGTH = 1000U;

CDExtraProtocolHandler::CDExtraProtocolHandler(int family) :
m_family(family),
m_socket(family, 0),
m_type(DE_NONE),
m_buffer(NULL),
m_length(0U),
m_yourAddress(),
m_yourPort(0U)
{
	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CDExtraProtocolHandler::~CDExtraProtocolHandler()
{
	delete[] m_buffer;
}

bool CDExtraProtocolHandler::open()
{
	return m_socket.Open();
}

unsigned short CDExtraProtocolHandler::getPort() const
{
	return m_socket.getPort();
}

bool CDExtraProtocolHandler::writeHeader(const CHeaderData& header)
{
	unsigned char buffer[60U];
	unsigned int length = header.getDExtraData(buffer, 60U, true);

#if defined(DUMP_TX)
	dump("Sending Header", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, header.getYourPort(), header.getYourAddress().c_str());
	for (unsigned int i = 0U; i < 5U; i++) {
		bool res = m_socket.Write(buffer, length, addr);
		if (!res)
			return false;
	}

	return true;
}

bool CDExtraProtocolHandler::writeAMBE(const CAMBEData& data)
{
	unsigned char buffer[40U];
	unsigned int length = data.getDExtraData(buffer, 40U);

#if defined(DUMP_TX)
	dump("Sending Data", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, data.getYourPort(), data.getYourAddress().c_str());
	return m_socket.Write(buffer, length, addr);
}

bool CDExtraProtocolHandler::writePoll(const CPollData& poll)
{
	unsigned char buffer[20U];
	unsigned int length = poll.getDExtraData(buffer, 20U);

#if defined(DUMP_TX)
	dump("Sending Poll", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, poll.getYourPort(), poll.getYourAddress().c_str());
	return m_socket.Write(buffer, length, addr);
}

bool CDExtraProtocolHandler::writeConnect(const CConnectData& connect)
{
	unsigned char buffer[20U];
	unsigned int length = connect.getDExtraData(buffer, 20U);

#if defined(DUMP_TX)
	dump("Sending Connect", buffer, length);
#endif
	CSockAddress addr;
	addr.Initialize(m_family, connect.getYourPort(), connect.getYourAddress().c_str());
	for (unsigned int i = 0U; i < 2U; i++) {
		bool res = m_socket.Write(buffer, length, addr);
		if (!res)
			return false;
	}

	return true;
}

DEXTRA_TYPE CDExtraProtocolHandler::read()
{
	bool res = true;

	// Loop until we have no more data from the socket or we have data for the higher layers
	while (res)
		res = readPackets();

	return m_type;
}

bool CDExtraProtocolHandler::readPackets()
{
	m_type = DE_NONE;

	// No more data?
	CSockAddress addr;
	int length = m_socket.Read(m_buffer, BUFFER_LENGTH, addr);
	if (length <= 0)
		return false;
    m_yourAddress = addr.GetAddress();
    m_yourPort = addr.GetPort();

	m_length = length;

	if (memcmp(m_buffer, "DSVT", 4)) {
		switch (m_length) {
			case 9U:
				m_type = DE_POLL;
				return false;
			case 11U:
			case 14U:
				m_type = DE_CONNECT;
				return false;
			default:
				return true;
		}
	} else {
		// Header or data packet type?
		if (56U==m_length && 0x10U==m_buffer[4] && 0x80U==m_buffer[14])
			m_type = DE_HEADER;
		else if (27U==m_length && 0x20U==m_buffer[4])
			m_type = DE_AMBE;
		else
			return true;

		return false;
	}
}

CHeaderData* CDExtraProtocolHandler::newHeader()
{
	if (m_type != DE_HEADER)
		return NULL;

	CHeaderData* header = new CHeaderData;

	// DExtra checksums are unreliable
	bool res = header->setDExtraData(m_buffer, m_length, false, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete header;
		return NULL;
	}

	return header;
}

CAMBEData* CDExtraProtocolHandler::newAMBE()
{
	if (m_type != DE_AMBE)
		return NULL;

	CAMBEData* data = new CAMBEData;

	bool res = data->setDExtraData(m_buffer, m_length, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete data;
		return NULL;
	}

	return data;
}

CPollData* CDExtraProtocolHandler::newPoll()
{
	if (m_type != DE_POLL)
		return NULL;

	CPollData* poll = new CPollData;

	bool res = poll->setDExtraData(m_buffer, m_length, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete poll;
		return NULL;
	}

	return poll;
}

CConnectData* CDExtraProtocolHandler::newConnect()
{
	if (m_type != DE_CONNECT)
		return NULL;

	CConnectData* connect = new CConnectData;

	bool res = connect->setDExtraData(m_buffer, m_length, m_yourAddress, m_yourPort, m_socket.getPort());
	if (!res) {
		delete connect;
		return NULL;
	}

	return connect;
}

void CDExtraProtocolHandler::close()
{
	m_socket.Close();
}
