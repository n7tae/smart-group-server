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

	if (! get_value(cfg, "gateway.callsign", m_callsign, 3, 8, ""))
		return;
	if (0 == m_callsign.size())
		return;
	CUtils::ToUpper(m_callsign);
	get_value(cfg, "gateway.address", m_address, 0, 20, "");
	printf("GATEWAY: callsign='%s' address='%s'\n", m_callsign.c_str(), m_address.c_str());
	if (! get_value(cfg, "ircddb.hostname", m_ircddbHostname, 5, 30, "rr.openquad.net"))
		return;
	if (! get_value(cfg, "ircddb.username", m_ircddbUsername, 3, 8, ""))
		return;
	if (0 == m_ircddbUsername.size())
		m_ircddbUsername = m_callsign;
	else
		CUtils::ToUpper(m_ircddbUsername);
	get_value(cfg, "ircddb.password", m_ircddbPassword, 1, 30, "");
	printf("IRCDDB: host='%s' user='%s' password='%s'\n", m_ircddbHostname.c_str(), m_ircddbUsername.c_str(), m_ircddbPassword.c_str());

	// module parameters
	for (int i=0; i<cfg.lookup("module").getLength(); i++) {
		char key[32];
		std::string basename, subscribe, unsubscribe, band;
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
		get_value(cfg, key, band, 1, 1, "A");
		CUtils::ToUpper(band);
		if (! isalpha(band[0])) {
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
		// skip to the next module definition
		if (0 == basename.size())
			continue;

		struct Smodule *pmod = new struct Smodule;
		// pad basename with spaces
		basename.resize(7, ' ');
		pmod->callsign = basename + subscribe;
		pmod->logoff = basename + unsubscribe;
		pmod->band = band;

		sprintf(key, "module.[%d].info", i);
		get_value(cfg, key, pmod->info, 0, 20, "Smart Group Server");
		if (pmod->info.size())
			pmod->info.resize(20, ' ');

		sprintf(key, "module.[%d].permanent", i);
		get_value(cfg, key, pmod->permanent, 0, 120, "");
		CUtils::ToUpper(pmod->permanent);

		int ivalue;
		sprintf(key, "module.[%d].usertimout", i);
		get_value(cfg, key, ivalue, 0, 300, 300);
		pmod->usertimeout = (unsigned int)ivalue;

		bool bvalue;
		sprintf(key, "module.[%d].callsignswitch", i);
		get_value(cfg, key, bvalue, false);
		pmod->callsignswitch = bvalue ? SCS_GROUP_CALLSIGN : SCS_USER_CALLSIGN;

		sprintf(key, "module.[%d].txmsgswitch", i);
		get_value(cfg, key, pmod->txmsgswitch, true);

		sprintf(key, "module.[%d].reflector", i);
		if (! get_value(cfg, key, basename, 8, 8, "")) {
			printf("reflector %d must be undefined or exactly 8 chars!\n", i);
			basename.empty();
		}
		pmod->reflector.empty();
		if (basename.size()) {
			CUtils::ToUpper(basename);
			if ( (0==basename.compare(0,3,"XRF") || 0==basename.compare(0,3,"DCS")) && isdigit(basename[3]) && isdigit(basename[4]) && isdigit(basename[5]) && ' '==basename[6] && isalpha(basename[7]) )
				pmod->reflector = basename;
		}
		printf("Module %d: callsign='%s' unsubscribe='%s' info='%s' permanent='%s' usertimeout=%d callsignswitch=%s, txmsgswitch=%s reflector='%s'\n",
			i, pmod->callsign.c_str(), pmod->logoff.c_str(), pmod->info.c_str(), pmod->permanent.c_str(), pmod->usertimeout,
			SCS_GROUP_CALLSIGN==pmod->callsignswitch ? "Group" : "User", pmod->txmsgswitch ? "true" : "false", pmod->reflector.c_str());
		m_module.push_back(pmod);
	}

	// remote control
	get_value(cfg, "remote.enabled", m_remoteEnabled, false);
	if (m_remoteEnabled) {
		get_value(cfg, "remote.password", m_remotePassword, 6, 30, "");
		int ivalue;
		get_value(cfg, "remote.port", ivalue, 1000, 65000, 39999);
		m_remotePort = (unsigned int)ivalue;
		printf("Remote enabled: password='%s', port=%d\n", m_remotePassword.c_str(), m_remotePort);
	} else {
		m_remotePort = 0U;
		m_remotePassword.empty();
		printf("Remote disabled\n");
	}
}

CStarNetServerConfig::~CStarNetServerConfig()
{
	while (m_module.size()) {
		delete m_module.back();
		m_module.pop_back();
	}
}

unsigned int CStarNetServerConfig::getModCount()
{
	return m_module.size();
}

unsigned int CStarNetServerConfig::getLinkCount(const char *type)
{
	unsigned int count = 0;
	for (unsigned int i=0; i<getModCount(); i++)
		if (0 == m_module[i]->reflector.compare(0, 3, type))
			count++;
	return count;
}

bool CStarNetServerConfig::get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
	return true;
}

bool CStarNetServerConfig::get_value(const Config &cfg, const char *path, bool &value, bool default_value)
{
	if (! cfg.lookupValue(path, value))
		value = default_value;
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
	return true;
}

void CStarNetServerConfig::getGateway(std::string& callsign, std::string& address) const
{
	callsign = m_callsign;
	address  = m_address;
}

void CStarNetServerConfig::getIrcDDB(std::string& hostname, std::string& username, std::string& password) const
{
	hostname = m_ircddbHostname;
	username = m_ircddbUsername;
	password = m_ircddbPassword;
}

void CStarNetServerConfig::getStarNet(unsigned int mod, std::string& band, std::string& callsign, std::string& logoff, std::string& info, std::string& permanent, unsigned int& userTimeout, CALLSIGN_SWITCH& callsignSwitch, bool& txMsgSwitch, std::string& reflector) const
{
	band           = m_module[mod]->band;
	callsign       = m_module[mod]->callsign;
	logoff         = m_module[mod]->logoff;
	info           = m_module[mod]->info;
	permanent      = m_module[mod]->permanent;
	userTimeout    = m_module[mod]->usertimeout;
	callsignSwitch = m_module[mod]->callsignswitch;
	txMsgSwitch    = m_module[mod]->txmsgswitch;
	reflector      = m_module[mod]->reflector;
}

void CStarNetServerConfig::getRemote(bool& enabled, std::string& password, unsigned int& port) const
{
	enabled  = m_remoteEnabled;
	password = m_remotePassword;
	port     = m_remotePort;
}
