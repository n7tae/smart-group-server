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

#include "IRCMessage.h"



IRCMessage::IRCMessage ()
{
  numParams = 0;
  prefixParsed = false;
}

IRCMessage::IRCMessage ( const std::string& toNick, const std::string& msg )
{
  command = "PRIVMSG";
  numParams = 2;
  params.push_back( toNick );
  params.push_back( msg );
  prefixParsed = false;
}

IRCMessage::IRCMessage ( const std::string& cmd )
{
  command = cmd;
  numParams = 0;
  prefixParsed = false;
}

IRCMessage::~IRCMessage()
{
}


void IRCMessage::addParam( const std::string& p )
{
  params.push_back( p );
  numParams = params.size();
}

int IRCMessage::getParamCount()
{
  return params.size();
}

std::string IRCMessage::getParam( int pos )
{
  return params[pos];
}

std::string IRCMessage::getCommand()
{
  return command;
}

	
void IRCMessage::parsePrefix()
{
  unsigned int i;

  for (i=0; i < 3; i++) {
    prefixComponents.push_back("");
  }

  int state = 0;
  
  for (i=0; i < prefix.size(); i++) {
    char c = prefix.at(i);
			
    switch (c)
    {
    case '!': 
	    state = 1; // next is name
	    break;
	    
    case '@':
	    state = 2; // next is host
	    break;
	    
    default:
	    prefixComponents[state].push_back(c);
	    break;
    }
  }

  prefixParsed = true;
}

std::string& IRCMessage::getPrefixNick()
{
  if (!prefixParsed)
  {
    parsePrefix();
  }
  
  return prefixComponents[0];
}

std::string& IRCMessage::getPrefixName()
{
  if (!prefixParsed)
  {
    parsePrefix();
  }
  
  return prefixComponents[1];
}

std::string& IRCMessage::getPrefixHost()
{
  if (!prefixParsed)
  {
    parsePrefix();
  }
  
  return prefixComponents[2];
}

void IRCMessage::composeMessage ( std::string& output )
{
  std::string o;

  if (prefix.size() > 0)
  {
    o = std::string(":") + prefix + std::string(" ");
  }

  o.append(command);

  for (int i=0; i < numParams; i++)
  {
    if (i == (numParams - 1))
    {
      o.append(std::string(" :") + params[i]);
    }
    else
    {
      o.append(std::string(" ") + params[i]);
    }
  }

  o.append("\r\n");

  output = o;
}

