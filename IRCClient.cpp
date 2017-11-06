/*

CIRCDDB - ircDDB client library in C++

Copyright (C) 2010-2011   Michael Dirska, DL1BFF (dl1bff@mdx.de)
Copyright (C) 2012        Jonathan Naylor, G4KLX
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>

#include "IRCClient.h"
#include "Utils.h"

IRCClient::IRCClient(IRCApplication *app, const std::string& update_channel, const std::string& hostName, unsigned int port, const std::string& callsign, const std::string& password,
    const std::string& versionInfo, const std::string& localAddr)
{
	CUtils::safeStringCopy(host_name, hostName.c_str(), sizeof host_name);

	this->callsign = callsign;
	CUtils::ToLower(this->callsign);
	this->port = port;
	this->password = password;

	this->app = app;

	if (0 == localAddr.size())
		CUtils::safeStringCopy(local_addr, "0.0.0.0", sizeof local_addr);
	else
		CUtils::safeStringCopy(local_addr, localAddr.c_str(), sizeof local_addr);

	proto = new IRCProtocol(app, this->callsign, password, update_channel, versionInfo);

	recvQ = NULL;
	sendQ = NULL;
	recv = NULL;
	 CUtils::lprint("IRCClient created");
}

IRCClient::~IRCClient()
{
	delete proto;
}

void IRCClient::startWork()
{
	terminateThread = false;

	m_future = std::async(std::launch::async, &IRCClient::Entry, this);
}

void IRCClient::stopWork()
{
	terminateThread = true;

	m_future.get();
}

int IRCClient::Entry ()
{
	int state = 0;
	int timer = 0;
	int sock = 0;
	unsigned int currentAddr = 0;
	unsigned int numAddr;
	const unsigned int MAXIPV4ADDR = 10;
	struct sockaddr_in addr[MAXIPV4ADDR];
	struct sockaddr_in myaddr;

	numAddr = 0;

	int result = CUtils::getAllIPV4Addresses(local_addr, 0, &numAddr, &myaddr, 1);

	if (result || (numAddr != 1)) {
		CUtils::lprint("IRCClient::Entry: local address not parseable, using 0.0.0.0");
		memset(&myaddr, 0x00, sizeof(struct sockaddr_in));
	}

	while (true) {
		if (timer > 0)
			timer--;

		switch (state) {
			case 0:
				if (terminateThread) {
					CUtils::lprint("IRCClient::Entry: thread terminated at state=%d", state);
					return 0;
				}

				if (timer == 0) {
					timer = 30;

					if (0 == CUtils::getAllIPV4Addresses(host_name, port, &numAddr, addr, MAXIPV4ADDR)) {
						CUtils::lprint("IRCClient::Entry: number of DNS entries %d", numAddr);
						if (numAddr > 0) {
							currentAddr = 0;
							state = 1;
							timer = 0;
						}
					}
				}
				break;

			case 1:
				if (terminateThread) {
					CUtils::lprint("IRCClient::Entry: thread terminated at state=%d", state);
					return 0;
				}

				if (timer == 0) {
					sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP);

					if (sock < 0) {
						CUtils::lprint("IRCClient::Entry: socket");
						timer = 30;
						state = 0;
					} else {
						if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
							CUtils::lprint("IRCClient::Entry: fcntl");
							close(sock);
							timer = 30;
							state = 0;
						} else {
							unsigned char * h = (unsigned char *) &(myaddr.sin_addr);
							int res;

							if (h[0] || h[1] || h[2] || h[3]) 
								CUtils::lprint("IRCClient::Entry: bind: local address %d.%d.%d.%d", h[0], h[1], h[2], h[3]);

							res = bind(sock, (struct sockaddr *) &myaddr, sizeof (struct sockaddr_in));

							if (res) {
								CUtils::lprint("IRCClient::Entry: bind");
								close(sock);
								state = 0;
								timer = 30;
								break;
							}

							h = (unsigned char *) &(addr[currentAddr].sin_addr);
							CUtils::lprint("IRCClient::Entry: trying to connect to %d.%d.%d.%d", h[0], h[1], h[2], h[3]);
							res = connect(sock, (struct sockaddr *) (addr + currentAddr), sizeof (struct sockaddr_in));
							if (res == 0) {
								CUtils::lprint("IRCClient::Entry: connected");
								state = 4;
							} else { 
								if (errno == EINPROGRESS) {
									CUtils::lprint("IRCClient::Entry: connect in progress");
									state = 3;
									timer = 10;  // 5 second timeout
								} else {
									CUtils::lprint("IRCClient::Entry: connect");
									close(sock);
									currentAddr++;
									if (currentAddr >= numAddr) {
										state = 0;
										timer = 30;
									} else {
										state = 1;
										timer = 4;
									}
								}
							}
						} // connect
					}
				}
				break;

			case 3:
				{
					struct timeval tv;
					tv.tv_sec = 0; 
					tv.tv_usec = 0; 

					fd_set myset;
					FD_ZERO(&myset);
					FD_SET(sock, &myset);

					int res = select(sock+1, NULL, &myset, NULL, &tv); 
					if (res < 0) {
						CUtils::lprint("IRCClient::Entry: select");
						close(sock);
						state = 0;
						timer = 30;
					} else if (res > 0) {	// connect is finished
						socklen_t val_len;
						int value;
						val_len = sizeof value;
						if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &value, &val_len) < 0) {
							CUtils::lprint("IRCClient::Entry: getsockopt");
							close(sock);
							state = 0;
							timer = 30;
						} else {
							if (value != 0) {
								CUtils::lprint("IRCClient::Entry: SO_ERROR=%d", value);
								close(sock);
								currentAddr ++;
								if (currentAddr >= numAddr) {
									state = 0;
									timer = 30;
								} else {
									state = 1;
									timer = 2;
								}
							} else {
								CUtils::lprint("IRCClient::Entry: connected2");
								state = 4;
							}
						}
					}
					else if (timer == 0) {  // select timeout and timer timeout
						CUtils::lprint("IRCClient::Entry: connect timeout");
						close(sock);
						currentAddr ++;
						if (currentAddr >= numAddr) {
							state = 0;
							timer = 30;
						} else {
							state = 1; // open new socket
							timer = 2;
						}
					}
				}
				break;

			case 4:
				{
					recvQ = new IRCMessageQueue();
					sendQ = new IRCMessageQueue();
					recv = new IRCReceiver(sock, recvQ);
					recv->startWork();
					proto->setNetworkReady(true);
					state = 5;
					timer = 0;
				}
				break;

			case 5:
				if (terminateThread)
					state = 6;
				else {
					if (recvQ -> isEOF()) {
						timer = 0;
						state = 6;
					} else if (proto -> processQueues(recvQ, sendQ) == false) {
						timer = 0;
						state = 6;
					}

					while (5==state && sendQ->messageAvailable()) {
						IRCMessage *m = sendQ -> getMessage();

						std::string out;
						m->composeMessage(out);

						char buf[200];
						CUtils::safeStringCopy(buf, out.c_str(), sizeof buf);
						int len = strlen(buf);

						if (buf[len - 1] == 10) { // is there a NL char at the end?
							int r = send(sock, buf, len, 0);
							if (r != len) {
								CUtils::lprint("IRCClient::Entry: short write %d < %d", r, len);
								timer = 0;
								state = 6;
							}
						} else {
							CUtils::lprint("IRCClient::Entry: no NL at end, len=%d", len);
							timer = 0;
							state = 6;
						}
						delete m;
					}
				}
				break;

			case 6:
				{
					if (app) {
						app->setSendQ(NULL);
						app->userListReset();
					}

					proto->setNetworkReady(false);
					recv->stopWork();

					sleep(2);

					delete recv;
					delete recvQ;
					delete sendQ;

					close(sock);

					if (terminateThread) {	// request to end the thread
						CUtils::lprint("IRCClient::Entry: thread terminated at state=%d", state);
						return 0;
					}
					timer = 30;
					state = 0;  // reconnect to IRC server
				}
				break;
		}
		usleep(500000);
	}
	return 0;
}





