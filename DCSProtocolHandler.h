/*
 *   Copyright (C) 2012,2013 by Jonathan Naylor G4KLX
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

#include <netinet/in.h>
#include <string>

#include "UDPReaderWriter.h"
#include "DStarDefines.h"
#include "ConnectData.h"
#include "AMBEData.h"
#include "PollData.h"

enum DCS_TYPE {
	DC_NONE,
	DC_DATA,
	DC_POLL,
	DC_CONNECT
};

class CDCSProtocolHandler {
public:
	CDCSProtocolHandler(int family, unsigned short port);
	~CDCSProtocolHandler();

	bool open();

	unsigned short getPort() const;

	bool writeData(const CAMBEData &data);
	bool writeConnect(const CConnectData &connect);
	bool writePoll(const CPollData &poll);

	DCS_TYPE      read();
	CAMBEData    *readData();
	CPollData    *readPoll();
	CConnectData *readConnect();

	void close();

private:
	int              m_family;
	CUDPReaderWriter m_socket;
	DCS_TYPE         m_type;
	unsigned char	*m_buffer;
	unsigned int     m_length;
	std::string      m_yourAddress;
	unsigned short   m_yourPort;
	unsigned short   m_myPort;

	bool readPackets();
};

