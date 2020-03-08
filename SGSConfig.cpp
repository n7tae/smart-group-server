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
#include "SGSConfig.h"


CSGSConfig::CSGSConfig(const std::string &pathname)
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
	ToUpper(m_callsign);
	printf("GATEWAY: callsign='%s'\n", m_callsign.c_str());

	Sircddb *pirc;
	if (cfg.exists("ircddb")) {
		switch (cfg.lookup("ircddb").getLength()) {
			case 0:
				pirc = new Sircddb;
				pirc->Hostname = "rr.openquad.net";
				pirc->Username = m_callsign;
                pirc->Password.empty();
				m_ircddb.push_back(pirc);
				break;
			case 1:
				pirc = new Sircddb;
				get_value(cfg, "ircddb.[0].hostname", pirc->Hostname, 5, 64, "rr.openquad.net");
				get_value(cfg, "ircddb.[0].username", pirc->Username, 0, 8, "");
				if (0 == pirc->Username.size())
					pirc->Username = m_callsign;
				else
					ToUpper(pirc->Username);
				get_value(cfg, "ircddb.[0].password", pirc->Password, 0, 30, "");
				m_ircddb.push_back(pirc);
				break;
			default:
				for (int i=0; i<2; i++) {
					char key[32];
					pirc = new Sircddb;
					snprintf(key, 32, "ircddb.[%d].hostname", i);
					get_value(cfg, key, pirc->Hostname, 5, 64, "rr.openquad.net");
					snprintf(key, 32, "ircddb.[%d].username", i);
					get_value(cfg, key, pirc->Username, 0, 8, "");
					if (0 == pirc->Username.size())
						pirc->Username = m_callsign;
					else
						ToUpper(pirc->Username);
					snprintf(key, 32, "ircddb.[%d].password", i);
					get_value(cfg, key, pirc->Password, 0, 30, "");
					m_ircddb.push_back(pirc);
				}
				break;
		}
	} else {
		pirc = new Sircddb;
		pirc->Hostname = "rr.openquad.net";
		pirc->Username = m_callsign;
		m_ircddb.push_back(pirc);
	}

	for (unsigned int i=0; i<m_ircddb.size(); i++)
		printf("IRCDDB[%d]: host='%s' user='%s' password='%s'\n", i, m_ircddb[i]->Hostname.c_str(), m_ircddb[i]->Username.c_str(), m_ircddb[i]->Password.c_str());
	if (cfg.exists("ircddb") && 2<cfg.lookup("ircddb").getLength())
		fprintf(stderr, "A maximum of two irc servers are supported!");

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
				ToUpper(basename);
			else {
				printf("Malformed basename for module %d: '%s'\n", i, basename.c_str());
				basename.empty();
			}
		}

		sprintf(key, "module.[%d].band", i);
		get_value(cfg, key, band, 1, 1, "A");
		ToUpper(band);
		if (! isalpha(band[0])) {
			printf("Module %d band is not a letter\n", i);
			basename.empty();
		}
		if (band[0] == 'G') {
			printf("Module %d: Band G is reserved for the Gateway\n", i);
			basename.empty();
		}

		sprintf(key, "module.[%d].subscribe", i);
		get_value(cfg, key, subscribe, 1, 1, "A");
		ToUpper(subscribe);
		if (subscribe[0] != ' ' && ('A' > subscribe[0] || subscribe[0] > 'Z')) {
			printf("subscribe suffix not space or letter\n");
			basename.empty();
		}
		if ('L' == subscribe[0]) {
			printf("subscribe cannot be 'L'\n");
			basename.empty();
		}
		sprintf(key, "module.[%d].unsubscribe", i);
		get_value(cfg, key, unsubscribe, 1, 1, "T");
		ToUpper(unsubscribe);
		if ('A' > unsubscribe[0] || unsubscribe[0] > 'Z') {
			printf("unsubscribe suffix not a letter\n");
			basename.empty();
		}
		if ('L' == unsubscribe[0]) {
			printf("unsubscribe cannot be 'L'\n");
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

		int ivalue;
		sprintf(key, "module.[%d].usertimeout", i);
		get_value(cfg, key, ivalue, 0, 600, 300);
		pmod->usertimeout = (unsigned int)ivalue;

		sprintf(key, "module.[%d].rxonly", i);
		get_value(cfg, key, pmod->listen_only, false);

		bool bvalue;
		sprintf(key, "module.[%d].callsignswitch", i);
		get_value(cfg, key, bvalue, false);
		pmod->callsignswitch = bvalue ? SCS_GROUP_CALLSIGN : SCS_USER_CALLSIGN;

		sprintf(key, "module.[%d].txmsgswitch", i);
		get_value(cfg, key, pmod->txmsgswitch, true);

		sprintf(key, "module.[%d].showlink", i);
		get_value(cfg, key, pmod->showlink, true);

		sprintf(key, "module.[%d].reflector", i);
		if (! get_value(cfg, key, basename, 8, 8, "")) {
			printf("reflector %d must be undefined or exactly 8 chars!\n", i);
			basename.empty();
		}
		pmod->reflector.empty();
		if (basename.size()) {
			ToUpper(basename);
			if ( (0==basename.compare(0,3,"XRF") || 0==basename.compare(0,3,"DCS")) && isdigit(basename[3]) && isdigit(basename[4]) && isdigit(basename[5]) && ' '==basename[6] && isalpha(basename[7]) )
				pmod->reflector = basename;
		}
		printf("Module %d: callsign='%s' unsubscribe='%s' info='%s' usertimeout=%d callsignswitch=%s, txmsgswitch=%s reflector='%s'", i, pmod->callsign.c_str(), pmod->logoff.c_str(), pmod->info.c_str(), pmod->usertimeout, SCS_GROUP_CALLSIGN==pmod->callsignswitch ? "Group" : "User", pmod->txmsgswitch ? "true" : "false", pmod->reflector.c_str());
		if (pmod->showlink)
			printf("\n");
		else
			printf(" (Hidden)\n");

		m_module.push_back(pmod);

		if (pmod->listen_only && pmod->reflector.size()<8) {
			printf("Smart Group %s is RxOnly, but no reflector is defined, resetting RxOnly\n", pmod->callsign.c_str());
			pmod->listen_only = false;
		}
	}

	// remote control
	get_value(cfg, "remote.enabled", m_remoteEnabled, false);
	if (m_remoteEnabled) {
		get_value(cfg, "remote.password", m_remotePassword, 6, 30, "");
		int ivalue;
		get_value(cfg, "remote.port", ivalue, 1000, 65000, 39999);
		m_remotePort = (unsigned short)ivalue;
		std::string str;
		get_value(cfg, "remote.family", str, 0, 30, "ipv4");
		m_ipv6 = (std::string::npos == str.find('4')) ? true : false;
		printf("Remote enabled: password='%s', port=%d, family=IPV%d\n", m_remotePassword.c_str(), m_remotePort, m_ipv6 ? 6 : 4);
	} else {
		m_remotePort = 0U;
		m_remotePassword.empty();
		m_ipv6 = false;
		printf("Remote disabled\n");
	}
}

CSGSConfig::~CSGSConfig()
{
	while (m_module.size()) {
		delete m_module.back();
		m_module.pop_back();
	}

	while (m_ircddb.size()) {
		delete m_ircddb.back();
		m_ircddb.pop_back();
	}
}

unsigned int CSGSConfig::getModCount()
{
	return m_module.size();
}

unsigned int CSGSConfig::getIRCCount()
{
	return m_ircddb.size();
}

unsigned int CSGSConfig::getLinkCount(const char *type)
{
	unsigned int count = 0;
	for (unsigned int i=0; i<getModCount(); i++)
		if (0 == m_module[i]->reflector.compare(0, 3, type))
			count++;
	return count;
}

bool CSGSConfig::get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
	return true;
}

bool CSGSConfig::get_value(const Config &cfg, const char *path, bool &value, bool default_value)
{
	if (! cfg.lookupValue(path, value))
		value = default_value;
	return true;
}

bool CSGSConfig::get_value(const Config &cfg, const char *path, std::string &value, int min, int max, const char *default_value)
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

void CSGSConfig::getGateway(std::string& callsign) const
{
	callsign = m_callsign;
}

void CSGSConfig::getIrcDDB(int irc, std::string& hostname, std::string& username, std::string& password) const
{
	hostname = m_ircddb[irc]->Hostname;
	username = m_ircddb[irc]->Username;
	password = m_ircddb[irc]->Password;
}

void CSGSConfig::getGroup(unsigned int mod, std::string &band, std::string &callsign, std::string &logoff, std::string &info,
	unsigned int &userTimeout, CALLSIGN_SWITCH &callsignSwitch, bool &txMsgSwitch, bool &listen_only, bool &showlink, std::string &reflector) const
{
	band           = m_module[mod]->band;
	callsign       = m_module[mod]->callsign;
	logoff         = m_module[mod]->logoff;
	info           = m_module[mod]->info;
	userTimeout    = m_module[mod]->usertimeout;
	callsignSwitch = m_module[mod]->callsignswitch;
	txMsgSwitch    = m_module[mod]->txmsgswitch;
	listen_only    = m_module[mod]->listen_only;
	reflector      = m_module[mod]->reflector;
	showlink       = m_module[mod]->showlink;
}

void CSGSConfig::getRemote(bool &enabled, std::string &password, unsigned short &port, bool &is_ipv6) const
{
	enabled  = m_remoteEnabled;
	password = m_remotePassword;
	port     = m_remotePort;
	is_ipv6  = m_ipv6;
}
