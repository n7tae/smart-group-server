/*
 *   Copyright (C) 2010-2014 by Jonathan Naylor G4KLX
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
#include <cstring>
#include <cmath>

#include "APRSWriter.h"
#include "Utils.h"

CAPRSEntry::CAPRSEntry(const std::string& callsign, const std::string& band, double frequency, double offset, double range, double latitude, double longitude, double agl) :
m_callsign(callsign),
m_band(band),
m_frequency(frequency),
m_offset(offset),
m_range(range),
m_latitude(latitude),
m_longitude(longitude),
m_agl(agl),
m_timer(1000U, 10U),
m_first(true),
m_collector(NULL)
{
	CUtils::Trim(m_callsign);

	m_collector = new CAPRSCollector;
}

CAPRSEntry::~CAPRSEntry()
{
	delete m_collector;
}

std::string CAPRSEntry::getCallsign() const
{
	return m_callsign;
}

std::string CAPRSEntry::getBand() const
{
	return m_band;
}

double CAPRSEntry::getFrequency() const
{
	return m_frequency;
}

double CAPRSEntry::getOffset() const
{
	return m_offset;
}

double CAPRSEntry::getRange() const
{
	return m_range;
}

double CAPRSEntry::getLatitude() const
{
	return m_latitude;
}

double CAPRSEntry::getLongitude() const
{
	return m_longitude;
}

double CAPRSEntry::getAGL() const
{
	return m_agl;
}

CAPRSCollector* CAPRSEntry::getCollector() const
{
	return m_collector;
}

void CAPRSEntry::reset()
{
	m_first = true;
	m_timer.stop();
	m_collector->reset();
}

void CAPRSEntry::clock(unsigned int ms)
{
	m_timer.clock(ms);
}

bool CAPRSEntry::isOK()
{
	if (m_first) {
		m_first = false;
		m_timer.start();
		return true;
	}

	if (m_timer.hasExpired()) {
		m_timer.start();
		return true;
	} else {
		m_timer.start();
		return false;
	}
}

CAPRSWriter::CAPRSWriter(const std::string& hostname, unsigned int port, const std::string& gateway, const std::string& address) :
m_thread(NULL),
m_enabled(false),
m_idTimer(1000U, 20U * 60U),		// 20 minutes
m_gateway(),
m_array()
{
	assert(hostname.size());
	assert(port > 0U);
	assert(gateway.size());

	m_thread = new CAPRSWriterThread(gateway, address, hostname, port);

	m_gateway = gateway;
	m_gateway.resize(LONG_CALLSIGN_LENGTH - 1U);
	CUtils::Trim(m_gateway);
}

CAPRSWriter::~CAPRSWriter()
{
	for (std::map<std::string, CAPRSEntry *>::iterator it = m_array.begin(); it != m_array.end(); ++it)
		delete it->second;

	m_array.clear();
}

void CAPRSWriter::setPort(const std::string& callsign, const std::string& band, double frequency, double offset, double range, double latitude, double longitude, double agl)
{
	std::string temp = callsign;
	temp.resize(LONG_CALLSIGN_LENGTH - 1U, ' ');
	temp.append(band);

	m_array[temp] = new CAPRSEntry(callsign, band, frequency, offset, range, latitude, longitude, agl);
}

bool CAPRSWriter::open()
{
	return m_thread->start();
}

void CAPRSWriter::writeData(const std::string& callsign, const CAMBEData& data)
{
	if (data.isEnd())
		return;

	CAPRSEntry* entry = m_array[callsign];
	if (entry == NULL) {
		CUtils::lprint("Cannot find the callsign \"%s\" in the APRS array", callsign.c_str());
		return;
	}

	CAPRSCollector* collector = entry->getCollector();

	if (data.isSync()) {
		collector->sync();
		return;
	}

	unsigned char buffer[400U];
	data.getData(buffer, DV_FRAME_MAX_LENGTH_BYTES);

	bool complete = collector->writeData(buffer + VOICE_FRAME_LENGTH_BYTES);
	if (!complete)
		return;

	if (!m_enabled) {
		collector->reset();
		return;
	}

	if (!m_thread->isConnected()) {
		collector->reset();
		return;
	}

	// Check the transmission timer
	bool ok = entry->isOK();
	if (!ok) {
		collector->reset();
		return;
	}

	unsigned int length = collector->getData(buffer, 400U);
	std::string text((char*)buffer);
	text.resize(length, ' ');

	std::string::size_type n = text.find(':');
	if (n == std::string::npos) {
		collector->reset();
		return;
	}

	std::string header = text.substr(0, n);
	std::string body   = text.substr(n + 1);

	// If we already have a q-construct, don't send it on
	n = header.find('q');
	if (n != std::string::npos)
		return;

	// Remove the trailing \r
	n = body.find('\r');
	if (n != std::string::npos)
		body = body.substr(0, n);


	char ascii[500U];
	snprintf(ascii, 500, "%s,qAR,%s-%s:%s", header.c_str(), entry->getCallsign().c_str(), entry->getBand().c_str(), body.c_str());

	m_thread->write(ascii);

	collector->reset();
}

void CAPRSWriter::reset(const std::string& callsign)
{
	CAPRSEntry* entry = m_array[callsign];
	if (entry == NULL) {
		CUtils::lprint("Cannot find the callsign \"%s\" in the APRS array", callsign.c_str());
		return;
	}

	entry->reset();
}

void CAPRSWriter::setEnabled(bool enabled)
{
	m_enabled = enabled;

	if (m_enabled) {
		sendIdFrames();
		m_idTimer.start();
	}
}

void CAPRSWriter::clock(unsigned int ms)
{
	m_idTimer.clock(ms);

	if (m_idTimer.hasExpired()) {
		sendIdFrames();
		m_idTimer.start();
	}

	for (std::map<std::string, CAPRSEntry *>::iterator it = m_array.begin(); it != m_array.end(); ++it)
		it->second->clock(ms);
}

bool CAPRSWriter::isConnected() const
{
	return m_thread->isConnected();
}

void CAPRSWriter::close()
{
	m_thread->stop();
}

void CAPRSWriter::sendIdFrames()
{
	if (!m_thread->isConnected())
		return;

	time_t now;
	::time(&now);
	struct tm* tm = ::gmtime(&now);

	for (std::map<std::string, CAPRSEntry *>::iterator it = m_array.begin(); it != m_array.end(); ++it) {
		CAPRSEntry* entry = it->second;
		if (entry == NULL)
			continue;

		// Default values aren't passed on
		if (entry->getLatitude() == 0.0 && entry->getLongitude() == 0.0)
			continue;

		char buffer[512];
		if (entry->getBand().size() > 1) {
			if (entry->getFrequency() != 0.0)
				sprintf(buffer, "Data %.5lfMHz", entry->getFrequency());
			else
				strcpy(buffer, "Data");
		} else {
			if (entry->getFrequency() != 0.0)
				sprintf(buffer, "Voice %.5lfMHz %c%.4lfMHz", entry->getFrequency(), entry->getOffset() < 0.0 ? '-' : '+', fabs(entry->getOffset()));
			else
				strcpy(buffer, "Voice");
		}
		std::string desc(buffer);

		std::string band;
		if (entry->getFrequency() >= 1200.0)
			band = "1.2";
		else if (entry->getFrequency() >= 420.0)
			band = "440";
		else if (entry->getFrequency() >= 144.0)
			band = "2m";
		else if (entry->getFrequency() >= 50.0)
			band = "6m";
		else if (entry->getFrequency() >= 28.0)
			band = "10m";

		double tempLat  = fabs(entry->getLatitude());
		double tempLong = fabs(entry->getLongitude());

		double latitude  = floor(tempLat);
		double longitude = floor(tempLong);

		latitude  = (tempLat  - latitude)  * 60.0 + latitude  * 100.0;
		longitude = (tempLong - longitude) * 60.0 + longitude * 100.0;

		if (latitude >= 1000.0F)
			sprintf(buffer, "%.2lf", latitude);
		else if (latitude >= 100.0F)
			sprintf(buffer, "0%.2lf", latitude);
		else if (latitude >= 10.0F)
			sprintf(buffer, "00%.2lf", latitude);
		else
			sprintf(buffer, "000%.2lf", latitude);
		std::string lat(buffer);

		if (longitude >= 10000.0F)
			sprintf(buffer, "%.2lf", longitude);
		else if (longitude >= 1000.0F)
			sprintf(buffer, "0%.2lf", longitude);
		else if (longitude >= 100.0F)
			sprintf(buffer, "00%.2lf", longitude);
		else if (longitude >= 10.0F)
			sprintf(buffer, "000%.2lf", longitude);
		else
			sprintf(buffer, "0000%.2lf", longitude);
		std::string lon(buffer);

		// Convert commas to periods in the latitude and longitude
		//lat.Replace(wxT(","), wxT("."));
		//lon.Replace(wxT(","), wxT("."));

		sprintf(buffer, "%s-S>APDG01,TCPIP*,qAC,%s-GS:;%-7s%-2s*%02d%02d%02dz%s%cD%s%caRNG%04.0lf %s %s",
			m_gateway.c_str(), m_gateway.c_str(), entry->getCallsign().c_str(), entry->getBand().c_str(),
			tm->tm_mday, tm->tm_hour, tm->tm_min,
			lat.c_str(), (entry->getLatitude() < 0.0F)  ? 'S' : 'N',
			lon.c_str(), (entry->getLongitude() < 0.0F) ? 'W' : 'E',
			entry->getRange() * 0.6214, band.c_str(), desc.c_str());

		m_thread->write(buffer);

		if (entry->getBand().size() == 1U) {
			sprintf(buffer, "%s-%s>APDG02,TCPIP*,qAC,%s-%sS:!%s%cD%s%c&RNG%04.0lf %s %s",
				entry->getCallsign().c_str(), entry->getBand().c_str(), entry->getCallsign().c_str(), entry->getBand().c_str(),
				lat.c_str(), (entry->getLatitude() < 0.0F)  ? 'S' : 'N',
				lon.c_str(), (entry->getLongitude() < 0.0F) ? 'W' : 'E',
				entry->getRange() * 0.6214, band.c_str(), desc.c_str());

			m_thread->write(buffer);
		}
	}

	m_idTimer.start();
}
