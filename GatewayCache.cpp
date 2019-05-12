/*
 *   Copyright (C) 2010,2011,2012 by Jonathan Naylor G4KLX
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

#include "GatewayCache.h"

CGatewayCache::CGatewayCache()
{
}

CGatewayCache::~CGatewayCache()
{
	for (auto it = m_cache.begin(); it != m_cache.end(); it++)
		delete it->second;
	m_cache.clear();
}

CGatewayRecord *CGatewayCache::find(const std::string &gateway)
{
	auto gateway_record = m_cache.find(gateway);
	if (gateway_record == m_cache.end())
		return NULL;
	else
		return gateway_record->second;
}

void CGatewayCache::update(const std::string &gateway, const std::string &address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock)
{
	CGatewayRecord *rec = NULL;
	auto gateway_record = m_cache.find(gateway);
	if (gateway_record != m_cache.end())
		rec = gateway_record->second;

	if (rec == NULL)
		// A brand new record is needed
		m_cache[gateway] = new CGatewayRecord(gateway, address, protocol, addrLock, protoLock);
	else
		// Update an existing record
		rec->setData(address, protocol, addrLock, protoLock);
}

unsigned int CGatewayCache::getCount() const
{
	return m_cache.size();
}
