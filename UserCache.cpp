/*
 *   Copyright (C) 2010 by Jonathan Naylor G4KLX
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

#include "UserCache.h"

CUserCache::CUserCache()
{
}

CUserCache::~CUserCache()
{
	for (auto it = m_cache.begin(); it != m_cache.end(); it++)
		delete it->second;
	m_cache.clear();
}

CUserRecord *CUserCache::find(const std::string& user)
{
	auto user_record = m_cache.find(user);
	if (user_record == m_cache.end())
		return NULL;
	else
		return user_record->second;
}

void CUserCache::update(const std::string& user, const std::string& repeater, const std::string& timestamp)
{
	CUserRecord *rec = NULL;
	auto user_record = m_cache.find(user);
	if (user_record != m_cache.end())
		rec = user_record->second;

	if (rec == NULL)
		// A brand new record is needed
		m_cache[user] = new CUserRecord(user, repeater, timestamp);
	else if (timestamp.compare(rec->getTimeStamp()) > 0) {
		// Update an existing record, but only if the received timestamp is newer
		rec->setRepeater(repeater);
		rec->setTimestamp(timestamp);
	} else if (timestamp.compare(rec->getTimeStamp()) < 0)
		printf("Tried to update user %s, but the timestamp was stale, record: %s, new: %s\n", user.c_str(), rec->getTimeStamp().c_str(), timestamp.c_str());
}

unsigned int CUserCache::getCount() const
{
	return m_cache.size();
}
