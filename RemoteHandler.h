/*
 *   Copyright (C) 2011,2013 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2018 by Thomas A. Early
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
#include <cstdint>

#include "GroupHandler.h"
#include "TLSServer.h"

class CRemoteHandler {
public:
	CRemoteHandler() {};
	~CRemoteHandler() {};

	bool open(const std::string &password, const unsigned short port, const bool isIPV6);

	bool process();

private:
	std::string	m_password;
	CTLSServer	m_tlsserver;

	void sendGroup(CGroupHandler *group);
	void link(CGroupHandler *group, const std::string &reflector);
	void unlink(CGroupHandler *group);
	void logoff(CGroupHandler *group, const std::string &user);
};
