/*
 *   Copyright (C) 2006-2014 by Jonathan Naylor G4KLX
 *   Copyright (c) 2020 by Thomas A. Early N7TAE
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
m_fd(-1)
{
	m_addr.Initialize(family, port);
}

CUDPReaderWriter::~CUDPReaderWriter()
{
}

bool CUDPReaderWriter::Open()
{
	m_fd = socket(m_addr.GetFamily(), SOCK_DGRAM, 0);
	if (m_fd < 0) {
		fprintf(stderr, "Cannot create the UDP socket, err: %s\n", strerror(errno));
		return false;
	}

	if (m_addr.GetFamily()==AF_INET || m_addr.GetFamily()==AF_INET6) {

		// int reuse = 1;
		// if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1) {
		//  	printf("Cannot set the UDP socket option (port: %u), err: %s\n", m_port, strerror(errno));
		//  	return false;
		// }

		if (bind(m_fd, m_addr.GetCPointer(), m_addr.GetSize())) {
			fprintf(stderr, "CUPDReaderWriter bind error [%s]:%u %s\n", m_addr.GetAddress(), m_addr.GetPort(), strerror(errno));
			Close();
			return false;
		}

		if (0 == m_addr.GetPort()) {	// get the assigned port for an ephemeral port request
			CSockAddress a;
			socklen_t len = sizeof(struct sockaddr_storage);
			if (getsockname(m_fd, a.GetPointer(), &len)) {
				fprintf(stderr, "CUPDReaderWriter getsockname error [%s]:%u %s\n", m_addr.GetAddress(), m_addr.GetPort(), strerror(errno));
				Close();
				return false;
			}
			if (a != m_addr)
				printf("CUPDReaderWriter getsockname didn't return the same address as set: returned %s, should have been %s\n", a.GetAddress(), m_addr.GetAddress());

			m_addr.SetPort(a.GetPort());
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
		fprintf(stderr, "CUPDReaderWriter select error [%s]:%u: %s\n", m_addr.GetAddress(), m_addr.GetPort(), strerror(errno));
		return -1;
	}

	if (0 == ret)
		return 0;

	socklen_t size = sizeof(struct sockaddr_storage);

	ssize_t len = recvfrom(m_fd, buffer, length, 0, addr.GetPointer(), &size);
	if (len <= 0) {
		fprintf(stderr, "CUPDReaderWriter recvfrom error [%s]:%u %s\n", m_addr.GetAddress(), m_addr.GetPort(), strerror(errno));
		return -1;
	}

	return len;
}

bool CUDPReaderWriter::Write(const unsigned char* buffer, unsigned int length, CSockAddress &addr)
{
	unsigned int count = 0;
	while (count < length) {
	 	ssize_t ret = sendto(m_fd, buffer+count, length-count, 0, addr.GetCPointer(), addr.GetSize());
		if (ret < 0) {
			fprintf(stderr, "CUPDReaderWriter sendto error [%s]:%u: %s\n", m_addr.GetAddress(), m_addr.GetPort(), strerror(errno));
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
	return m_addr.GetPort();
}
