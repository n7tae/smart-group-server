/*
 *   Copyright (C) 2010,2011,2012,2014 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2019 by Thomas A. Early
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
#include <vector>
#include <libconfig.h++>
#include "Defs.h"

using namespace libconfig;

struct Smodule {
	std::string band;
	std::string callsign;
	std::string logoff;
	std::string info;
	std::string reflector;
	bool txmsgswitch;
	bool listen_only;
	unsigned int usertimeout;
	CALLSIGN_SWITCH callsignswitch;
};

struct Sircddb {
	std::string Hostname;
	std::string Username;
	std::string Password;
};

class CSGSConfig {
public:
	CSGSConfig(const std::string &pathname);
	~CSGSConfig();

	void getGateway(std::string &callsign) const;

	void getIrcDDB(int irc, std::string &hostname, std::string &username, std::string &password) const;

	void getGroup(unsigned int mod, std::string &band, std::string &callsign, std::string &logoff, std::string &info,
		unsigned int &userTimeout, CALLSIGN_SWITCH &callsignSwitch, bool &txMsgSwitch, bool &listen_only, std::string &reflector) const;

	void getRemote(bool &enabled, std::string &password, unsigned short &port, bool &is_ipv6) const;

	unsigned int getModCount();
	unsigned int getLinkCount(const char *type);
	unsigned int getIRCCount();

private:
	bool get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value);
	bool get_value(const Config &cfg, const char *path, bool &value, bool default_value);
	bool get_value(const Config &cfg, const char *path, std::string &value, int min, int max, const char *default_value);

	std::string m_fileName;
	std::string m_callsign;
	std::vector<struct Smodule *> m_module;
	std::vector<struct Sircddb *> m_ircddb;

	bool m_remoteEnabled;
	std::string m_remotePassword;
	unsigned short m_remotePort;
	bool m_ipv6;
}
;
