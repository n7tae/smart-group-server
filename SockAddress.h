#pragma once

/*
 *   Copyright (C) 2019 by Thomas Early N7TAE
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

class CSockAddress
{
public:
	CSockAddress();
	CSockAddress(const struct sockaddr_storage &from);
	CSockAddress(const int family, const unsigned short port, const char *address);
	~CSockAddress() {}

	void Initialize(int family, uint16_t port = 0U, const char *address = NULL);
	CSockAddress &operator=(const CSockAddress &from);
	bool operator==(CSockAddress &from);
	bool AddressIsZero();
	void ClearAddress();
	const char *GetAddress();
	unsigned short GetPort();
	struct sockaddr *GetPointer();
	size_t GetSize();
	void Clear();

private:
	struct sockaddr_storage addr;
	char straddr[INET6_ADDRSTRLEN];
};
