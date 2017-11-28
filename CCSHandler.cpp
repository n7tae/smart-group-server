/*
 *   Copyright (C) 2013,2014 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017 by Thomas A. Early
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
#include <mutex>

#include "RepeaterHandler.h"
#include "CCSHandler.h"
#include "Utils.h"


CCCSHandler**    CCCSHandler::m_handlers = NULL;

unsigned int     CCCSHandler::m_count = 0U;

std::string      CCCSHandler::m_localAddress;

std::string      CCCSHandler::m_ccsHost;

std::map<std::string, std::string> CCCSHandler::m_cache;
std::mutex       CCCSHandler::m_mutex;

bool             CCCSHandler::m_stateChange = false;


void CCCSHandler::initialise(unsigned int count)
{
	assert(count > 0U);

	m_count = count;
	m_handlers = new CCCSHandler*[m_count];

	for (unsigned int i = 0U; i < m_count; i++)
		m_handlers[i] = NULL;
}

void CCCSHandler::setLocalAddress(const std::string& address)
{
	m_localAddress = address;
}

void CCCSHandler::setHost(const std::string& host)
{
	m_ccsHost = host;
}

void CCCSHandler::process()
{
	for (unsigned int i = 0U; i < m_count; i++) {
		if (m_handlers[i] != NULL)
			m_handlers[i]->processInt();
	}
}

void CCCSHandler::disconnect()
{
	for (unsigned int i = 0U; i < m_count; i++) {
		if (m_handlers[i] != NULL)
			m_handlers[i]->disconnectInt();
	}
}

void CCCSHandler::clock(unsigned int ms)
{
	for (unsigned int i = 0U; i < m_count; i++) {
		if (m_handlers[i] != NULL)
			m_handlers[i]->clockInt(ms);
	}
}

void CCCSHandler::getInfo(ICCSCallback* handler, CRemoteRepeaterData& data)
{
	assert(handler != NULL);

	for (unsigned int i = 0U; i < m_count; i++) {
		CCCSHandler* ccs = m_handlers[i];
		if (ccs != NULL && ccs->m_handler == handler && ccs->m_state == CS_ACTIVE)
			data.addLink(ccs->m_yourCall, PROTO_CCS, true, ccs->m_direction, false);
	}
}

std::string CCCSHandler::getIncoming(const std::string& callsign)
{
	std::string incoming;

	for (unsigned int i = 0U; i < m_count; i++) {
		CCCSHandler* handler = m_handlers[i];
		if (handler != NULL && handler->m_direction == DIR_INCOMING && handler->m_state == CS_ACTIVE && 0==handler->m_callsign.compare(callsign)) {
			incoming.append(handler->m_yourCall);
			incoming.append("  ");
		}
	}

	return incoming;
}

void CCCSHandler::finalise()
{
	for (unsigned int i = 0U; i < m_count; i++)
		delete m_handlers[i];

	delete[] m_handlers;
}

CCCSHandler::CCCSHandler(ICCSCallback* handler, const std::string& callsign, unsigned int delay, double latitude, double longitude, double frequency, double offset, const std::string& description1, const std::string& description2, const std::string& url, unsigned int localPort) :
m_handler(handler),
m_callsign(callsign),
m_reflector(),
m_latitude(latitude),
m_longitude(longitude),
m_frequency(frequency),
m_offset(offset),
m_description1(description1),
m_description2(description2),
m_url(url),
m_ccsAddress(),
m_protocol(localPort, m_localAddress),
m_state(CS_DISABLED),
m_local(),
m_announceTimer(1000U, 20U),			// 20 seconds
m_inactivityTimer(1000U, 300U),			// 5 minutes
m_pollInactivityTimer(1000U, 60U),		// 60 seconds
m_pollTimer(1000U, 10U),				// 10 seconds
m_waitTimer(1000U, delay),
m_tryTimer(1000U, 1U),					// 1 second
m_tryCount(0U),
m_id(0x00U),
m_seqNo(0U),
m_time(),
m_direction(DIR_OUTGOING),
m_yourCall(),
m_myCall1(),
m_myCall2(),
m_rptCall1()
{
	assert(handler != NULL);

	// Add to the global list
	for (unsigned int i = 0U; i < m_count; i++) {
		if (m_handlers[i] == NULL) {
			m_handlers[i] = this;
			break;
		}
	}
}

CCCSHandler::~CCCSHandler()
{
}

void CCCSHandler::setReflector(const std::string& callsign)
{
	m_reflector = callsign;

	if (0 == m_reflector.size())
		m_reflector.assign("        ");
}

void CCCSHandler::processInt()
{
	if (m_state == CS_DISABLED)
		return;

	for (;;) {
		CCS_TYPE type = m_protocol.read();

		switch (type) {
			case CT_DATA: {
					CAMBEData* data = m_protocol.readData();
					if (data != NULL) {
						process(*data);
						delete data;
					}
				}
				break;

			case CT_POLL: {
					CPollData* poll = m_protocol.readPoll();
					if (poll != NULL) {
						process(*poll);
						delete poll;
					}
				}
				break;

			case CT_CONNECT: {
					CConnectData* connect = m_protocol.readConnect();
					if (connect != NULL) {
						process(*connect);
						delete connect;
					}
				}
				break;

			case CT_MISC: {
					CCCSData* data = m_protocol.readMisc();
					if (data != NULL) {
						process(*data);
						delete data;
					}
				}
				break;

			default:
				return;
		}
	}
}

void CCCSHandler::process(CAMBEData& data)
{
	CHeaderData& header = data.getHeader();
	std::string myCall1 = header.getMyCall1();
	std::string rptCall1 = header.getRptCall1();
	std::string yourCall = header.getYourCall();
	unsigned int seqNo = data.getSeq();
	unsigned int id = data.getId();

	if (m_state != CS_CONNECTED && m_state != CS_ACTIVE)
		return;

	// This is a new incoming CCS call
	if (m_state == CS_CONNECTED) {
		m_yourCall    = myCall1;
		m_local       = yourCall;
		m_rptCall1    = rptCall1;
		m_direction   = DIR_INCOMING;
		m_time        = ::time(NULL);
		m_state       = CS_ACTIVE;
		m_stateChange = true;
		m_inactivityTimer.start();

		m_handler->ccsLinkMade(m_yourCall, m_direction);

		printf("CCS: New incoming link to %s from %s @ %s\n", m_local.c_str(), m_yourCall.c_str(), m_rptCall1.c_str());
	} else {
		if (m_yourCall.compare(myCall1) && m_rptCall1.compare(rptCall1)) {
			printf("CCS: Rejecting new incoming CCS link from %s @ %s to %s\n", myCall1.c_str(), rptCall1.c_str(), yourCall.c_str());

			CCCSData data(yourCall, myCall1, CT_TERMINATE);
			data.setDestination(m_ccsAddress, CCS_PORT);

			m_protocol.writeMisc(data);
			m_protocol.writeMisc(data);
			m_protocol.writeMisc(data);
			m_protocol.writeMisc(data);
			m_protocol.writeMisc(data);

			return;
		}

		// Allow for the fact that the distant repeater may change during the QSO
		if (0==m_yourCall.compare(myCall1) && m_rptCall1.compare(rptCall1)) {
			printf("CCS: %s has moved from repeater %s to %s\n", m_yourCall.c_str(), m_rptCall1.c_str(), rptCall1.c_str());
			m_rptCall1 = rptCall1;
		}
	}

	m_pollInactivityTimer.start();
	m_inactivityTimer.start();

	if (m_id != id) {
		header.setCQCQCQ();
		m_handler->process(header, DIR_INCOMING, AS_CCS);

		m_id = id;
	} else if (seqNo == 0U) {
		header.setCQCQCQ();
		m_handler->process(header, DIR_INCOMING, AS_DUP);
	}

	m_handler->process(data, DIR_INCOMING, AS_CCS);
}

void CCCSHandler::process(CCCSData& data)
{
	CC_TYPE type = data.getType();

	switch (type) {
		case CT_TERMINATE:
			if (m_state == CS_ACTIVE) {
				printf("CCS: Link between %s and %s has been terminated\n", data.getLocal().c_str(), data.getRemote().c_str());
				m_stateChange = true;
				m_state       = CS_CONNECTED;
				m_inactivityTimer.stop();
				m_handler->ccsLinkEnded(data.getRemote(), m_direction);
			}
			break;

		case CT_DTMFNOTFOUND:
			printf("CCS: Cannot map %s to a callsign\n", m_yourCall.c_str());
			m_stateChange = true;
			m_state       = CS_CONNECTED;
			m_inactivityTimer.stop();
			m_handler->ccsLinkFailed(m_yourCall, m_direction);
			break;

		case CT_DTMFFOUND:
			printf("CCS: Mapped %s to %s, added to the cache\n", m_yourCall.c_str(), data.getRemote().c_str());
			addToCache(m_yourCall, data.getRemote());
			m_stateChange = true;
			m_yourCall = data.getRemote();
			m_rptCall1 = data.getRemote();
			m_handler->ccsLinkMade(m_yourCall, m_direction);
			break;

		default:
			break;
	}
}

void CCCSHandler::process(CPollData&)
{
	m_pollInactivityTimer.start();
}

void CCCSHandler::process(CConnectData& connect)
{
	CD_TYPE type = connect.getType();

	if (type == CT_ACK && m_state == CS_CONNECTING) {
		printf("CCS: %s connected to server %s\n", m_callsign.c_str(), m_ccsHost.c_str());

		m_announceTimer.start();
		m_pollInactivityTimer.start();
		m_pollTimer.start();
		m_tryTimer.stop();

		// Give our location, frequency, etc
		CCCSData data(m_callsign, m_latitude, m_longitude, m_frequency, m_offset, m_description1, m_description2, m_url, CT_INFO);
		data.setDestination(m_ccsAddress, CCS_PORT);
		m_protocol.writeMisc(data);

		m_state = CS_CONNECTED;

		return;
	}

	if (type == CT_NAK && m_state == CS_CONNECTING) {
		printf("CCS: Connection refused for %s\n", m_callsign.c_str());
		m_tryTimer.stop();
		m_state = CS_DISABLED;
		return;
	}
}

bool CCCSHandler::connect()
{
	// Is CCS disabled?
	if (0 == m_localAddress.compare("127.0.0.1"))
		return false;

	// Can we resolve the CCS server address?
	m_ccsAddress = CUDPReaderWriter::lookup(m_ccsHost);
	if (m_ccsAddress.s_addr == INADDR_NONE) {
		printf("CCS: Unable to find the IP address for %s\n", m_ccsHost.c_str());
		return false;
	}

	bool res = m_protocol.open();
	if (!res)
		return false;

	printf("CCS: Opening UDP port %u for %s\n", m_protocol.getPort(), m_callsign.c_str());

	m_waitTimer.start();

	m_state = CS_CONNECTING;

	return true;
}

void CCCSHandler::disconnectInt()
{
	if (m_state == CS_CONNECTED || m_state == CS_ACTIVE) {
		CConnectData connect(m_callsign, CT_UNLINK, m_ccsAddress, CCS_PORT);
		m_protocol.writeConnect(connect);
	}

	m_announceTimer.stop();
	m_pollInactivityTimer.stop();
	m_inactivityTimer.stop();
	m_pollTimer.stop();
	m_tryTimer.stop();

	if (m_state != CS_DISABLED)
		m_protocol.close();

	m_state = CS_DISABLED;
}

void CCCSHandler::startLink(const std::string& dtmf, const std::string& user, const std::string& type)
{
	if (m_state != CS_CONNECTED)
		return;

	std::string callsign = findInCache(dtmf);
	if (callsign.size()) {
		printf("CCS: New outgoing link to %s/%s via %s by %s\n", dtmf.c_str(), callsign.c_str(), type.c_str(), user.c_str());
		m_handler->ccsLinkMade(callsign, m_direction);
		m_yourCall = callsign;
		m_rptCall1 = callsign;
	} else {
		printf("CCS: New outgoing link to %s via %s by %s\n", dtmf.c_str(), type.c_str(), user.c_str());
		m_yourCall = dtmf;
		m_yourCall.resize(LONG_CALLSIGN_LENGTH, ' ');
		m_rptCall1.clear();
	}

	m_local = user;
	m_seqNo = 0U;

	m_time        = ::time(NULL);
	m_stateChange = true;
	m_state       = CS_ACTIVE;
	m_direction   = DIR_OUTGOING;
	m_inactivityTimer.start();
}

void CCCSHandler::stopLink(const std::string& user, const std::string& type)
{
	if (m_state != CS_ACTIVE)
		return;

	if (user.size() && type.size())
		printf("CCS: Link to %s from %s has been terminated via %s by %s\n", m_yourCall.c_str(), m_local.c_str(), type.c_str(), user.c_str());

	CCCSData data(m_local, m_yourCall, CT_TERMINATE);
	data.setDestination(m_ccsAddress, CCS_PORT);

	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);

	m_stateChange = true;
	m_state       = CS_CONNECTED;
	m_inactivityTimer.stop();

	m_handler->ccsLinkEnded(m_yourCall, m_direction);
}

void CCCSHandler::unlink(const std::string& callsign)
{
	if (m_state != CS_ACTIVE)
		return;

	if (m_yourCall.compare(callsign))
		return;

	printf("CCS: Link to %s from %s has been terminated by command\n", m_yourCall.c_str(), m_local.c_str());

	CCCSData data(m_local, m_yourCall, CT_TERMINATE);
	data.setDestination(m_ccsAddress, CCS_PORT);

	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);
	m_protocol.writeMisc(data);

	m_stateChange = true;
	m_state       = CS_CONNECTED;
	m_inactivityTimer.stop();

	m_handler->ccsLinkEnded(m_yourCall, m_direction);
}

void CCCSHandler::writeHeard(CHeaderData& header)
{
	if (m_state != CS_CONNECTED && m_state != CS_ACTIVE)
		return;

	CHeardData heard(header, m_callsign, m_reflector);
	heard.setDestination(m_ccsAddress, CCS_PORT);
	m_protocol.writeHeard(heard);
}

void CCCSHandler::writeHeader(CHeaderData& header)
{
	m_myCall1 = header.getMyCall1();
	m_myCall2 = header.getMyCall2();

	m_seqNo = 0U;
}

void CCCSHandler::writeAMBE(CAMBEData& data)
{
	if (m_state != CS_ACTIVE)
		return;

	CAMBEData temp(data);

	CHeaderData& header = temp.getHeader();
	header.setMyCall1(m_myCall1);
	header.setMyCall2(m_myCall2);
	header.setYourCall(m_yourCall);
	header.setRptCall1(m_callsign);
	header.setRptCall2(m_reflector);

	temp.setRptSeq(m_seqNo++);
	temp.setDestination(m_ccsAddress, CCS_PORT);
	m_protocol.writeData(temp);
}

CCS_STATUS CCCSHandler::getStatus() const
{
	return m_state;
}

void CCCSHandler::clockInt(unsigned int ms)
{
	m_announceTimer.clock(ms);
	m_pollInactivityTimer.clock(ms);
	m_inactivityTimer.clock(ms);
	m_pollTimer.clock(ms);
	m_waitTimer.clock(ms);
	m_tryTimer.clock(ms);

	if (m_pollInactivityTimer.isRunning() && m_pollInactivityTimer.hasExpired()) {
		printf("CCS: Connection has failed (poll inactivity) for %s, reconnecting\n", m_callsign.c_str());

		m_announceTimer.stop();
		m_pollInactivityTimer.stop();
		m_inactivityTimer.stop();
		m_pollTimer.stop();

		if (m_state == CS_ACTIVE) {
			m_stateChange = true;
			m_handler->ccsLinkEnded(m_yourCall, m_direction);
		}

		m_waitTimer.start();

		m_state = CS_CONNECTING;

		return;
	}

	if (m_tryTimer.isRunning() && m_tryTimer.hasExpired()) {
		CConnectData connect(m_callsign, CT_LINK1, m_ccsAddress, CCS_PORT);
		if (m_latitude != 0.0 && m_longitude != 0.0) {
			std::string locator = CUtils::latLonToLoc(m_latitude, m_longitude);
			connect.setLocator(locator);
		}
		m_protocol.writeConnect(connect);

		unsigned int t = calcBackoff();
		m_tryTimer.start(t);
	}

	if (m_pollTimer.isRunning() && m_pollTimer.hasExpired()) {
		CPollData poll(m_callsign, m_ccsAddress, CCS_PORT);
		m_protocol.writePoll(poll);

		m_pollTimer.start();
	}

	if (m_inactivityTimer.isRunning() && m_inactivityTimer.hasExpired()) {
		printf("CCS: Activity timeout on link for %s\n", m_callsign.c_str());

		CCCSData data(m_local, m_yourCall, CT_TERMINATE);
		data.setDestination(m_ccsAddress, CCS_PORT);

		m_protocol.writeMisc(data);
		m_protocol.writeMisc(data);
		m_protocol.writeMisc(data);
		m_protocol.writeMisc(data);
		m_protocol.writeMisc(data);

		m_stateChange = true;
		m_state       = CS_CONNECTED;
		m_inactivityTimer.stop();

		m_handler->ccsLinkEnded(m_yourCall, m_direction);
	}

	if (m_waitTimer.isRunning() && m_waitTimer.hasExpired()) {
		CConnectData connect(m_callsign, CT_LINK1, m_ccsAddress, CCS_PORT);
		if (m_latitude != 0.0 && m_longitude != 0.0) {
			std::string locator = CUtils::latLonToLoc(m_latitude, m_longitude);
			connect.setLocator(locator);
		}
		m_protocol.writeConnect(connect);

		m_tryTimer.start(1U);
		m_tryCount = 1U;

		m_waitTimer.stop();
	}

	if (m_announceTimer.isRunning() && m_announceTimer.hasExpired()) {
		CHeaderData header;
		header.setMyCall1(m_callsign.substr(0, LONG_CALLSIGN_LENGTH - 1U));
		CHeardData heard(header, m_callsign, std::string(""));
		heard.setDestination(m_ccsAddress, CCS_PORT);
		m_protocol.writeHeard(heard);

		m_announceTimer.start(3600U);
	}
}

unsigned int CCCSHandler::calcBackoff()
{
	if (m_tryCount >= 7U) {
		m_tryCount++;
		return 60U;
	}

	unsigned int timeout = 1U;

	for (unsigned int i = 0U; i < m_tryCount; i++)
		timeout *= 2U;

	m_tryCount++;

	if (timeout > 60U)
		return 60U;
	else
		return timeout;
}

bool CCCSHandler::stateChange()
{
	bool stateChange = m_stateChange;

	m_stateChange = false;

	return stateChange;
}

void CCCSHandler::writeStatus(FILE *file)
{
	for (unsigned int i = 0U; i < m_count; i++) {
		CCCSHandler* handler = m_handlers[i];
		if (handler != NULL) {
			struct tm* tm = ::gmtime(&handler->m_time);

			switch (handler->m_direction) {
				case DIR_OUTGOING:
					if (handler->m_state == CS_ACTIVE) {
						fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: CCS link - Rptr: %s Remote: %s Dir: Outgoing\n",
							tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, 
							handler->m_callsign.c_str(), handler->m_yourCall.c_str());
					}
					break;

				case DIR_INCOMING:
					if (handler->m_state == CS_ACTIVE) {
						fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: CCS link - Rptr: %s Remote: %s Dir: Incoming\n",
							tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, 
							handler->m_callsign.c_str(), handler->m_yourCall.c_str());
					}
					break;
			}
		}
	}
}

void CCCSHandler::addToCache(const std::string& dtmf, const std::string& callsign)
{
	m_mutex.lock();

	m_cache[dtmf] = callsign;

	m_mutex.unlock();
}

std::string CCCSHandler::findInCache(const std::string& dtmf)
{
	m_mutex.lock();

	return m_cache[dtmf];

	m_mutex.unlock();
}
