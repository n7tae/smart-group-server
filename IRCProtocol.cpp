/*
CIRCDDB - ircDDB client library in C++

Copyright (C) 2010-2011   Michael Dirska, DL1BFF (dl1bff@mdx.de)
Copyright (c) 2017 by Thomas A. Early N7TAE

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

#include <stdlib.h>
#include <regex>

#include "IRCProtocol.h"
#include "Utils.h"

#define CIRCDDB_VERSION	"2.0.0"

IRCProtocol::IRCProtocol(IRCApplication * app, const std::string& callsign, const std::string& password, const std::string& channel, const std::string& versionInfo)
{
	this->password = password;
	this->channel = channel;
	this->app = app;

	this->versionInfo = "CIRCDDB:";
	this->versionInfo.append(CIRCDDB_VERSION);

	if (versionInfo.size() > 0) {
		this->versionInfo.append(" ");
		this->versionInfo.append(versionInfo);
	}

	std::string::size_type hyphenPos = callsign.find('-');

	if (hyphenPos == std::string::npos) {
		std::string n;

		n = callsign + std::string("-1");
		nicks.push_back(n);
		n = callsign + std::string("-2");
		nicks.push_back(n);
		n = callsign + std::string("-3");
		nicks.push_back(n);
		n = callsign + std::string("-4");
		nicks.push_back(n);
	} else
		nicks.push_back(callsign);

	name = callsign;

	pingTimer = 60; // 30 seconds
	state = 0;
	timer = 0;

	chooseNewNick();
}

IRCProtocol::~IRCProtocol()
{
}

void IRCProtocol::chooseNewNick()
{
	int r = rand() % nicks.size();

	currentNick = nicks[r];
}

void IRCProtocol::setNetworkReady(bool b)
{
	if (b == true) {
		if (state != 0)
		CUtils::lprint("IRCProtocol::setNetworkReady: unexpected state");

		state = 1;
		chooseNewNick();
	} else
		state = 0;
}


bool IRCProtocol::processQueues(IRCMessageQueue *recvQ, IRCMessageQueue * sendQ)
{
	if (timer > 0)
		timer--;

	while (recvQ->messageAvailable()) {
		IRCMessage *m = recvQ->getMessage();

		if (0 == m->command.compare("004")) {
			if (state == 4) {
				if (m->params.size() > 1) {
					std::regex serverNamePattern("^grp[1-9]s[1-9].ircDDB$");

					if (std::regex_match( m->params[1], serverNamePattern))
						app->setBestServer(std::string("s-") + m->params[1].substr(0,6));
				}
				state = 5;  // next: JOIN
				app->setCurrentNick(currentNick);
			}
		} else if (0 == m->command.compare("PING")) {
			IRCMessage *m2 = new IRCMessage();
			m2->command = "PONG";
			if (m->params.size() > 0) {
				m2->numParams = 1;
				m2->params.push_back(m->params[0]);
			}
			sendQ -> putMessage(m2);
		} else if (0 == m->command.compare("JOIN")) {
			if (m->numParams>=1 && 0==m->params[0].compare(channel)) {
				if (0==m->getPrefixNick().compare(currentNick) && 6==state) {
					if (debugChannel.size())
						state = 7;  // next: join debug_channel
					else
						state = 10; // next: WHO *
				}
				else if (app != NULL)
					app->userJoin(m->getPrefixNick(), m->getPrefixName(), m->getPrefixHost());
			}

			if ((m->numParams >= 1) && 0==m->params[0].compare(debugChannel)) {
				if (0==m->getPrefixNick().compare(currentNick) && 8==state)
					state = 10; // next: WHO *
			}
		} else if (0 == m->command.compare("PONG")) {
			if (state == 12) {
				timer = pingTimer;
				state = 11;
			}
		} else if (0 == m->command.compare("PART")) {
			if (m->numParams>=1 && 0==m->params[0].compare(channel)) {
				if (app != NULL)
					app->userLeave( m->getPrefixNick() );
			}
		} else if (0 == m->command.compare("KICK")) {
			if (m->numParams>=2 && 0==m->params[0].compare(channel)) {
				if (0 == m->params[1].compare(currentNick)) {
					// i was kicked!!
					delete m;
					return false;
				} else if (app)
					app->userLeave(m->params[1]);
			}
		} else if (0 == m->command.compare("QUIT")) {
			if (app)
				app->userLeave(m->getPrefixNick());
		} else if (0 == m->command.compare("MODE")) {
			if (m->numParams>=3 && 0==m->params[0].compare(channel)) {
				if (app) {
					size_t i;
					std::string mode = m->params[1];

					for (i = 1; i<mode.size() && (size_t)m->numParams>=(i+2); i++) {
						if (mode[i] == 'o') {
							if (mode[0] == '+')
								app->userChanOp(m->params[i+1], true);
							else if ( mode[0] == '-')
								app->userChanOp(m->params[i+1], false);
						}
					} // for
				}
			}
		} else if (0 == m->command.compare("PRIVMSG")) {
			if (2==m->numParams && app) {
				if (0 == m->params[0].compare(channel))
					app->msgChannel(m);
				else if (m->params[0].compare(currentNick))
					app->msgQuery(m);
			}
		} else if (0 == m->command.compare("352")) {  // WHO list
			if (m->numParams>=7 && 0==m->params[0].compare(currentNick) && m->params[1].compare(channel)) {
				if (app) {
					app->userJoin(m->params[5], m->params[2], m->params[3]);
					app->userChanOp (m->params[5], 0==m->params[6].compare("H@"));
				}
			}
		} else if (m->command.compare("433")) {  // nick collision
			if (state == 2) {
				state = 3;  // nick collision, choose new nick
				timer = 10; // wait 5 seconds..
			}
		} else if (0==m->command.compare("332") || 0==m->command.compare("TOPIC")) {  // topic
			if (2==m->numParams && app && 0==m->params[0].compare(channel) )
				app->setTopic(m->params[1]);
		}
		delete m;
	}

	IRCMessage *m;

	switch (state) {
		case 1:
			m = new IRCMessage();
			m->command = "PASS";
			m->numParams = 1;
			m->params.push_back(password);
			sendQ->putMessage(m);

			m = new IRCMessage();
			m->command = "NICK";
			m->numParams = 1;
			m->params.push_back(currentNick);
			sendQ->putMessage(m);
			timer = 10;  // wait for possible nick collision message
			state = 2;
			break;

		case 2:
			if (0 == timer) {
				m = new IRCMessage();
				m->command = "USER";
				m->numParams = 4;
				m->params.push_back(name);
				m->params.push_back("0");
				m->params.push_back("*");
				m->params.push_back(versionInfo);
				sendQ->putMessage(m);
				timer = 30;
				state = 4; // wait for login message
			}
			break;

		case 3:
			if (0 == timer) {
				chooseNewNick();
				m = new IRCMessage();
				m->command = "NICK";
				m->numParams = 1;
				m->params.push_back(currentNick);
				sendQ->putMessage(m);
				timer = 10;  // wait for possible nick collision message
				state = 2;
			}
			break;

		case 4:
			if (0 == timer)	// no login message received -> disconnect
				return false;
			break;

		case 5:
			m = new IRCMessage();
			m->command = "JOIN";
			m->numParams = 1;
			m->params.push_back(channel);
			sendQ->putMessage(m);
			timer = 30;
			state = 6; // wait for join message
			break;

		case 6:
			if (0 == timer) // no join message received -> disconnect
				return false;
			break;

		case 7:
			if (0 == debugChannel.size())
				return false; // this state cannot be processed if there is no debug_channel
			m = new IRCMessage();
			m->command = "JOIN";
			m->numParams = 1;
			m->params.push_back(debugChannel);
			sendQ->putMessage(m);
			timer = 30;
			state = 8; // wait for join message
			break;

		case 8:
			if (0 == timer)	// no join message received -> disconnect
				return false;
			break;

		case 10:
			m = new IRCMessage();
			m->command = "WHO";
			m->numParams = 2;
			m->params.push_back(channel);
			m->params.push_back("*");
			sendQ->putMessage(m);
			timer = pingTimer;
			state = 11; // wait for timer and then send ping
			if (app)
				app->setSendQ(sendQ);  // this switches the application on
			break;

		case 11:
			if (0 == timer) {
				m = new IRCMessage();
				m->command = "PING";
				m->numParams = 1;
				m->params.push_back(currentNick);
				sendQ->putMessage(m);
				timer = pingTimer;
				state = 12; // wait for pong
			}
			break;

		case 12:
			if (0 == timer)	// no pong message received -> disconnect
				return false;
			break;
	}
	return true;
}


