// Copyright (c) 2017 by Thomas A. Early

#include "UserCache.h"

CUserCache::CUserCache()
{
}

CUserCache::~CUserCache()
{
	for (std::unordered_map<std::string, CUserRecord *>::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
		delete it->second;
	m_cache.clear();
}

CUserRecord* CUserCache::find(const std::string& user)
{
	return m_cache[user];
}

void CUserCache::update(const std::string& user, const std::string& repeater, const std::string& timestamp)
{
	CUserRecord* rec = m_cache[user];

	if (rec == NULL)
		// A brand new record is needed
		m_cache[user] = new CUserRecord(user, repeater, timestamp);
	else if(timestamp.compare(rec->getTimeStamp()) > 0) {
		// Update an existing record, but only if the received timestamp is newer
		rec->setRepeater(repeater);
		rec->setTimestamp(timestamp);
	}
}

unsigned int CUserCache::getCount() const
{
	return m_cache.size();
}
