/*
 *   Copyright (C) 2010,2011,2012,2014 by Jonathan Naylor G4KLX
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

#include <string>
#include <libconfig.h++>
#include "Defs.h"

using namespace libconfig;

typedef struct mod_tag {
	std::string band;
	std::string callsign;
	std::string logoff;
	std::string info;
	std::string permanent;
	std::string reflector;
	bool txmsgswitch;
	unsigned int usertimeout;
	unsigned int grouptimeout;
	STARNET_CALLSIGN_SWITCH callsignswitch;
} TMODULE;

class CStarNetServerConfig {
public:
	CStarNetServerConfig(const std::string &pathname);
	~CStarNetServerConfig();

	void getGateway(std::string &callsign, std::string &address) const;
	void setGateway(const std::string &callsign, const std::string &address);

	void getIrcDDB(std::string &hostname, std::string &username, std::string &password) const;
	void setIrcDDB(const std::string &hostname, const std::string &username, const std::string &password);

#if defined(DEXTRA_LINK) || defined(DCS_LINK)
	void getStarNet(int mod, std::string &band, std::string &callsign, std::string &logoff, std::string &info, std::string &permanent, unsigned int &userTimeout, unsigned int &groupTimeout, STARNET_CALLSIGN_SWITCH &callsignSwitch, bool &txMsgSwitch, std::string &reflector) const;
	void setStarNet(int mod, const std::string &band, const std::string &callsign, const std::string &logoff, const std::string &info, const std::string &permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string &reflector);
#else
	void getStarNet(int mod, std::string &band, std::string &callsign, std::string &logoff, std::string &info, std::string &permanent, unsigned int &userTimeout, unsigned int &groupTimeout, STARNET_CALLSIGN_SWITCH &callsignSwitch, bool &txMsgSwitch) const;
	void setStarNet(int mod, const std::string &band, const std::string &callsign, const std::string &logoff, const std::string &info, const std::string &permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch);
#endif

	void getRemote(bool &enabled, std::string &password, unsigned int &port) const;
	void setRemote(bool enabled, const std::string &password, unsigned int port);

	void getMiscellaneous(bool &enabled) const;
	void setMiscellaneous(bool enabled);

	void getPosition(int &x, int &y) const;
	void setPosition(int x, int y);

private:
	bool get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value);
	bool get_value(const Config &cfg, const char *path, bool &value, bool default_value);
	bool get_value(const Config &cfg, const char *path, std::string &value, int min, int max, const char *default_value);

	std::string m_fileName;
	std::string m_callsign;
	std::string m_address;
	std::string m_ircddbHostname;
	std::string m_ircddbUsername;
	std::string m_ircddbPassword;
	TMODULE module[15];

	bool m_remoteEnabled;
	std::string m_remotePassword;
	unsigned int m_remotePort;
	bool m_logEnabled;
	int m_x;
	int m_y;
};
