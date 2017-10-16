// Copyright (c) 2017 by Thomas A. Early N7TAE

#include <string>

#include "Utilities.h"
#include "SmartServerConfig.h"


CSmartServerConfig::CSmartServerConfig(const std::string &pathname)
{
	
	if (pathname.size() < 1) {
		lprint("Configuration filename too short!");
		return;
	}

	Config cfg;
	try {
		cfg.readFile(pathname.c_str());
	}
	catch(const FileIOException &fioex) {
		lprint("Can't read %s\n", pathname.c_str());
		return;
	}
	catch(const ParseException &pex) {
		lprint("Parse error at %s:%d - %s\n", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

	if (! get_value(cfg, "ircddb.callsign", m_callsign, 3, 8, "ABXCDE"))
		return;
	toupper(m_callsign);
	get_value(cfg, "ircddb.address", m_address, 0, 20, "");
	if (! get_value(cfg, "ircddb.hostname", m_ircddbHostname, 5, 30, "rr.openquad.net"))
		return;
	if (! get_value(cfg, "ircddb.username", m_ircddbUsername, 3, 8, "ABXCDE"))
		return;
	toupper(m_ircddbUsername);
	get_value(cfg, "ircddb.password", m_ircddbPassword, 1, 30, "");
	lprint("IRCDDB: calsign='%s' address='%s' host='%s' user='%s' password='%s'", m_callsign, m_address, m_ircddbHostname, m_ircddbUsername, m_ircddbPassword);

	// module parameters
	for (int i=1; i<=15; i++) {
		char key[32];
		std::string basename, subscribe, unsubscribe;
		sprintf(key, "module.%d.basename", i);
		if (! get_value(cfg, key, basename, 1, 7, ""))
			basename.empty();
		else {
			toupper(basename);
			if (! isalnum(basename[0])) {
				lprint("Malformed basename for module %d", i);
				basename.empty();
			}
		}
		
		sprintf(key, "module.%d.band", i);
		get_value(cfg, key, module[i].band, 1, 1, "A");
		toupper(module[i].band);
		if (! isalpha(module[i].band[0])) {
			lprint("Module %d band is not a letter", i);
			basename.empty();
		}
		
		sprintf(key, "module.%d,subscribe", i);
		get_value(cfg, key, subscribe, 1, 1, "A");
		toupper(subscribe);
		if (subscribe[0] != ' ' && ('A' > subscribe[0] || subscribe[0] > 'Z')) {
			lprint("subscribe suffix not space or letter");
			basename.empty();
		}
		sprintf(key, "module.%d.unsubscribe", i);
		get_value(cfg, key, unsubscribe, 1, 1, "T");
		toupper(unsubscribe);
		if ('A' > unsubscribe[0] || unsubscribe[0] > 'Z') {
			lprint("unsubscribe suffix not a letter");
			basename.empty();
		}
		if (! subscribe.compare(unsubscribe)) {
			// subscribe and unsubscribe suffix needs to be different
			lprint("subscribe and unsubscribe for %s are identical", basename);
			basename.empty();
		}
		if (0 == basename.size()) {
			module[i].callsign.empty();
			continue;
		}

		// pad basename with spaces
		while (basename.size() < 7)
			basename.push_back(' ');
		module[i].callsign = basename + subscribe;
		module[i].logoff = basename + unsubscribe;
		
		sprintf(key, "module.%d.info", i);
		get_value(cfg, key, module[i].info, 0, 20, "");

		sprintf(key, "module.%d.permanent", i);
		get_value(cfg, key, module[i].permanent, 0, 8, "");
		toupper(module[i].permanent);

		int ivalue;
		sprintf(key, "module.%d.usertimout", i);
		get_value(cfg, key, ivalue, 0, 300, 300);
		module[i].usertimeout = (unsigned int)ivalue;
	
		sprintf(key, "module.%d.grouptimeout", i);
		get_value(cfg, key, ivalue, 0, 300, 300);
		module[i].grouptimeout = (unsigned int)ivalue;

		bool bvalue;
		sprintf(key, "module.%d.callsignswitch", i);
		get_value(cfg, key, bvalue, true);
		module[i].callsignswitch = bvalue ? SCS_GROUP_CALLSIGN : SCS_USER_CALLSIGN;

		sprintf(key, "module.%d.txmsgswitch", i);
		get_value(cfg, key, module[i].txmsgswitch, true);

		sprintf(key, "module.%d.reflector", i);
		if (! get_value(cfg, key, basename, 8, 8, "")) {
			lprint("reflector %d must be undefined or exactly 8 chars!", i);
			basename.empty();
		}
		if (basename.size()) {
			toupper(basename);
			if ( (0==basename.compare(0,3,"XRF") || 0==basename.compare(0,3,"DCS"))
							&& isdigit(basename[3]) && isdigit(basename[4]) && isdigit(basename[5]) && ' '==basename[6] && isalpha(basename[7]) )
				module[i].reflector = basename;
			else
				module[i].reflector.empty();
		}
		lprint("Module %d: callsign='%s' unsubscribe='%s' info='%s' permanent='%s' usertimeout=%d grouptimeout=%d callsignswitch=%s, txmsgswitch=%s reflector='%s'",
			i, module[i].callsign, module[i].logoff, module[i].info, module[i].permanent, module[i].usertimeout, module[i].grouptimeout,
			SCS_GROUP_CALLSIGN==module[i].callsignswitch ? "Group" : "User",
			module[i].txmsgswitch ? "true" : "false", module[i].reflector);
	}

	// remote control
	get_value(cfg, "remote.enabled", m_remoteEnabled, false);
	if (m_remoteEnabled) {
		get_value(cfg, "remote.password", m_remotePassword, 6, 30, "");
		int ivalue;
		get_value(cfg, "remote.port", ivalue, 1000, 65000, 32156);
		m_remotePort = (unsigned int)ivalue;
		get_value(cfg, "remote.logEnabled", m_logEnabled, false);
		get_value(cfg, "remote.windowX", ivalue, 0, 2000, 0);
		m_x = (unsigned int)ivalue;
		get_value(cfg, "remote.windowY", ivalue, 0, 2000, 0);
		m_y = (unsigned int)ivalue;
		lprint("Remote enabled: password='%s', port=%d, logEnabled=%s windowX=%d windowY=%d", m_remotePassword, m_remotePort, m_logEnabled?"true":"false", m_x, m_y);
	} else {
		m_remotePort = m_x = m_y = 0U;
		m_remotePassword.empty();
		m_logEnabled = false;
		lprint("Remote disabled");
	}
}

CSmartServerConfig::~CSmartServerConfig()
{
}

bool CSmartServerConfig::get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
	lprint("%s = [%u]\n", path, value);
	return true;
}

bool CSmartServerConfig::get_value(const Config &cfg, const char *path, bool &value, bool default_value)
{
	if (! cfg.lookupValue(path, value))
		value = default_value;
	lprint("%s = [%s]\n", path, value ? "true" : "false");
	return true;
}

bool CSmartServerConfig::get_value(const Config &cfg, const char *path, std::string &value, int min, int max, const char *default_value)
{
	if (cfg.lookupValue(path, value)) {
		int l = value.length();
		if (l<min || l>max) {
			lprint("%s is invalid\n", path, value.c_str());
			return false;
		}
	} else
		value = default_value;
	lprint("%s = [%s]\n", path, value.c_str());
	return true;
}


void CSmartServerConfig::getGateway(std::string& callsign, std::string& address) const
{
	callsign = m_callsign;
	address  = m_address;
}

void CSmartServerConfig::setGateway(const std::string& callsign, const std::string& address)
{
	m_callsign = callsign;
	m_address  = address;
}

void CSmartServerConfig::getIrcDDB(std::string& hostname, std::string& username, std::string& password) const
{
	hostname = m_ircddbHostname;
	username = m_ircddbUsername;
	password = m_ircddbPassword;
}

void CSmartServerConfig::setIrcDDB(const std::string& hostname, const std::string& username, const std::string& password)
{
	m_ircddbHostname = hostname;
	m_ircddbUsername = username;
	m_ircddbPassword = password;
}

#if defined(DEXTRA_LINK) || defined(DCS_LINK)
void CSmartServerConfig::getStarNet(int mod, std::string& band, std::string& callsign, std::string& logoff, std::string& info, std::string& permanent, unsigned int& userTimeout, unsigned int& groupTimeout, STARNET_CALLSIGN_SWITCH& callsignSwitch, bool& txMsgSwitch, std::string& reflector) const
#else
void CSmartServerConfig::getStarNet(int mod, std::string& band, std::string& callsign, std::string& logoff, std::string& info, std::string& permanent, unsigned int& userTimeout, unsigned int& groupTimeout, STARNET_CALLSIGN_SWITCH& callsignSwitch, bool& txMsgSwitch) const
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
void CSmartServerConfig::setStarNet(int mod, const std::string& band, const std::string& callsign, const std::string& logoff, const std::string& info, const std::string& permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch, const std::string& reflector)
#else
void CSmartServerConfig::setStarNet(int mod, const std::string& band, const std::string& callsign, const std::string& logoff, const std::string& info, const std::string& permanent, unsigned int userTimeout, unsigned int groupTimeout, STARNET_CALLSIGN_SWITCH callsignSwitch, bool txMsgSwitch)
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


void CSmartServerConfig::getRemote(bool& enabled, std::string& password, unsigned int& port) const
{
	enabled  = m_remoteEnabled;
	password = m_remotePassword;
	port     = m_remotePort;
}

void CSmartServerConfig::setRemote(bool enabled, const std::string& password, unsigned int port)
{
	m_remoteEnabled  = enabled;
	m_remotePassword = password;
	m_remotePort     = port;
}

void CSmartServerConfig::getMiscellaneous(bool& enabled) const
{
	enabled = m_logEnabled;
}

void CSmartServerConfig::setMiscellaneous(bool enabled)
{
	m_logEnabled = enabled;
}

void CSmartServerConfig::getPosition(int& x, int& y) const
{
	x = m_x;
	y = m_y;
}

void CSmartServerConfig::setPosition(int x, int y)
{
	m_x = x;
	m_y = y;
}
