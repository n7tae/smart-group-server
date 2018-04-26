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
#include <sstream>
#include <iostream>

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
	CUtils::ToUpper(m_callsign);
	get_value(cfg, "gateway.address", m_address, 0, 20, "");
	printf("GATEWAY: callsign='%s' address='%s'\n", m_callsign.c_str(), m_address.c_str());

	//ircDDB Networks
	for(int i = 0; i < cfg.lookup("ircddb").getLength(); i++) {
		SircDDB * ircddb = new SircDDB();
		std::stringstream key;
		key << "ircddb.[" << i << "].hostname";
		if(! get_value(cfg, key.str(), ircddb->hostname, 5, 30, "") || ircddb->hostname == "") {//do not allow hostname to be empty
			delete ircddb;
			continue;
		}

		key.str("");key.clear();
		key << "ircddb.[" << i << "].username";
		if (! get_value(cfg, key.str(), ircddb->username, 3, 8, m_callsign)) {//default user name to callsign
			delete ircddb;
			continue;
		}
		CUtils::ToUpper(ircddb->username);

		key.str("");key.clear();
		key << "ircddb.[" << i << "].password";
		if(!get_value(cfg, key.str(), ircddb->password, 1, 30, "")) {
			delete ircddb;
			continue;
		}

		ircddb->isQuadNet = ircddb->hostname.find("openquad.net") != std::string::npos;
		this->m_ircDDB.push_back(ircddb);
		std::cout << "IRCDDB: host=" << ircddb->hostname << " user=" << ircddb->username << " password=" << ircddb->password << "\n";
	}

	if(this->m_ircDDB.size() == 0) {//no ircddb network specified? Default to openquad!
		SircDDB * ircddb  = new SircDDB();
		ircddb->hostname  = "rr.openquad.net";
		ircddb->password  = "";
		ircddb->username  = m_callsign;
		ircddb->isQuadNet = true;
		this->m_ircDDB.push_back(ircddb);
		std::cout << "No ircDDB networks configure'd, defaulting to IRCDDB: host=" << ircddb->hostname << " user=" << ircddb->username << " password=" << ircddb->password << "\n";
	}

	// module parameters
	for (int i=0; i<cfg.lookup("module").getLength(); i++) {
		std::stringstream key;
		std::string basename, subscribe, unsubscribe, band;
		key << "module.[" << i << "].basename";
		if (get_value(cfg, key.str(), basename, 1, 7, "")) {
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

		key.str("");key.clear();
		key << "module.[" << i << "].band";
		get_value(cfg, key.str(), band, 1, 1, "A");
		CUtils::ToUpper(band);
		if (! isalpha(band[0])) {
			printf("Module %d band is not a letter\n", i);
			basename.empty();
		}

		key.str("");key.clear();
		key << "module.[" << i << "].subscribe";
		get_value(cfg, key.str(), subscribe, 1, 1, "A");
		CUtils::ToUpper(subscribe);
		if (subscribe[0] != ' ' && ('A' > subscribe[0] || subscribe[0] > 'Z')) {
			printf("subscribe suffix not space or letter\n");
			basename.empty();
		}

		key.str("");key.clear();
		key << "module.[" << i << "].unsubscribe";
		get_value(cfg, key.str(), unsubscribe, 1, 1, "T");
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

		key.str("");key.clear();
		key << "module.[" << i << "].info";
		get_value(cfg, key.str(), pmod->info, 0, 20, "Smart Group Server");
		if (pmod->info.size())
			pmod->info.resize(20, ' ');

		key.str("");key.clear();
		key << "module.[" << i << "].permanent";
		get_value(cfg, key.str(), pmod->permanent, 0, 120, "");
		CUtils::ToUpper(pmod->permanent);

		int ivalue;
		key.str("");key.clear();
		key << "module.[" << i << "].usertimeout";
		get_value(cfg, key.str(), ivalue, 0, 300, 300);
		pmod->usertimeout = (unsigned int)ivalue;

		bool bvalue;
		key.str("");key.clear();
		key << "module.[" << i << "].callsignswitch";
		get_value(cfg, key.str(), bvalue, false);
		pmod->callsignswitch = bvalue ? SCS_GROUP_CALLSIGN : SCS_USER_CALLSIGN;

		key.str("");key.clear();
		key << "module.[" << i << "].txmsgswitch";
		get_value(cfg, key.str(), pmod->txmsgswitch, true);

		key.str("");key.clear();
		key << "module.[" << i << "].reflector";
		if (! get_value(cfg, key.str(), basename, 8, 8, "")) {
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

CSGSConfig::~CSGSConfig()
{
	while (m_module.size()) {
		delete m_module.back();
		m_module.pop_back();
	}

	while(m_ircDDB.size()) {
		delete m_ircDDB.back();
		m_ircDDB.pop_back();
	}
}

unsigned int CSGSConfig::getModCount()
{
	return m_module.size();
}

unsigned int CSGSConfig::getIrcDDBCount()
{
	return m_ircDDB.size();
}

unsigned int CSGSConfig::getLinkCount(const char *type)
{
	unsigned int count = 0;
	for (unsigned int i=0; i<getModCount(); i++)
		if (0 == m_module[i]->reflector.compare(0, 3, type))
			count++;
	return count;
}

bool CSGSConfig::get_value(const Config &cfg, const std::string &path, int &value, int min, int max, int default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
	return true;
}

bool CSGSConfig::get_value(const Config &cfg, const std::string &path, bool &value, bool default_value)
{
	if (! cfg.lookupValue(path, value))
		value = default_value;
	return true;
}

bool CSGSConfig::get_value(const Config &cfg, const std::string &path, std::string &value, int min, int max, const std::string &default_value)
{
	if (cfg.lookupValue(path, value)) {
		int l = value.length();
		if (l<min || l>max) {
			std::cout << path << "=" << value << " has an inalid length, must be between " << min << " and " << " max\n";
			return false;
		}
	} else
		value = default_value;
	return true;
}

void CSGSConfig::getGateway(std::string& callsign, std::string& address) const
{
	callsign = m_callsign;
	address  = m_address;
}

void CSGSConfig::getIrcDDB(unsigned int ircddb, std::string& hostname, std::string& username, std::string& password, bool &isQuadNet) const
{
	hostname  = m_ircDDB[ircddb]->hostname;
	username  = m_ircDDB[ircddb]->username;
	password  = m_ircDDB[ircddb]->password;
	isQuadNet = m_ircDDB[ircddb]->isQuadNet;
}

void CSGSConfig::getGroup(unsigned int mod, std::string& band, std::string& callsign, std::string& logoff, std::string& info, std::string& permanent, unsigned int& userTimeout, CALLSIGN_SWITCH& callsignSwitch, bool& txMsgSwitch, std::string& reflector) const
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

void CSGSConfig::getRemote(bool& enabled, std::string& password, unsigned int& port) const
{
	enabled  = m_remoteEnabled;
	password = m_remotePassword;
	port     = m_remotePort;
}
