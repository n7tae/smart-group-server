/*
 *   Copyright (C) 2010,2011 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017,2018 by Thomas A. Early N7TAE
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>

#include "SGSConfig.h"
#include "SGSApp.h"
#include "Version.h"
#include "IRCDDBClient.h"
#include "Utils.h"

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	if (2 != argc) {
		printf("usage: %s path_to_config_file\n", argv[0]);
		printf("       %s --version\n", argv[0]);
		return 1;
	}

	if ('-' == argv[1][0]) {
		printf("\nSmart Group Server Version %s Copyright (C) %s\n", VERSION.c_str(), VENDOR_NAME.c_str());
		printf("Smart Group Server comes with ABSOLUTELY NO WARRANTY; see the LICENSE for details.\n");
		printf("This is free software, and you are welcome to distribute it\nunder certain conditions that are discussed in the LICENSE file.\n\n");
		return 0;
	}

	std::string cfgFile(argv[1]);

	CSGSApp gateway(cfgFile);

	if (!gateway.init()) {
		return 1;
	}

	gateway.run();

	return 0;
}

CSGSApp::CSGSApp(const std::string &configFile) : m_configFile(configFile), m_thread(NULL)
{
}

CSGSApp::~CSGSApp()
{
}

bool CSGSApp::init()
{
	return createThread();
}

void CSGSApp::run()
{
	m_thread->run();

	printf("exiting\n");
}

bool CSGSApp::createThread()
{
	CSGSConfig config(m_configFile);
	m_thread = new CSGSThread(config.getLinkCount("XRF"), config.getLinkCount("DCS"));

	std::string CallSign, address;
	config.getGateway(CallSign, address);

	CallSign.resize(7, ' ');
	CallSign.push_back('G');

	printf("Gateway callsign set to %s, local address set to %s\n", CallSign.c_str(), address.c_str());

	std::string hostname, username, password;
	config.getIrcDDB(hostname, username, password);
	printf("ircDDB host set to %s, username set to %s\n", hostname.c_str(), username.c_str());

	if (hostname.size() && username.size()) {
		CIRCDDB *ircDDB = new CIRCDDBClient(hostname, 9007U, username, password, std::string("linux_SmartGroupServer") + std::string("-") + VERSION, address);
		bool res = ircDDB->open();
		if (!res) {
			printf("Cannot initialise the ircDDB protocol handler\n");
			return false;
		}

		m_thread->setIRC(ircDDB);
	}

	for (unsigned int i=0; i<config.getModCount(); i++) {
		std::string band, callsign, logoff, info, permanent, reflector;
		unsigned int usertimeout;
		CALLSIGN_SWITCH callsignswitch;
		bool txmsgswitch;

		config.getGroup(i, band, callsign, logoff, info, permanent, usertimeout, callsignswitch, txmsgswitch, reflector);

		if (callsign.size() && isalnum(callsign[0])) {
			std::string repeater(CallSign);
			repeater.resize(7, ' ');
			repeater.push_back(band[0]);
			m_thread->addGroup(callsign, logoff, repeater, info, permanent, usertimeout, callsignswitch, txmsgswitch, reflector);
			printf("Group %d: %s/%s using %s, \"%s\", perm: %s, timeout: %u mins, c/s switch: %s, msg switch: %s, Linked: %s\n",
				i, callsign.c_str(), logoff.c_str(), repeater.c_str(), info.c_str(), permanent.c_str(), usertimeout,
				SCS_GROUP_CALLSIGN==callsignswitch ? "Group" : "User", txmsgswitch ? "true" : "false", reflector.c_str());
		}
	}

	bool remoteEnabled;
	std::string remotePassword;
	unsigned int remotePort;
	config.getRemote(remoteEnabled, remotePassword, remotePort);
	printf("Remote enabled set to %d, port set to %u\n", int(remoteEnabled), remotePort);
	m_thread->setRemote(remoteEnabled, remotePassword, remotePort);

	m_thread->setAddress(address);
	m_thread->setCallsign(CallSign);

	return true;
}

