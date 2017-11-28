/*
 *   Copyright (C) 2010,2011,2012 by Jonathan Naylor G4KLX
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

#include <string>

#include "Utils.h"
#include "StarNetServerConfig.h"


CStarNetServerConfig::CStarNetServerConfig(const std::string &pathname)
{
	
	if (pathname.size() < 1) {
		printf("Configuration filename too short!\n");
		return;
	}

	Config cfg;
	try {
		cfg.readFile(pathname.c_str());
	}
	catch(const FileIOException &fioex) {
		printf("Can't read %s\n", pathname.c_str());
		return;
	}
	catch(const ParseException &pex) {
		printf("Parse error at %s:%d - %s\n", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

	if (! get_value(cfg, "ircddb.callsign", m_callsign, 3, 8, ""))
		return;
	if (0 == m_callsign.size())
		return;
	CUtils::ToUpper(m_callsign);
	get_value(cfg, "ircddb.address", m_address, 0, 20, "");
	if (! get_value(cfg, "ircddb.hostname", m_ircddbHostname, 5, 30, "rr.openquad.net"))
		return;
	if (! get_value(cfg, "ircddb.username", m_ircddbUsername, 3, 8, ""))
		return;
	if (0 == m_ircddbUsername.size())
		m_ircddbUsername = m_callsign;
	else
		CUtils::ToUpper(m_ircddbUsername);
	get_value(cfg, "ircddb.password", m_ircddbPassword, 1, 30, "");
	printf("IRCDDB: calsign='%s' address='%s' host='%s' user='%s' password='%s'\n", m_callsign.c_str(), m_address.c_str(), m_ircddbHostname.c_str(),
																									m_ircddbUsername.c_str(), m_ircddbPassword.c_str());

	// module parameters
	for (int i=0; i<15; i++) {
		char key[32];
		std::string basename, subscribe, unsubscribe;
		snprintf(key, 32, "module.[%d].basename", i);
		if (get_value(cfg, key, basename, 1, 7, "")) {
			bool isokay = true;
			for (std::string::iterator it=basename.begin(); it!=basename.end(); it++) {
				if (! isalnum(*it)) {
					isokay = false;
					break;
				}
			}
			if (isokay)
				CUtils::ToUpper(basename);
			else {
				printf("Malformed basename for module %d: '%s'\n", i, basename.c_str());
				basename.empty();
			}
		}
		
		sprintf(key, "module.[%d].band", i);
		get_value(cfg, key, module[i].band, 1, 1, "A");
		CUtils::ToUpper(module[i].band);
		if (! isalpha(module[i].band[0])) {
			printf("Module %d band is not a letter\n", i);
			basename.empty();
		}
		
		sprintf(key, "module.[%d].subscribe", i);
		get_value(cfg, key, subscribe, 1, 1, "A");
		CUtils::ToUpper(subscribe);
		if (subscribe[0] != ' ' && ('A' > subscribe[0] || subscribe[0] > 'Z')) {
			printf("subscribe suffix not space or letter\n");
			basename.empty();
		}
		sprintf(key, "module.[%d].unsubscribe", i);
		get_value(cfg, key, unsubscribe, 1, 1, "T");
		CUtils::ToUpper(unsubscribe);
		if ('A' > unsubscribe[0] || unsubscribe[0] > 'Z') {
			printf("unsubscribe suffix not a letter\n");
			basename.empty();
		}
		if (! subscribe.compare(unsubscribe)) {
			// subscribe and unsubscribe suffix needs to be different
			printf("subscribe and unsubscribe for %s are identical\n", basename.c_str());
			basename.empty();
		}
		if (0 == basename.size()) {
			module[i].callsign.empty();
			continue;
		}

		// pad basename with spaces
		basename.resize(7, ' ');
		module[i].callsign = basename + subscribe;
		module[i].logoff = basename + unsubscribe;
		
		sprintf(key, "module.[%d].info", i);
		get_value(cfg, key, module[i].info, 0, 20, "");

		sprintf(key, "module.[%d].permanent", i);
		get_value(cfg, key, module[i].permanent, 0, 8, "");
		CUtils::ToUpper(module[i].permanent);

		int ivalue;
		sprintf(key, "module.[%d].usertimout", i);
		get_value(cfg, key, ivalue, 0, 300, 300);
		module[i].usertimeout = (unsigned int)ivalue;
	
		sprintf(key, "module.[%d].grouptimeout", i);
		get_value(cfg, key, ivalue, 0, 300, 300);
		module[i].grouptimeout = (unsigned int)ivalue;

		bool bvalue;
		sprintf(key, "module.[%d].callsignswitch", i);
		get_value(cfg, key, bvalue, true);
		module[i].callsignswitch = bvalue ? SCS_GROUP_CALLSIGN : SCS_USER_CALLSIGN;

		sprintf(key, "module.[%d].txmsgswitch", i);
		get_value(cfg, key, module[i].txmsgswitch, true);

		sprintf(key, "module.[%d].reflector", i);
		if (! get_value(cfg, key, basename, 8, 8, "")) {
			printf("reflector %d must be undefined or exactly 8 chars!\n", i);
			basename.empty();
		}
		if (basename.size()) {
			CUtils::ToUpper(basename);
			if ( (0==basename.compare(0,3,"XRF") || 0==basename.compare(0,3,"DCS"))
							&& isdigit(basename[3]) && isdigit(basename[4]) && isdigit(basename[5]) && ' '==basename[6] && isalpha(basename[7]) )
				module[i].reflector = basename;
			else
				module[i].reflector.empty();
		}
		printf("Module %d: callsign='%s' unsubscribe='%s' info='%s' permanent='%s' usertimeout=%d grouptimeout=%d callsignswitch=%s, txmsgswitch=%s reflector='%s'\n",
			i, module[i].callsign.c_str(), module[i].logoff.c_str(), module[i].info.c_str(), module[i].permanent.c_str(), module[i].usertimeout, module[i].grouptimeout,
			SCS_GROUP_CALLSIGN==module[i].callsignswitch ? "Group" : "User",
			module[i].txmsgswitch ? "true" : "false", module[i].reflector.c_str());
	}

	// remote control
	get_value(cfg, "remote.enabled", m_remoteEnabled, false);
	if (m_remoteEnabled) {
		get_value(cfg, "remote.password", m_remotePassword, 6, 30, "");
		int ivalue;
		get_value(cfg, "remote.port", ivalue, 1000, 65000, 32156);
		m_remotePort = (unsigned int)ivalue;
		get_value(cfg, "remote.windowX", ivalue, 0, 2000, 0);
		m_x = (unsigned int)ivalue;
		get_value(cfg, "remote.windowY", ivalue, 0, 2000, 0);
		m_y = (unsigned int)ivalue;
		printf("Remote enabled: password='%s', port=%d, windowX=%d windowY=%d\n", m_remotePassword.c_str(), m_remotePort, m_x, m_y);
	} else {
		m_remotePort = m_x = m_y = 0U;
		m_remotePassword.empty();
		printf("Remote disabled\n");
	}
}

CStarNetServerConfig::~CStarNetServerConfig()
{
}

bool CStarNetServerConfig::get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
//	printf("%s = [%u]\n", path, value);
	return true;
}

bool CStarNetServerConfig::get_value(const Config &cfg, const char *path, bool &value, bool default_value)
{
	if (! cfg.lookupValue(path, value))
		value = default_value;
//	printf("%s = [%s]\n", path, value ? "true" : "false");
	return true;
}

bool CStarNetServerConfig::get_value(const Config &cfg, const char *path, std::string &value, int min, int max, const char *default_value)
{
	if (cfg.lookupValue(path, value)) {
		int l = value.length();
		if (l<min || l>max) {
			printf("%s=%s has  invalid length\n", path, value.c_str());
			return false;
		}
	} else
		value = default_value;
//	printf("%s = [%s]\n", path, value.c_str());
	return true;
}


void CStarNetServerConfig::getGateway(std::string& callsign, std::string& address) const
{
	callsign = m_callsign;
	address  = m_address;
}

void CStarNetServerConfig::setGateway(const std::string& callsign, const std::string& address)
{
	m_callsign = callsign;
	m_address  = address;
}

void CStarNetServerConfig::getIrcDDB(std::string& hostname, std::string& username, std::string& password) const
{
	hostname = m_ircddbHostname;
	username = m_ircddbUsername;
	password = m_ircddbPassword;
}

void CStarNetServerConfig::setIrcDDB(const std::string& hostname, const std::string& username, const std::string& password)
{
	m_ircddbHostname = hostname;
	m_ircddbUsername = username;
	m_ircddbPassword = password;
}

#if defined(DEXTRA_LINK) || defined(DCS_LINK)
void CStarNetServerConfig::getStarNet(int mod, std::string& band, std::string& callsign, std::string& logoff, std::string& info, std::string& permanent, unsigned int& userTimeout, unsigned int& groupTimeout, STARNET_CALLSIGN_SWITCH& callsignSwitch, bool& txMsgSwitch, std::string& reflector) const
#else
void CStarNetServerConfig::getStarNet(int mod, std::string& band, std::string& callsign, std::string& logoff, std::string& info, std::string& permanent, unsigned int& userTimeout, unsigned int& groupTimeout, STARNET_CALLSIGN_SWITCH& callsignSwitch, bool& txMsgSwitch) const
#endif
{
	band           = module[mod].band;
	callsign       = module[mod].callsign;
	logoff         = module[mod].logoff;
	info           = module[mod].info;
	permanent      = module[mod].permanent;
	userTimeout    = module[mod].usertimeout;
	groupTimeout   = module[mod].grouptimeout;
	callsignSwitch = module[mod].callsignswitch;
	txMsgSwitch    = module[mod].txmsgswitch;

#if defined(DEXTRA_LINK) || defined(DCS_LINK)
	reflector      = module[mod].reflector;
#endif
}

#if defined(DEXTRA_LINK) || defined(DCS_LINK)
void CStarNetServerConfig::setStarNet(int mod, const std::string& band, const std::string& callsign, const std::string& logoff, const std::string& info, const std::string& permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string& reflector)
#else
void CStarNetServerConfig::setStarNet(int mod, const std::string& band, const std::string& callsign, const std::string& logoff, const std::string& info, const std::string& permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch)
#endif
{
	module[mod].band           = band;
	module[mod].callsign       = callsign;
	module[mod].logoff         = logoff;
	module[mod].info           = info;
	module[mod].permanent      = permanent;
	module[mod].usertimeout    = userTimeout;
	module[mod].grouptimeout   = groupTimeout;
	module[mod].callsignswitch = callsignSwitch;
	module[mod].txmsgswitch    = txMsgSwitch;

#if defined(DEXTRA_LINK) || defined(DCS_LINK)
	module[mod].reflector      = reflector;
#endif
}


void CStarNetServerConfig::getRemote(bool& enabled, std::string& password, unsigned int& port) const
{
	enabled  = m_remoteEnabled;
	password = m_remotePassword;
	port     = m_remotePort;
}

void CStarNetServerConfig::setRemote(bool enabled, const std::string& password, unsigned int port)
{
	m_remoteEnabled  = enabled;
	m_remotePassword = password;
	m_remotePort     = port;
}

//void CStarNetServerConfig::getMiscellaneous(bool& enabled) const
//{
	//enabled = m_logEnabled;
//}

//void CStarNetServerConfig::setMiscellaneous(bool enabled)
//{
	//m_logEnabled = enabled;
//}

void CStarNetServerConfig::getPosition(int& x, int& y) const
{
	x = m_x;
	y = m_y;
}

void CStarNetServerConfig::setPosition(int x, int y)
{
	m_x = x;
	m_y = y;
}
