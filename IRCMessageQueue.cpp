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

#include "IRCMessageQueue.h"


IRCMessageQueue::IRCMessageQueue()
{
  eof = false;
  first = NULL;
  last = NULL;

}

IRCMessageQueue::~IRCMessageQueue()
{
  while (messageAvailable()) {
    IRCMessage * m = getMessage();

    delete m;
  }
}


bool IRCMessageQueue::isEOF()
{
  return eof;
}


void IRCMessageQueue::signalEOF()
{
  eof = true;
}


bool IRCMessageQueue::messageAvailable()
{
  accessMutex.lock();

  IRCMessageQueueItem *mqi = first;

  accessMutex.unlock();

  return (mqi != NULL);
}


IRCMessage * IRCMessageQueue::peekFirst()
{
  accessMutex.lock();

  IRCMessageQueueItem * k = first;

  accessMutex.unlock();

  if ( k == NULL ) {
    return NULL;
  }

  return k->msg;
}


IRCMessage * IRCMessageQueue::getMessage()
{
  accessMutex.lock();

  IRCMessageQueueItem * k;

  if (first == NULL) {
	  accessMutex.unlock();
	  return NULL;
  }

  k = first;

  first = k -> next;

  if (k -> next == NULL) {
	  last = NULL;
  } else {
	  k -> next -> prev = NULL;
  }


  IRCMessage * msg = k -> msg;

  delete k;

  accessMutex.unlock();

  return msg;
}


void IRCMessageQueue::putMessage( IRCMessage * m )
{
  accessMutex.lock();

  IRCMessageQueueItem * k = new IRCMessageQueueItem(m);

  k -> prev = last;
  k -> next = NULL;

  if (last == NULL) {
	  first = k;
  } else {
	  last -> next = k;
  }

  last = k;

  accessMutex.unlock();
}




