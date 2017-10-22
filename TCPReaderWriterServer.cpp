/*
 *   Copyright (C) 2011,2014 by Jonathan Naylor G4KLX
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

#include <cerrno>
#include <cassert>
#include <cstring>
#include <unistd.h>

#include "TCPReaderWriterServer.h"
#include "Utils.h"

CTCPReaderWriterServer::CTCPReaderWriterServer(const std::string& address, unsigned int port) :
m_address(address)
{
	assert(port > 0U);
	m_fd = -1;
	m_port = port;
	m_client = NULL;
	m_stopped = false;
}

CTCPReaderWriterServer::~CTCPReaderWriterServer()
{
}

bool CTCPReaderWriterServer::start()
{
	bool ret = open();
	if (!ret) {
		close();
		return false;
	}

	m_future = std::async(std::launch::async, &CTCPReaderWriterServer::Entry, this);

	return true;
}

int CTCPReaderWriterServer::read(unsigned char* buffer, unsigned int length, unsigned int secs)
{
	assert(buffer != NULL);
	assert(length > 0U);

	if (m_client != NULL) {
		int ret = m_client->read(buffer, length, secs);
		if (ret < 0) {
			CUtils::lprint("Lost TCP connection to port %u", m_port);

			m_client->close();
			delete m_client;
			m_client = NULL;

			open();

			return 0;
		}

		return ret;
	}

	return 0;
}

bool CTCPReaderWriterServer::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(length > 0U);

	if (m_client != NULL) {
		bool ret = m_client->write(buffer, length);
		if (!ret) {
			CUtils::lprint("Lost TCP connection to port %u", m_port);

			m_client->close();
			delete m_client;
			m_client = NULL;

			open();

			return false;
		}

		return true;
	}

	return true;
}

bool CTCPReaderWriterServer::Entry()
{
	try {
		while (! m_stopped) {
			int ret = accept();
			switch (ret) {
				case -2:
					break;
				case -1:
					break;
				default:
					CUtils::lprint("Incoming TCP connection to port %u", m_port);
					m_client = new CTCPReaderWriterClient(ret);
					close();
					break;
			}

			usleep(1000000);
		}

		if (m_client != NULL) {
			m_client->close();
			delete m_client;
		}

		close();
	}
	catch (std::exception& e) {
		std::string message(e.what());
		CUtils::lprint("Exception raised in the TCP Reader-Writer Server thread - \"%s\"", message.c_str());
	}
	catch (...) {
		CUtils::lprint("Unknown exception raised in the TCP Reader-Writer Server thread");
	}

	return NULL;
}

void CTCPReaderWriterServer::stop()
{
	m_stopped = true;

	m_future.get();
}

bool CTCPReaderWriterServer::open()
{
	m_fd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd < 0) {
		CUtils::lprint("Cannot create the TCP server socket, err=%d", errno);
		return false;
	}

	struct sockaddr_in addr;
	::memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(m_port);
	if (0==m_address.size())
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		addr.sin_addr = lookup(m_address);

	if (addr.sin_addr.s_addr == INADDR_NONE) {
		CUtils::lprint("The address is invalid - %s", m_address.c_str());
		close();
		return false;
	}

	int reuse = 1;
	if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1) {
		CUtils::lprint("Cannot set the TCP server socket option, err=%d", errno);
		close();
		return false;
	}

	if (::bind(m_fd, (sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
		CUtils::lprint("Cannot bind the TCP server address, err=%d", errno);
		close();
		return false;
	}

	::listen(m_fd, 5);

	return true;
}

int CTCPReaderWriterServer::accept()
{
	if (m_fd == -1)
		return -1;

	// Check that the accept() won't block
	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(m_fd, &readFds);

	// Return after timeout
	timeval tv;
	tv.tv_sec  = 0L;
	tv.tv_usec = 0L;

	int ret = ::select(m_fd + 1, &readFds, NULL, NULL, &tv);
	if (ret < 0) {
		CUtils::lprint("Error returned from TCP server select, err=%d", errno);
		return -2;
	}

	if (!FD_ISSET(m_fd, &readFds))
		return -1;

	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);

	ret = ::accept(m_fd, (sockaddr*)&addr, &len);
	if (ret < 0) {
		CUtils::lprint("Error returned from TCP server accept, err=%d", errno);
	}

	return ret;
}

void CTCPReaderWriterServer::close()
{
	if (m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
}

in_addr CTCPReaderWriterServer::lookup(const std::string& hostname) const
{
	in_addr addr;
	in_addr_t address = ::inet_addr(hostname.c_str());
	if (address != in_addr_t(-1)) {
		addr.s_addr = address;
		return addr;
	}

	struct hostent* hp = ::gethostbyname(hostname.c_str());
	if (hp != NULL) {
		::memcpy(&addr, hp->h_addr_list[0], hp->h_length);
		return addr;
	}

	CUtils::lprint("Cannot find %s", hostname.c_str());

	addr.s_addr = INADDR_NONE;
	return addr;
}
