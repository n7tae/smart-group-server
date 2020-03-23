/*
CIRCDDB - ircDDB client library in C++

Copyright (C) 2010-2011   Michael Dirska, DL1BFF (dl1bff@mdx.de)
Copyright (C) 2011,2012   Jonathan Naylor, G4KLX
Copyright (c) 2017,2019,2020 by Thomas A. Early

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

struct CIRCDDBPrivate
{
	IRCClient *client;
	IRCDDBApp *app;
};

CIRCDDB::CIRCDDB(const std::string& hostName, unsigned int port, const std::string& callsign, const std::string& password, const std::string& versionInfo) : d(new CIRCDDBPrivate)
{
	std::string update_channel("#dstar");
	d->app = new IRCDDBApp(update_channel);
	d->client = new IRCClient(d->app, update_channel, hostName, port, callsign, password, versionInfo);
}

CIRCDDB::~CIRCDDB()
{
	delete d->client;
	delete d->app;
	delete d;
}

int CIRCDDB::GetFamily()
{
	return d->client->GetFamily();
}

	// A false return implies a network error, or unable to log in
bool CIRCDDB::open()
{
	printf("start client and app\n");
	d->client->startWork();
	d->app->startWork();
	return true;
}


int CIRCDDB::getConnectionState()
{
	return d->app->getConnectionState();
}


void CIRCDDB::rptrQTH(const std::string& callsign, double latitude, double longitude, const std::string& desc1, const std::string& desc2, const std::string& infoURL)
{
	d->app->rptrQTH(callsign, latitude, longitude, desc1, desc2, infoURL);
}


void CIRCDDB::rptrQRG(const std::string& callsign, double txFrequency, double duplexShift, double range, double agl)
{
	d->app->rptrQRG(callsign, txFrequency, duplexShift, range, agl);
}


void CIRCDDB::kickWatchdog(const std::string& callsign, const std::string& wdInfo)
{
	d->app->kickWatchdog(callsign, wdInfo);
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

	return d->app->sendHeard(myCall, myCallExt, yourCall, rpt1, rpt2, flag1, flag2, flag3, std::string("        "), std::string(""), std::string(""));
}

void CIRCDDB::sendSGSInfo(const std::string subcommand, const std::vector<std::string> parms)
{
	d->app->sendSGSInfo(subcommand, parms);
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
	return d->app->sendHeard(myCall, myCallExt, yourCall, rpt1, rpt2, flag1, flag2, flag3, dest, msg, std::string(""));
}

// Send query for a gateway/reflector, a false return implies a network error
bool CIRCDDB::findGateway(const std::string& gatewayCallsign)
{
	if (8 != gatewayCallsign.size()) {
		printf("CIRCDDB::findGateway:gatewayCallsign='%s' len != 8\n", gatewayCallsign.c_str());
		return false;
	}
	std::string gw(gatewayCallsign);
	ToUpper(gw);
	return d->app->findGateway(gw);
}


bool CIRCDDB::findRepeater(const std::string& repeaterCallsign)
{
	if (8 != repeaterCallsign.size()) {
		printf("CIRCDDB::findRepeater:repeaterCallsign='%s' len != 8\n", repeaterCallsign.c_str());
		return false;
	}
	std::string rptr(repeaterCallsign);
	ToUpper(rptr);
	return d->app->findRepeater(rptr);
}

// Send query for a user, a false return implies a network error
bool CIRCDDB::findUser(const std::string& userCallsign)
{
	if (8 != userCallsign.size()) {
		printf("CIRCDDB::findUser:userCall='%s' len != 8\n", userCallsign.c_str());
		return false;
	}
	std::string usr(userCallsign);
	ToUpper(usr);
	return d->app->findUser(usr);
}

// The following functions are for processing received messages

// Get the waiting message type
IRCDDB_RESPONSE_TYPE CIRCDDB::getMessageType()
{
	return d->app->getReplyMessageType();
}

// Get a gateway message, as a result of IDRT_REPEATER returned from getMessageType()
// A false return implies a network error
bool CIRCDDB::receiveRepeater(std::string& repeaterCallsign, std::string& gatewayCallsign, std::string& address)
{
	IRCDDB_RESPONSE_TYPE rt = d->app->getReplyMessageType();

	if (rt != IDRT_REPEATER) {
		printf("CIRCDDB::receiveRepeater: unexpected response type=%d\n", rt);
		return false;
	}

	IRCMessage *m = d->app->getReplyMessage();
	if (m == NULL) {
		printf("CIRCDDB::receiveRepeater: no message\n");
		return false;
	}

	if (m->getCommand().compare("IDRT_REPEATER")) {
		printf("CIRCDDB::receiveRepeater: wrong message type, expected 'IDRT_REPEATER, got '%s'\n", m->getCommand().c_str());
		delete m;
		return false;
	}

	if (3 != m->getParamCount()) {
		printf("CIRCDDB::receiveRepeater: unexpected number of message parameters, expected 3, got %d\n", m->getParamCount());
		delete m;
		return false;
	}

	repeaterCallsign = m->getParam(0);
	gatewayCallsign = m->getParam(1);
	address = m->getParam(2);
	delete m;
	return true;
}

// Get a gateway message, as a result of IDRT_GATEWAY returned from getMessageType()
// A false return implies a network error
bool CIRCDDB::receiveGateway(std::string& gatewayCallsign, std::string& address)
{
	IRCDDB_RESPONSE_TYPE rt = d->app->getReplyMessageType();

	if (rt != IDRT_GATEWAY) {
		printf("CIRCDDB::receiveGateway: unexpected response type=%d\n", rt);
		return false;
	}

	IRCMessage *m = d->app->getReplyMessage();

	if (m == NULL) {
		printf("CIRCDDB::receiveGateway: no message\n");
		return false;
	}

	if (m->getCommand().compare("IDRT_GATEWAY")) {
		printf("CIRCDDB::receiveGateway: wrong message type, expected 'IDRT_GATEWAY' got '%s'\n", m->getCommand().c_str());
		delete m;
	return false;
	}

	if (2 != m->getParamCount()) {
		printf("CIRCDDB::receiveGateway: unexpected number of message parameters, expected 2, got %d\n", m->getParamCount());
		delete m;
	return false;
	}

	gatewayCallsign = m->getParam(0);
	address = m->getParam(1);
	delete m;
	return true;
}

// Get a user message, as a result of IDRT_USER returned from getMessageType()
// A false return implies a network error
bool CIRCDDB::receiveUser(std::string& userCallsign, std::string& repeaterCallsign, std::string& gatewayCallsign, std::string& address)
{
	std::string dummy("");
	return receiveUser(userCallsign, repeaterCallsign, gatewayCallsign, address, dummy);
}

bool CIRCDDB::receiveUser(std::string& userCallsign, std::string& repeaterCallsign, std::string& gatewayCallsign, std::string& address, std::string& timeStamp)
{
	IRCDDB_RESPONSE_TYPE rt = d->app->getReplyMessageType();

	if (rt != IDRT_USER) {
		printf("CIRCDDB::receiveUser: unexpected response type=%d\n", rt);
		return false;
	}

	IRCMessage * m = d->app->getReplyMessage();

	if (m == NULL) {
		printf("CIRCDDB::receiveUser: no message\n");
		return false;
	}

	if (m->getCommand().compare("IDRT_USER")) {
		printf("CIRCDDB::receiveUser: wrong message type, expected 'IDRT_USER', got '%s'\n", m->getCommand().c_str());
		delete m;
		return false;
	}

	if (5 != m->getParamCount()) {
		printf("CIRCDDB::receiveUser: unexpected number of message parameters, expected 5, got %d\n", m->getParamCount());
		delete m;
		return false;
	}

	userCallsign = m->getParam(0);
	repeaterCallsign = m->getParam(1);
	gatewayCallsign = m->getParam(2);
	address = m->getParam(3);
	timeStamp = m->getParam(4);
	delete m;
	return true;
}

void CIRCDDB::close()		// Implictely kills any threads in the IRC code
{
	d->client -> stopWork();
	d->app -> stopWork();
}
