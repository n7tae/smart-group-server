/*
CIRCDDB - ircDDB client library in C++

Copyright (C) 2010-2011   Michael Dirska, DL1BFF (dl1bff@mdx.de)
Copyright (C) 2011,2012   Jonathan Naylor, G4KLX
Copyright (c) 2017,2019-2021 by Thomas A. Early

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "IRCDDB.h"
#include "IRCClient.h"
#include "IRCDDBApp.h"
#include "Utils.h"

CIRCDDB::CIRCDDB(const std::string& hostName, unsigned int port, const std::string& callsign, const std::string& password, const std::string& versionInfo)
{
	std::string update_channel("#dstar");
	app = new IRCDDBApp(update_channel, &cache);
	client = new IRCClient(app, update_channel, hostName, port, callsign, password, versionInfo);
}

CIRCDDB::~CIRCDDB()
{
	delete client;
	delete app;
}

int CIRCDDB::GetFamily()
{
	return client->GetFamily();
}

	// A false return implies a network error, or unable to log in
bool CIRCDDB::open()
{
	printf("start client and app\n");
	client->startWork();
	app->startWork();
	return true;
}

bool CIRCDDB::findUser(const std::string &user)
{
	return app->findUser(user);
}

int CIRCDDB::getConnectionState()
{
	return app->getConnectionState();
}


void CIRCDDB::rptrQTH(const std::string& callsign, double latitude, double longitude, const std::string& desc1, const std::string& desc2, const std::string& infoURL)
{
	app->rptrQTH(callsign, latitude, longitude, desc1, desc2, infoURL);
}


void CIRCDDB::rptrQRG(const std::string& callsign, double txFrequency, double duplexShift, double range, double agl)
{
	app->rptrQRG(callsign, txFrequency, duplexShift, range, agl);
}


void CIRCDDB::kickWatchdog(const std::string& callsign, const std::string& wdInfo)
{
	app->kickWatchdog(callsign, wdInfo);
}



// Send heard data, a false return implies a network error
bool CIRCDDB::sendHeard( const std::string& myCall, const std::string& myCallExt, const std::string& yourCall, const std::string& rpt1, const std::string& rpt2, unsigned char flag1, unsigned char flag2, unsigned char flag3 )
{
	if (myCall.size() != 8) {
		printf("CIRCDDB::sendHeard:myCall='%s' len != 8\n", myCall.c_str());
		return false;
	}

	if (myCallExt.size() != 4) {
		printf("CIRCDDB::sendHeard:myCallExt='%s' len != 4\n", myCallExt.c_str());
		return false;
	}

	if (yourCall.size() != 8) {
		printf("CIRCDDB::sendHeard:yourCall='%s' len != 8\n", yourCall.c_str());
		return false;
	}

	if (rpt1.size() != 8) {
		printf("CIRCDDB::sendHeard:rpt1='%s' len != 8\n", rpt1.c_str());
		return false;
	}

	if (rpt2.size() != 8) {
		printf("CIRCDDB::sendHeard:rpt2='%s' len != 8\n", rpt2.c_str());
		return false;
	}

	return app->sendHeard(myCall, myCallExt, yourCall, rpt1, rpt2, flag1, flag2, flag3, std::string("        "), std::string(""), std::string(""));
}

// Send heard data, a false return implies a network error
bool CIRCDDB::sendHeardWithTXMsg(const std::string& myCall, const std::string& myCallExt, const std::string& yourCall, const std::string& rpt1, const std::string& rpt2, unsigned char flag1, unsigned char flag2, unsigned char flag3, const std::string& network_destination, const std::string& tx_message)
{
	if (myCall.size() != 8) {
		printf("CIRCDDB::sendHeardWithTXMsg:myCall='%s' len != 8\n", myCall.c_str());
		return false;
	}

	if (myCallExt.size() != 4) {
		printf("CIRCDDB::sendHeardWithTXMsg:myCallExt='%s' len != 4\n", myCallExt.c_str());
		return false;
	}

	if (yourCall.size() != 8) {
		printf("CIRCDDB::sendHeardWithTXMsg:yourCall='%s' len != 8\n", yourCall.c_str());
		return false;
	}

	if (rpt1.size() != 8) {
		printf("CIRCDDB::sendHeardWithTXMsg:rpt1='%s' len != 8\n", rpt1.c_str());
		return false;
	}

	if (rpt2.size() != 8) {
		printf("CIRCDDB::sendHeardWithTXMsg:rpt2='%s' len != 8\n", rpt2.c_str());
		return false;
	}

	std::string dest(network_destination);
	if (0 == dest.size())
		dest = std::string("        ");

	if (8 != dest.size()) {
		printf("CIRCDDB::sendHeardWithTXMsg:network_destination='%s' len != 8\n", dest.c_str());
		return false;
	}

	std::string msg;
	if (20 == tx_message.size()) {
		for (unsigned int i=0; i < tx_message.size(); i++) {
			char ch = tx_message.at(i);
			if ((ch > 32) && (ch < 127))
				msg.push_back(ch);
			else
				msg.push_back('_');
		}
	}
	return app->sendHeard(myCall, myCallExt, yourCall, rpt1, rpt2, flag1, flag2, flag3, dest, msg, std::string(""));
}

void CIRCDDB::close()		// Implictely kills any threads in the IRC code
{
	client->stopWork();
	app->stopWork();
}
