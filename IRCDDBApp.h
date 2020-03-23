/*
CIRCDDB - ircDDB client library in C++

Copyright (C) 2010-2011   Michael Dirska, DL1BFF (dl1bff@mdx.de)
Copyright (C) 2012        Jonathan Naylor, G4KLX
Copyright (c) 2017,2020 by Thomas A. Early N7TAE

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

#pragma once

#include <string>
#include <future>
#include <ctime>
#include <vector>
#include <regex>
#include <map>

#include "IRCDDB.h"
#include "IRCMessageQueue.h"

class IRCDDBAppUserObject
{
public:
	std::string nick;
	std::string name;
	std::string host;
	bool op;
	unsigned int usn;

	IRCDDBAppUserObject() {}

	IRCDDBAppUserObject(const std::string &n, const std::string &nm, const std::string &h)
	{
		nick = n;
		name = nm;
		host = h;
		op = false;
		usn = 0;
	}
};

class IRCDDBAppRptrObject
{
public:
	std::string arearp_cs;
	time_t lastChanged;
	std::string zonerp_cs;

	IRCDDBAppRptrObject ()
	{
	}

	IRCDDBAppRptrObject (time_t &dt, std::string &repeaterCallsign, std::string &gatewayCallsign, time_t &maxTime)
	{
		arearp_cs = repeaterCallsign;
		lastChanged = dt;
		zonerp_cs = gatewayCallsign;

		if (dt > maxTime)
			maxTime = dt;
	}
};

class IRCDDBApp
{
public:
	IRCDDBApp(const std::string &update_channel);

	~IRCDDBApp();

	void userJoin(const std::string &nick, const std::string &name, const std::string &host);

	void userLeave(const std::string &nick);

	void userChanOp(const std::string &nick, bool op);
	void userListReset();

	void msgChannel(IRCMessage *m);
	void msgQuery(IRCMessage *m);

	void setCurrentNick(const std::string &nick);
	void setTopic(const std::string &topic);

	void setBestServer(const std::string &ircUser);

	void setSendQ(IRCMessageQueue *s);
	IRCMessageQueue *getSendQ();

	void startWork();
	void stopWork();

	IRCDDB_RESPONSE_TYPE getReplyMessageType();

	IRCMessage *getReplyMessage();

	bool findUser(const std::string& s);
	bool findRepeater(const std::string &s);
	bool findGateway(const std::string &s);

	bool sendHeard(const std::string &myCall, const std::string &myCallExt, const std::string &yourCall, const std::string &rpt1, const std::string &rpt2, unsigned char flag1, unsigned char flag2, unsigned char flag3, const std::string &destination, const std::string &tx_msg, const std::string &tx_stats);

	void sendSGSInfo(const std::string &subcommand, const std::vector<std::string> &pars);

	int getConnectionState();

	void rptrQRG(const std::string &callsign, double txFrequency, double duplexShift, double range, double agl);

	void rptrQTH(const std::string &callsign, double latitude, double longitude, const std::string &desc1, const std::string &desc2, const std::string &infoURL);

	void kickWatchdog(const std::string &callsign, const std::string &wdInfo);

protected:
	void Entry();

private:
	void doUpdate(std::string& msg);
	void doNotFound(std::string &msg, std::string &retval);
	std::string getIPAddress(std::string &zonerp_cs);
	bool findServerUser();
	unsigned int calculateUsn(const std::string &nick);
	std::string getLastEntryTime(int tableID);
	time_t m_maxTime;
	std::future<void> m_future;
	int state;
	int timer;
	int infoTimer;
	int wdTimer;
	time_t maxTime;

	IRCMessageQueue *sendQ;
	IRCMessageQueue replyQ;
	std::string currentServer;
	std::string myNick;
	std::string updateChannel;
	std::string channelTopic;
	std::string bestServer;
	std::regex tablePattern;
	std::regex datePattern;
	std::regex timePattern;
	std::regex dbPattern;
	bool initReady;
	bool terminateThread;
	std::map<std::string, IRCDDBAppUserObject> user;
	std::mutex userMapMutex;
	std::map<std::string, IRCDDBAppRptrObject> rptrMap;
	std::mutex rptrMapMutex;
	std::map<std::string, std::string> moduleQRG;
	std::mutex moduleQRGMutex;
	std::map<std::string, std::string> moduleQTH;
	std::map<std::string, std::string> moduleURL;
	std::mutex moduleQTHURLMutex;
	std::map<std::string, std::string> moduleWD;
	std::mutex moduleWDMutex;
};
