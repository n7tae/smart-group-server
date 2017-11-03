/*
CIRCDDB - ircDDB client library in C++

Copyright (C) 2010   Michael Dirska, DL1BFF (dl1bff@mdx.de)
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

#pragma once

#include <string>
#include <vector>

#include "IRCMessageQueue.h"
#include "IRCApplication.h"


class IRCProtocol
{
  public:
    IRCProtocol(IRCApplication * app, const std::string& callsign, const std::string& password, const std::string& channel, const std::string& versionInfo );
    ~IRCProtocol();

    void setNetworkReady(bool state);

    bool processQueues(IRCMessageQueue *recvQ, IRCMessageQueue *sendQ);

  private:
    void chooseNewNick();

    std::vector<std::string> nicks;
    std::string password;
    std::string channel;
    std::string name;
    std::string currentNick;
    std::string versionInfo;

    int state;
    int timer;
    int pingTimer;

    std::string debugChannel;

    IRCApplication *app;

};

