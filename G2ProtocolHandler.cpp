/*
 *   Copyright (C) 2010,2011,2013 by Jonathan Naylor G4KLX
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

#include <string>
#include <cstring>
#include "G2ProtocolHandler.h"
#include "Utils.h"

// #define	DUMP_TX

const unsigned int BUFFER_LENGTH = 255U;

CG2ProtocolHandler::CG2ProtocolHandler(int family, unsigned short port) :
m_socket(family, port),
m_type(GT_NONE),
m_buffer(NULL),
m_length(0U)
{
	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CG2ProtocolHandler::~CG2ProtocolHandler()
{
	delete[] m_buffer;
	portmap.clear();
}

bool CG2ProtocolHandler::open()
{
	return m_socket.Open();
}

bool CG2ProtocolHandler::writeHeader(const CHeaderData& header)
{
	unsigned char buffer[60U];
	unsigned int length = header.getG2Data(buffer, 60U, true);

#if defined(DUMP_TX)
	CUtils::dump("Sending Header", buffer, length);
#endif

	CSockAddress saddr;
	std::string addr = header.getYourAddress();
	auto it = portmap.find(addr);
	if (addr.npos == addr.find(':')) {
		if (portmap.end() == it)
			saddr.Initialize(AF_INET, G2_DV_PORT, addr.c_str());
		else
			saddr.Initialize(AF_INET, it->second, addr.c_str());
	} else {
		if (portmap.end() == it)
			saddr.Initialize(AF_INET6, G2_IPV6_PORT, addr.c_str());
		else
			saddr.Initialize(AF_INET6, it->second, addr.c_str());
	} 

	for (unsigned int i = 0U; i < 5U; i++) {
		bool res = m_socket.Write(buffer, length, saddr);
		if (!res)
			return false;
	}

	return true;
}

bool CG2ProtocolHandler::writeAMBE(const CAMBEData& data)
{
	unsigned char buffer[40U];
	unsigned int length = data.getG2Data(buffer, 40U);

#if defined(DUMP_TX)
	CUtils::dump("Sending Data", buffer, length);
#endif

	CSockAddress saddr;
	std::string addr = data.getYourAddress();
	auto it = portmap.find(addr);
	if (addr.npos == addr.find(':')) {
		if (portmap.end() == it)
			saddr.Initialize(AF_INET, G2_DV_PORT, addr.c_str());
		else
			saddr.Initialize(AF_INET, it->second, addr.c_str());
	} else {
		if (portmap.end() == it)
			saddr.Initialize(AF_INET6, G2_IPV6_PORT, addr.c_str());
		else
			saddr.Initialize(AF_INET6, it->second, addr.c_str());
	} 

	return m_socket.Write(buffer, length, saddr);
}

bool CG2ProtocolHandler::writePing(const std::string &addr)
{
	unsigned char test[4];
	memcpy(test, "PING", 4);
	auto it = portmap.find(addr);
	CSockAddress saddr;
	if (addr.npos == addr.find(':')) {
		if (portmap.end() == it)
			saddr.Initialize(AF_INET, G2_DV_PORT, addr.c_str());
		else
			saddr.Initialize(AF_INET, it->second, addr.c_str());
	} else {
		if (portmap.end() == it)
			saddr.Initialize(AF_INET6, G2_IPV6_PORT, addr.c_str());
		else
			saddr.Initialize(AF_INET6, it->second, addr.c_str());
	} 

	return m_socket.Write(test, 4, saddr);
}

G2_TYPE CG2ProtocolHandler::read()
{
	bool res = true;

	// Loop until we have no more data from the socket or we have data for the higher layers
	while (res)
		res = readPackets();

	return m_type;
}

bool CG2ProtocolHandler::readPackets()
{
	m_type = GT_NONE;

	// No more data?
	CSockAddress addr;
	int length = m_socket.Read(m_buffer, BUFFER_LENGTH, addr);
	if (length <= 0)
		return false;

	m_length = length;
	bool isdsvt = ((27==length || 56==length) && 0==memcmp(m_buffer, "DSVT", 4)) ? true : false;
	if (isdsvt) {
		m_type = (m_buffer[14] & 0x80U) ? GT_HEADER : GT_AMBE;
	}

	// save the incoming port (this is to enable mobile hotspots)
	// We will only save it if it's different from the "standard" port
	m_port = addr.GetPort();
	const int family = addr.GetFamily();
	if ((AF_INET==family && G2_DV_PORT!=m_port) || (AF_INET6==family && G2_IPV6_PORT!=m_port)) {
		m_address.assign(addr.GetAddress());
		if (portmap.end() == portmap.find(m_address)) {
			if (GT_HEADER == m_type)
				printf("%.6s at %s is on port %u\n", m_buffer+42, m_address.c_str(), m_port);
			portmap[m_address] = m_port;
		} else {
			if (portmap[m_address] != m_port) {
				if (GT_HEADER == m_type)
					printf("%.6s at %s is now on port %u, was %u\n", m_buffer+42, m_address.c_str(), m_port, portmap[m_address]);
				portmap[m_address] = m_port;
			}
		}
	}
	return isdsvt ? false : true;
}

CHeaderData* CG2ProtocolHandler::readHeader()
{
	if (m_type != GT_HEADER)
		return NULL;

	CHeaderData* header = new CHeaderData;

	CSockAddress addr;
	bool res = header->setG2Data(m_buffer, m_length, false, m_address, m_port);
	if (!res) {
		delete header;
		return NULL;
	}

	return header;
}

CAMBEData* CG2ProtocolHandler::readAMBE()
{
	if (m_type != GT_AMBE)
		return NULL;

	CAMBEData* data = new CAMBEData;

	bool res = data->setG2Data(m_buffer, m_length, m_address, m_port);
	if (!res) {
		delete data;
		return NULL;
	}

	return data;
}

void CG2ProtocolHandler::close()
{
	m_socket.Close();
}
