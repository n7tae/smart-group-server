/*
 *   Copyright (C) 2006-2014 by Jonathan Naylor G4KLX
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
#include <cstring>
#include <string.h>
#include "UDPReaderWriter.h"

CUDPReaderWriter::CUDPReaderWriter(int family, unsigned short port) :
m_family(family),
m_port(port),
m_fd(-1)
{
}

CUDPReaderWriter::~CUDPReaderWriter()
{
}

bool CUDPReaderWriter::Open()
{
	m_fd = socket(m_family, SOCK_DGRAM, 0);
	if (m_fd < 0) {
		printf("Cannot create the UDP socket, err: %s\n", strerror(errno));
		return false;
	}

	if (m_port > 0U && (m_family==AF_INET || m_family==AF_INET6)) {
		m_addr.Initialize(m_family, m_port, "ANY_ADDRESS");

		// int reuse = 1;
		// if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1) {
		//  	printf("Cannot set the UDP socket option (port: %u), err: %s\n", m_port, strerror(errno));
		//  	return false;
		// }

		if (bind(m_fd, m_addr.GetPointer(), sizeof(struct sockaddr_storage)) == -1) {
			printf("Cannot bind the UDP address (port: %u), err: %s\n", m_port, strerror(errno));
			return false;
		}
	} else
		return false;
	

	return true;
}

int CUDPReaderWriter::Read(unsigned char *buffer, unsigned int length, CSockAddress &addr)
{
	// Check that the readfrom() won't block
	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(m_fd, &readFds);

	// Return immediately
	timeval tv;
	tv.tv_sec  = 0L;
	tv.tv_usec = 0L;

	int ret = select(m_fd + 1, &readFds, NULL, NULL, &tv);
	if (ret < 0) {
		printf("Error returned from UDP select (port: %u), err: %s\n", m_port, strerror(errno));
		return -1;
	}

	if (0 == ret) 
		return 0;

	socklen_t size = sizeof(struct sockaddr_storage);

	ssize_t len = recvfrom(m_fd, buffer, length, 0, addr.GetPointer(), &size);
	if (len <= 0) {
		printf("Error returned from recvfrom (port: %u), err: %s\n", m_port, strerror(errno));
		return -1;
	}

	return len;
}

bool CUDPReaderWriter::Write(const unsigned char* buffer, unsigned int length, CSockAddress &addr)
{
	unsigned int count = 0;
	while (count < length) {
	 	ssize_t ret = ::sendto(m_fd, buffer+count, length-count, 0, addr.GetPointer(), sizeof(struct sockaddr_storage));
		if (ret < 0) {
			fprintf(stderr, "sendto ERROR: This is an %s socket, trying to write to [%s]:%u\n", (AF_INET==m_family) ? "IPv4" : (AF_INET6==m_family) ? "IPv6" : "UNKNOWN", addr.GetAddress(), addr.GetPort());
			//fprintf(stderr, "Error returned from sendto (port: %u), err: %s\n", m_port, strerror(errno));
			return false;
		}

		count += ret;
	}

	return true;
}

void CUDPReaderWriter::Close()
{
	if (m_fd != -1) {
		close(m_fd);
		m_fd = -1;
	}
}

unsigned int CUDPReaderWriter::getPort() const
{
	return m_port;
}
