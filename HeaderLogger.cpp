/*
 *   Copyright (C) 2010,2011,2012,2014 by Jonathan Naylor G4KLX
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

#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "HeaderLogger.h"
#include "Utils.h"
#include "Defs.h"

CHeaderLogger::CHeaderLogger(const std::string& dir, const std::string& name) :
m_dir(dir),
m_name(name),
m_file()
{
}

CHeaderLogger::~CHeaderLogger()
{
}

bool CHeaderLogger::open()
{
	std::string fullName("Headers");

	if (m_name.size()) {
		fullName.append("_");
		fullName.append(m_name);
	}

	std::string fileName(m_dir);
	fileName += fullName + ".log";

	m_file = fopen(fileName.c_str(), "a+t");
	if (!m_file) {
		CUtils::lprint("Cannot open %s file for appending", fileName.c_str());
		return false;
	}

	return true;
}

void CHeaderLogger::write(const char* type, const CHeaderData& header)
{
	assert(type != NULL);

	time_t timeNow = ::time(NULL);
	struct tm* tm = ::gmtime(&timeNow);

	char* t = ::inet_ntoa(header.getYourAddress());

	fprintf(m_file, "%04d-%02d-%02d %02d:%02d:%02d: %s header - My: %s/%s  Your: %s  Rpt1: %s  Rpt2: %s  Flags: %02X %02X %02X (%s:%u)\n",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, type,
		header.getMyCall1().c_str(), header.getMyCall2().c_str(), header.getYourCall().c_str(),
		header.getRptCall1().c_str(), header.getRptCall2().c_str(), header.getFlag1(), header.getFlag2(),
		header.getFlag3(), t, header.getYourPort());

	fflush(m_file);
}

void CHeaderLogger::write(const char* type, const CDDData& data)
{
	assert(type != NULL);

	time_t timeNow = ::time(NULL);
	struct tm* tm = ::gmtime(&timeNow);

	fprintf(m_file, "%04d-%02d-%02d %02d:%02d:%02d: %s header - My: %s/%s  Your: %s  Rpt1: %s  Rpt2: %s  Flags: %02X %02X %02X\n",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, type,
		data.getMyCall1().c_str(), data.getMyCall2().c_str(), data.getYourCall().c_str(),
		data.getRptCall1().c_str(), data.getRptCall2().c_str(), data.getFlag1(), data.getFlag2(),
		data.getFlag3());

	fflush(m_file);
}

void CHeaderLogger::close()
{
	fclose(m_file);
}
