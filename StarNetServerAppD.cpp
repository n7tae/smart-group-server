/*
 *   Copyright (C) 2010,2011 by Jonathan Naylor G4KLX
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <string>

#include "StarNetServerConfig.h"
#include "StarNetServerAppD.h"
#include "StarNetServerDefs.h"
//#include "APRSWriter.h"
#include "Version.h"
#include "IRCDDBClient.h"
#include "Utils.h"

int main(int argc, char** argv)
{
	std::string cfgFile;
	while (1) {
		static struct option long_options[] = {
			{"help",    no_argument,       0, 'h'},
			{"cfgfile", required_argument, 0, 'c'},
			{0,         0,                 0,   0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		int c = getopt_long_only(argc, argv, "hc:", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				::printf("usage: %s [-c<cfgfile>|--cfgfile=<cfgfile>]\n", argv[0]);
				exit(0);
			case 'c':
				cfgFile.assign(optarg);
				break;
			default:
				exit(1);
		}
	}
	if (optind < argc) {
		printf ("Unknown arguments: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);
		putchar ('\n');
		exit(1);
	}

	CStarNetServerAppD gateway(cfgFile);

	if (!gateway.init()) {
		return 1;
	}

	gateway.run();

	return 0;
}

CStarNetServerAppD::CStarNetServerAppD(const std::string &configFile) : m_configFile(configFile), m_thread(NULL)
{
}

CStarNetServerAppD::~CStarNetServerAppD()
{
}

bool CStarNetServerAppD::init()
{
	return createThread();
}

void CStarNetServerAppD::run()
{
	m_thread->run();

	CUtils::lprint("exiting");
}

bool CStarNetServerAppD::createThread()
{
	CStarNetServerConfig config(m_configFile);

	m_thread = new CStarNetServerThread();

	std::string CallSign, address;
	config.getGateway(CallSign, address);

	CallSign.resize(7, ' ');
	CallSign.push_back('G');

	CUtils::lprint("Gateway callsign set to %s, local address set to %s", CallSign.c_str(), address.c_str());

	bool logEnabled;
	config.getMiscellaneous(logEnabled);
	CUtils::lprint("Log enabled set to %s", logEnabled ? "true" : "false");

	std::string hostname, username, password;
	config.getIrcDDB(hostname, username, password);
	CUtils::lprint("ircDDB host set to %s, username set to %s", hostname.c_str(), username.c_str());

	if (hostname.size() && username.size()) {
		CIRCDDB *ircDDB = new CIRCDDBClient(hostname, 9007U, username, password, std::string("linux_") + LOG_BASE_NAME + std::string("-") + VERSION, address); 
		bool res = ircDDB->open();
		if (!res) {
			CUtils::lprint("Cannot initialise the ircDDB protocol handler");
			return false;
		}

		m_thread->setIRC(ircDDB);
	}

	for (unsigned int i=0; i<MAX_STARNETS; i++) {
		std::string band, callsign, logoff, info, permanent, reflector;
		unsigned int usertimeout, grouptimeout;
		STARNET_CALLSIGN_SWITCH callsignswitch;
		bool txmsgswitch;
		
#if defined(DEXTRA_LINK) || defined(DCS_LINK)
		config.getStarNet(i, band, callsign, logoff, info, permanent, usertimeout, grouptimeout, callsignswitch, txmsgswitch, reflector);
#else
		config.getStarNet(i, band, callsign, logoff, info, permanent, usertimeout, grouptimeout, callsignswitch, txmsgswitch);
#endif

		if (callsign.size() && isalnum(callsign[0])) {
			std::string repeater(CallSign);
			repeater.resize(7, ' ');
			repeater.push_back(band[0]);
#if defined(DEXTRA_LINK) || defined(DCS_LINK)
			m_thread->addStarNet(callsign, logoff, repeater, info, permanent, usertimeout, grouptimeout, callsignswitch, txmsgswitch, reflector);
			CUtils::lprint("StarNet %d set to %s/%s on repeater %s, info: \"%s\", permanent: %s, user: %u mins, group: %u mins, callsign switch: %s, tx msg switch: %s, reflector: %s",
				i, callsign.c_str(), logoff.c_str(), repeater.c_str(), info.c_str(), permanent.c_str(), usertimeout, grouptimeout,
				SCS_GROUP_CALLSIGN==callsignswitch ? "Group" : "User", txmsgswitch ? "true" : "false", reflector.c_str());
#else
			m_thread->addStarNet(callsign, logoff, repeater, info, permanent, usertimeout, grouptimeout, callsignswitch, txmsgswitch);
			CUtils::lprint("StarNet %d set to %s/%s on repeater %s, info: \"%s\", permanent: %s, user: %u mins, group: %u mins, callsign switch: %s, tx msg switch: %s",
				i, callsign.c_str(), logoff.c_str(), repeater.c_str(), info.c_str(), permanent.c_str(), usertimeout, grouptimeout,
				SCS_GROUP_CALLSIGN==callsignswitch ? "Group" : "User", txmsgswitch ? "true" : "false");
#endif
		}
	}

	bool remoteEnabled;
	std::string remotePassword;
	unsigned int remotePort;
	config.getRemote(remoteEnabled, remotePassword, remotePort);
	CUtils::lprint("Remote enabled set to %d, port set to %u", int(remoteEnabled), remotePort);
	m_thread->setRemote(remoteEnabled, remotePassword, remotePort);

	m_thread->setAddress(address);
	m_thread->setCallsign(CallSign);

	return true;
}

