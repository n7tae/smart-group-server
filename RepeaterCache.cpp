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

#include "RepeaterCache.h"

CRepeaterCache::CRepeaterCache()
{
}

CRepeaterCache::~CRepeaterCache()
{
	for (auto it = m_cache.begin(); it != m_cache.end(); it++)
		delete it->second;
	m_cache.clear();
}

CRepeaterRecord *CRepeaterCache::find(const std::string &repeater)
{
	auto repeater_record = m_cache.find(repeater);
	if (repeater_record == m_cache.end())
		return NULL;
	else
		return repeater_record->second;
}

void CRepeaterCache::update(const std::string &repeater, const std::string &gateway)
{
	CRepeaterRecord *rec = NULL;
	auto repeater_record = m_cache.find(repeater);
	if (repeater_record != m_cache.end())
		rec = repeater_record->second;

	if (rec == NULL)
		m_cache[repeater] = new CRepeaterRecord(repeater, gateway);
	else
		rec->setGateway(gateway);
}

unsigned int CRepeaterCache::getCount() const
{
	return m_cache.size();
}
