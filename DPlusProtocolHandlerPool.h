/*
 *   Copyright (C) 2012,2013 by Jonathan Naylor G4KLX
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

#include "DPlusProtocolHandler.h"

class CDPlusProtocolHandlerEntry {
public:
	CDPlusProtocolHandler* m_handler;
	unsigned int           m_port;
	bool                   m_inUse;
};

class CDPlusProtocolHandlerPool {
public:
	CDPlusProtocolHandlerPool(unsigned int n, unsigned int port, const std::string& addr = std::string(""));
	~CDPlusProtocolHandlerPool();

	bool open();

	CDPlusProtocolHandler* getHandler(unsigned int port = 0U);
	void release(CDPlusProtocolHandler* handler);

	DPLUS_TYPE    read();
	CHeaderData*  readHeader();
	CAMBEData*    readAMBE();
	CPollData*    readPoll();
	CConnectData* readConnect();

	void close();

private:
	CDPlusProtocolHandlerEntry* m_pool;
	unsigned int                m_n;
	unsigned int                m_index;
};
