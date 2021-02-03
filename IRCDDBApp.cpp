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

#include <netdb.h>
#include <cstdio>
#include <chrono>
#include <thread>

#include "IRCDDBApp.h"
#include "Utils.h"

IRCDDBApp::IRCDDBApp(const std::string &u_chan, CCacheManager *cache)
{
	this->cache = cache;
	maxTime = ((time_t)950000000);	//februray 2000
	tablePattern = std::regex("^[0-9]$");
	datePattern = std::regex("^20[0-9][0-9]-((1[0-2])|(0[1-9]))-((3[01])|([12][0-9])|(0[1-9]))$");
	timePattern = std::regex("^((2[0-3])|([01][0-9])):[0-5][0-9]:[0-5][0-9]$");
	dbPattern = std::regex("^[0-9A-Z_]{8}$");
	sendQ = NULL;
	initReady = false;

	userListReset();

	state = 0;
	timer = 0;
	myNick = std::string("none");

	updateChannel = u_chan;

	terminateThread = false;
}

IRCDDBApp::~IRCDDBApp()
{
	delete sendQ;
}

void IRCDDBApp::rptrQTH(const std::string &callsign, double latitude, double longitude, const std::string &desc1, const std::string &desc2, const std::string &infoURL)
{
	char pstr[32];
	snprintf(pstr, 32, "%+09.5f %+010.5f", latitude, longitude);
	std::string pos(pstr);

	std::string cs(callsign);
	std::string d1(desc1);
	std::string d2(desc2);

	d1.resize(20, '_');
	d2.resize(20, '_');

	std::regex nonValid("[^a-zA-Z0-9 +&(),./'-]");
	std::smatch sm;
	while (std::regex_search(d1, sm, nonValid))
		d1.erase(sm.position(0), sm.length());
	while (std::regex_search(d2, sm, nonValid))
		d2.erase(sm.position(0), sm.length());

	ReplaceChar(pos, ',', '.');
	ReplaceChar(d1, ' ', '_');
	ReplaceChar(d2, ' ', '_');
	ReplaceChar(cs, ' ', '_');

	moduleQTHURLMutex.lock();

	moduleQTH[cs] = cs + std::string(" ") + pos + std::string(" ") + d1 + std::string(" ") + d2;

	printf("QTH: %s\n", moduleQTH[cs].c_str());

	std::string url = infoURL;

	std::regex urlNonValid("[^[:graph:]]");
	while (std::regex_search(url, sm, urlNonValid))
		url.erase(sm.position(0), sm.length());

	if (url.size()) {
		moduleURL[cs] = cs + std::string(" ") + url;
		printf("URL: %s\n", moduleURL[cs].c_str());
	}

	moduleQTHURLMutex.unlock();
	infoTimer = 5; // send info in 5 seconds
}

void IRCDDBApp::rptrQRG(const std::string &callsign, double txFrequency, double duplexShift, double range, double agl)
{
	std::string cs = callsign;
	ReplaceChar(cs, ' ', '_');

	char fstr[64];
	snprintf(fstr, 64, "%011.5f %+010.5f %06.2f %06.1f", txFrequency, duplexShift, range / 1609.344, agl);
	std::string f(fstr);
	ReplaceChar(f, ',', '.');

	moduleQRGMutex.lock();
	moduleQRG[cs] = cs + std::string(" ") + f;
	printf("QRG: %s\n", moduleQRG[cs].c_str());
	moduleQRGMutex.unlock();

	infoTimer = 5; // send info in 5 seconds
}

void IRCDDBApp::kickWatchdog(const std::string &callsign, const std::string &s)
{
	std::string text = s;

	std::regex nonValid("[^[:graph:]]");
	std::smatch sm;
	while (std::regex_search(text, sm, nonValid))
		text.erase(sm.position(0), sm.length());

	if (text.size()) {
		std::string cs = callsign;
		ReplaceChar(cs, ' ', '_');

		moduleWDMutex.lock();
		moduleWD[cs] = cs + std::string(" ") + text;
		moduleWDMutex.unlock();
		wdTimer = 60;
	}
}

int IRCDDBApp::getConnectionState()
{
	return state;
}

void IRCDDBApp::startWork()
{
	terminateThread = false;
	m_future = std::async(std::launch::async, &IRCDDBApp::Entry, this);
}

void IRCDDBApp::stopWork()
{
    terminateThread = true;
	m_future.get();
}

void IRCDDBApp::userJoin(const std::string &nick, const std::string &name, const std::string &addr)
{
	if (0 == nick.compare(0, 2, "u-")) {
		return;
	}
	std::string gate(name);
	ToUpper(gate);
	gate.resize(7, ' ');
	gate.push_back('G');
	cache->updateName(name, nick);
	cache->updateGate(gate, addr);
}

void IRCDDBApp::userLeave(const std::string &nick)
{
	if (0 == nick.compare(0, 2, "s-")) {
		currentServer.clear();
		state = 2;
		timer = 200;
		initReady = false;
		return;
	}
	std::string name(nick);
	name.pop_back();
	if ('-' == name.back()) {
		name.pop_back();
		cache->eraseName(name);
		ToUpper(name);
		name.resize(7, ' ');
		name.push_back('G');
		cache->eraseGate(name);
	}
}

void IRCDDBApp::userListReset()
{
	cache->clearGate();
}

void IRCDDBApp::setCurrentNick(const std::string &nick)
{
	myNick = nick;
	printf("IRCDDBApp::setCurrentNick %s\n", nick.c_str());
}

void IRCDDBApp::setBestServer(const std::string &ircUser)
{
	bestServer = ircUser;
	printf("IRCDDBApp::setBestServer %s\n", ircUser.c_str());
}

void IRCDDBApp::setTopic(const std::string &topic)
{
	channelTopic = topic;
}

bool IRCDDBApp::findServerUser()
{
	std::string suser(cache->findServerUser());
	if (suser.empty())
		return false;
	currentServer.assign(suser);
	return true;
}

static const int numberOfTables = 2;

bool IRCDDBApp::sendHeard(const std::string &myCall, const std::string &myCallExt, const std::string &yourCall, const std::string &rpt1, const std::string &rpt2, unsigned char flag1, unsigned char flag2, unsigned char flag3, const std::string &destination, const std::string &tx_msg, const std::string &tx_stats)
{
	std::string my(myCall);
	std::string myext(myCallExt);
	std::string ur(yourCall);
	std::string r1(rpt1);
	std::string r2(rpt2);
	std::string dest(destination);
	std::regex nonValid("[^A-Z0-9/_]");
	std::smatch sm;
	char underScore = '_';
	while (std::regex_search(my, sm, nonValid))
		my[sm.position(0)] = underScore;
	while (std::regex_search(myext, sm, nonValid))
		myext[sm.position(0)] = underScore;
	while (std::regex_search(ur, sm, nonValid))
		ur[sm.position(0)] = underScore;
	while (std::regex_search(r1, sm, nonValid))
		r1[sm.position(0)] = underScore;
	while (std::regex_search(r2, sm, nonValid))
		r2[sm.position(0)] = underScore;
	while (std::regex_search(dest, sm, nonValid))
		dest[sm.position(0)] = underScore;

	bool statsMsg = (tx_stats.size() > 0);

	std::string srv(currentServer);
	IRCMessageQueue *q = getSendQ();

	if (srv.size() && state>=6 && q) {
		std::string cmd("UPDATE ");

		cmd += getCurrentTime();
		cmd += std::string(" ") + my + std::string(" ") + r1 + std::string(" ");
		if (!statsMsg)
			cmd.append("0 ");
		cmd += r2 + std::string(" ") + ur + std::string(" ");

		char flags[10];
		snprintf(flags, 10, "%02X %02X %02X", flag1, flag2, flag3);
		cmd.append(flags);

		cmd += std::string(" ") + myext;

		if (statsMsg)
			cmd += std::string(" # ") + tx_stats;
		else {
			cmd += std::string(" 00 ") + dest;
			if (20 == tx_msg.size())
				cmd += std::string(" ") + tx_msg;
		}
		IRCMessage *m = new IRCMessage(srv, cmd);
		q->putMessage(m);
		return true;
	}
	return false;
}

bool IRCDDBApp::findUser(const std::string &usrCall)
{
	std::string srv = currentServer;
	IRCMessageQueue *q = getSendQ();

	if ((srv.length() > 0) && (state >= 6) && (q != NULL)) {
		std::string usr = usrCall;

		ReplaceChar(usr, ' ', '_');

		IRCMessage *m = new IRCMessage(srv, std::string("FIND ") + usr );

		q->putMessage(m);
	}

	return true;
}

void IRCDDBApp::msgChannel(IRCMessage *m)
{
	if (0==m->getPrefixNick().compare(0, 2, "s-") && m->numParams >= 2)  // server msg
		doUpdate(m->params[1]);
}

void IRCDDBApp::doNotFound(std::string &msg, std::string &retval)
{
	int tableID = 0;
	std::vector<std::string> tkz = stringTokenizer(msg);

	if (tkz.empty())
		return;  // no text in message

	std::string tk = tkz.front();
	tkz.erase(tkz.begin());

	if (std::regex_match(tk, tablePattern)) {
		tableID = std::stoi(tk);

		if (tableID<0 || tableID>=numberOfTables) {
			printf("invalid table ID %d\n", tableID);
			return;
		}

		if (tkz.empty())
			return;  // received nothing but the tableID

		tk = tkz.front();
		tk.erase(tk.begin());
	}

	if (0 == tableID) {
		if (! std::regex_match(tk, dbPattern))
			return; // no valid key
		retval = tk;
	}
}

void IRCDDBApp::doUpdate(std::string &msg)
{
	int tableID = 0;

	std::vector<std::string> tkz = stringTokenizer(msg);

	if (0u == tkz.size())
		return;  // no text in message

	std::string tk = tkz.front();
	tkz.erase(tkz.begin());

	if (std::regex_match(tk, tablePattern)) {
		tableID = stol(tk);
		if ((tableID < 0) || (tableID >= numberOfTables)) {
			printf("invalid table ID %d", tableID);
			return;
		}

		if (0 == tkz.size())
			return;  // received nothing but the tableID

		tk = tkz.front();
		tkz.erase(tkz.begin());
	}

	if (std::regex_match(tk, datePattern)) {
		if (0 == tkz.size())
			return;  // nothing after date string

		std::string timeToken = tkz.front();
		tkz.erase(tkz.begin());

		if (! std::regex_match(timeToken, timePattern))
			return; // no time string after date string

		std::string tstr(std::string(tk + " " + timeToken));	// used to update user time
		auto rtime = parseTime(tstr);							// used to update maxTime for sendlist

		if ((tableID == 0) || (tableID == 1)) {
			if (0 == tkz.size())
				return;  // nothing after time string

			std::string key = tkz.front();
			tkz.erase(tkz.begin());

			if (! std::regex_match(key, dbPattern))
				return; // no valid key

			if (0 == tkz.size())
				return;  // nothing after time string

			std::string value = tkz.front();
			tkz.erase(tkz.begin());

			if (! std::regex_match(value, dbPattern))
				return; // no valid key

			//printf("TABLE %d %s %s\n", tableID, key.c_str(), value.c_str());

			if (tableID == 1) {

				if (initReady && key.compare(0,6, value, 0, 6)) {
					std::string rptr(key);
					std::string gate(value);

					ReplaceChar(rptr, '_', ' ');
					ReplaceChar(gate, '_', ' ');
					gate[7] = 'G';
					cache->updateRptr(rptr, gate, "");
					if (rtime > maxTime)
						maxTime = rtime;
				}
			} else if ((tableID == 0) && initReady) {
				std::string user(key);
				std::string rptr(value);

				ReplaceChar(user, '_', ' ');
				ReplaceChar(rptr, '_', ' ');

				cache->updateUser(user, rptr, "", "", tstr);

			}
		}
	}
}

static std::string getTableIDString(int tableID, bool spaceBeforeNumber)
{
	if (0 == tableID)
		return std::string("");
	else if (tableID>0 && tableID<numberOfTables) {
		if (spaceBeforeNumber)
			return std::string(" ") + std::to_string(tableID);
		else
			return std::to_string(tableID) + std::string(" ");
	}
	else
		return " TABLE_ID_OUT_OF_RANGE ";
}

void IRCDDBApp::msgQuery(IRCMessage *m)
{
	if (0==m->getPrefixNick().compare(0, 2, "s-") && m->numParams>=2) {	// server msg
		std::string msg(m->params[1]);
		std::vector<std::string> tkz = stringTokenizer(msg);

		if (tkz.empty())
			return;  // no text in message

		std::string cmd = tkz.front();
		tkz.erase(tkz.begin());

		if (0 == cmd.compare("UPDATE")) {
			std::string restOfLine;
			while (! tkz.empty()) {
				restOfLine += tkz.front();
				tkz.erase(tkz.begin());
				if (! tkz.empty())
					restOfLine.push_back(' ');
			}
			doUpdate(restOfLine);
		} else if (0 == cmd.compare("LIST_END")) {
			if (5 == state) // if in sendlist processing state
				state = 3;  // get next table
		} else if (0 == cmd.compare("LIST_MORE")) {
			if (5 == state) // if in sendlist processing state
				state = 4;  // send next SENDLIST
		} else if (0 == cmd.compare("NOT_FOUND")) {
			std::string callsign;
			std::string restOfLine;
			while (! tkz.empty()) {
				restOfLine += tkz.front();
				tkz.erase(tkz.begin());
				if (! tkz.empty())
					restOfLine.push_back(' ');
			}
			doNotFound(restOfLine, callsign);

			if (callsign.size() > 0) {
				ReplaceChar(callsign, '_', ' ');
				findUser(callsign);
			}
		}
	}
}

void IRCDDBApp::setSendQ(IRCMessageQueue *s)
{
	sendQ = s;
}

IRCMessageQueue *IRCDDBApp::getSendQ()
{
	return sendQ;
}

std::string IRCDDBApp::getLastEntryTime(int tableID)
{
	if (1 == tableID) {
		struct tm *ptm = gmtime(&m_maxTime);
		char tstr[80];
		strftime(tstr, 80, "%Y-%m-%d %H:%M:%S", ptm);
		std::string max = tstr;
		return max;
	}
	return "DBERROR";
}

static bool needsDatabaseUpdate(int tableID)
{
	return (1 == tableID);
}

void IRCDDBApp::Entry()
{
	int sendlistTableID = 0;
	while (!terminateThread) {
		if (timer > 0)
			timer--;
		switch(state) {
			case 0:	// wait for network to start
				if (getSendQ())
					state = 1;
				break;

			case 1:	// connect to db
				state = 2;
				timer = 200;
				break;

			case 2:	// choose server
				printf("IRCDDBApp: state=2 choose new 's-'-user\n");
				if (NULL == getSendQ())
					state = 10;
				else {
					if (findServerUser()) {
						sendlistTableID = numberOfTables;
						state = 3; // next: send "SENDLIST"
					} else if (0 == timer) {
						state = 10;
						IRCMessage *m = new IRCMessage("QUIT");
						m->addParam("no op user with 's-' found.");
						IRCMessageQueue *q = getSendQ();
						if (q)
							q->putMessage(m);
					}
				}
				break;

			case 3:
				if (NULL == getSendQ())
					state = 10; // disconnect DB
				else {
					sendlistTableID--;
					if (sendlistTableID < 0)
						state = 6; // end of sendlist
					else {
						printf("IRCDDBApp: state=3 tableID=%d\n", sendlistTableID);
						state = 4; // send "SENDLIST"
						timer = 900; // 15 minutes max for update
					}
				}
				break;

			case 4:
				if (NULL == getSendQ())
					state = 10; // disconnect DB
				else {
					if (needsDatabaseUpdate(sendlistTableID)) {
						IRCMessage *m = new IRCMessage(currentServer, std::string("SENDLIST") + getTableIDString(sendlistTableID, true) + std::string(" ") + getLastEntryTime(sendlistTableID));
						IRCMessageQueue *q = getSendQ();
						if (q)
							q->putMessage(m);
						state = 5; // wait for answers
					} else
						state = 3; // don't send SENDLIST for this table, go to next table
				}
				break;

			case 5: // sendlist processing
				if (NULL == getSendQ())
					state = 10; // disconnect DB
				else if (0 == timer) {
					state = 10; // disconnect DB
					IRCMessage *m = new IRCMessage("QUIT");
					m->addParam("timeout SENDLIST");
					IRCMessageQueue *q = getSendQ();
					if (q)
						q->putMessage(m);
				}
				break;

			case 6:
				if (NULL == getSendQ())
					state = 10; // disconnect DB
				else {
					printf( "IRCDDBApp: state=6 initialization completed\n");
					infoTimer = 2;
					initReady = true;
					state = 7;
				}
				break;

			case 7: // standby state after initialization
				if (NULL == getSendQ())
					state = 10; // disconnect DB

				if (infoTimer > 0) {
					infoTimer--;

					if (0 == infoTimer) {
						moduleQTHURLMutex.lock();
						for (auto it = moduleQTH.begin(); it != moduleQTH.end(); ++it) {
							std::string value = it->second;
							IRCMessage *m = new IRCMessage(currentServer, std::string("IRCDDB RPTRQTH: ") + value);
							IRCMessageQueue *q = getSendQ();
							if (q != NULL)
								q->putMessage(m);
						}
						moduleQTH.clear();

						for (auto it = moduleURL.begin(); it != moduleURL.end(); ++it) {
							std::string value = it->second;
							IRCMessage *m = new IRCMessage(currentServer, std::string("IRCDDB RPTRURL: ") + value);
							IRCMessageQueue *q = getSendQ();
							if (q != NULL)
								q->putMessage(m);
						}
						moduleURL.clear();
						moduleQTHURLMutex.unlock();

						moduleQRGMutex.lock();
						for (auto it = moduleQRG.begin(); it != moduleQRG.end(); ++it) {
							std::string value = it->second;
							IRCMessage* m = new IRCMessage(currentServer, std::string("IRCDDB RPTRQRG: ") + value);
							IRCMessageQueue* q = getSendQ();
							if (q != NULL)
								q->putMessage(m);
						}
						moduleQRG.clear();
						moduleQRGMutex.unlock();
					}
				}

				if (wdTimer > 0) {
					wdTimer--;

					if (0 == wdTimer) {
						moduleWDMutex.lock();

						for (auto it = moduleWD.begin(); it != moduleWD.end(); ++it) {
							std::string value = it->second;
							IRCMessage *m = new IRCMessage(currentServer, std::string("IRCDDB RPTRSW: ") + value);
							IRCMessageQueue *q = getSendQ();
							if (q)
								q->putMessage(m);
						}
						moduleWD.clear();
						moduleWDMutex.unlock();
					}
				}
				break;

			case 10:
				// disconnect db
				state = 0;
				timer = 0;
				initReady = false;
				break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} // while
	return;
}
