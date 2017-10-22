// Copyright (c) 2017 by Thomas A. Early N7TAE

#include "RepeaterCache.h"

CRepeaterCache::CRepeaterCache()
{
}

CRepeaterCache::~CRepeaterCache()
{
	for (std::unordered_map<std::string, CRepeaterRecord *>::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
		delete it->second;
}

CRepeaterRecord* CRepeaterCache::find(const std::string& repeater)
{
	return m_cache[repeater];
}

void CRepeaterCache::update(const std::string& repeater, const std::string& gateway)
{
	CRepeaterRecord* rec = m_cache[repeater];

	if (rec == NULL)
		// A brand new record is needed
		m_cache[repeater] = new CRepeaterRecord(repeater, gateway);
	else
		// Update an existing record
		rec->setGateway(gateway);
}

unsigned int CRepeaterCache::getCount() const
{
	return m_cache.size();
}
