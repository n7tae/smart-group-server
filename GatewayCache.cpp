// Copyright (c) 2017 by Thomas A. Early N7TAE

#include "GatewayCache.h"

CGatewayCache::CGatewayCache()
{
}

CGatewayCache::~CGatewayCache()
{
	for (std::unordered_map<std::string, CGatewayRecord *>::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
		delete it->second;
}

CGatewayRecord* CGatewayCache::find(const std::string& gateway)
{
	return m_cache[gateway];
}

void CGatewayCache::update(const std::string& gateway, const std::string& address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock)
{
	CGatewayRecord* rec = m_cache[gateway];

	in_addr addr_in;
	addr_in.s_addr = ::inet_addr(address.c_str());

	if (rec == NULL)
		// A brand new record is needed
		m_cache[gateway] = new CGatewayRecord(gateway, addr_in, protocol, addrLock, protoLock);
	else
		// Update an existing record
		rec->setData(addr_in, protocol, addrLock, protoLock);
}

unsigned int CGatewayCache::getCount() const
{
	return m_cache.size();
}
