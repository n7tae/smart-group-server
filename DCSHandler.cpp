/*
 *   Copyright (C) 2012-2015 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2018 by Thomas A. Early N7TAE
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

#include "RepeaterHandler.h"
#include "DCSHandler.h"
#include "Utils.h"

CDCSProtocolHandlerPool *CDCSHandler::m_pool = NULL;
CDCSProtocolHandler     *CDCSHandler::m_incoming = NULL;

bool                     CDCSHandler::m_stateChange = false;

GATEWAY_TYPE             CDCSHandler::m_gatewayType  = GT_REPEATER;

CCallsignList           *CDCSHandler::m_whiteList = NULL;
CCallsignList           *CDCSHandler::m_blackList = NULL;
std::list<CDCSHandler *> m_reflectors;


CDCSHandler::CDCSHandler(IReflectorCallback *handler, const std::string &reflector, const std::string &repeater, CDCSProtocolHandler *protoHandler, const in_addr &address, unsigned int port, DIRECTION direction) :
m_reflector(reflector),
m_repeater(repeater),
m_handler(protoHandler),
m_yourAddress(address),
m_yourPort(port),
m_myPort(0U),
m_direction(direction),
m_linkState(DCS_LINKING),
m_destination(handler),
m_time(),
m_pollTimer(1000U, 5U),
m_pollInactivityTimer(1000U, 60U),
m_tryTimer(1000U, 1U),
m_tryCount(0U),
m_dcsId(0x00U),
m_dcsSeq(0x00U),
m_seqNo(0x00U),
m_inactivityTimer(1000U, NETWORK_TIMEOUT),
m_yourCall(),
m_myCall1(),
m_myCall2(),
m_rptCall1(),
m_rptCall2()
{
	assert(protoHandler != NULL);
	assert(handler != NULL);
	assert(port > 0U);

	m_myPort = protoHandler->getPort();

	m_pollInactivityTimer.start();

	m_time = ::time(NULL);

	if (direction == DIR_INCOMING) {
		m_pollTimer.start();
		m_stateChange = true;
		m_linkState = DCS_LINKED;
	} else {
		m_linkState = DCS_LINKING;
		m_tryTimer.start();
	}
}

CDCSHandler::~CDCSHandler()
{
	if (m_direction == DIR_OUTGOING)
		m_pool->release(m_handler);
}

void CDCSHandler::setDCSProtocolHandlerPool(CDCSProtocolHandlerPool *pool)
{
	assert(pool != NULL);

	m_pool = pool;
}

void CDCSHandler::setDCSProtocolIncoming(CDCSProtocolHandler *handler)
{
	assert(handler != NULL);

	m_incoming = handler;
}

void CDCSHandler::setGatewayType(GATEWAY_TYPE type)
{
	m_gatewayType = type;
}

void CDCSHandler::setWhiteList(CCallsignList *list)
{
	assert(list != NULL);

	m_whiteList = list;
}

void CDCSHandler::setBlackList(CCallsignList *list)
{
	assert(list != NULL);

	m_blackList = list;
}

std::string CDCSHandler::getIncoming(const std::string &callsign)
{
	std::string incoming;

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		if (reflector->m_direction == DIR_INCOMING && 0==reflector->m_repeater.compare(callsign)) {
			incoming.append(reflector->m_reflector);
			incoming.append("  ");
		}
	}

	return incoming;
}

void CDCSHandler::getInfo(IReflectorCallback *handler, CRemoteRepeaterData &data)
{
	assert(handler != NULL);

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		if (reflector->m_destination == handler) {
			if (reflector->m_direction == DIR_INCOMING && 0==reflector->m_repeater.size()) {
				if (reflector->m_linkState != DCS_UNLINKING)
					data.addLink(reflector->m_reflector, PROTO_DCS, reflector->m_linkState == DCS_LINKED, DIR_INCOMING, true);
			} else {
				if (reflector->m_linkState != DCS_UNLINKING)
					data.addLink(reflector->m_reflector, PROTO_DCS, reflector->m_linkState == DCS_LINKED, reflector->m_direction, false);
			}
		}
	}
}

void CDCSHandler::process(CAMBEData &data)
{
	in_addr   yourAddress = data.getYourAddress();
	unsigned int yourPort = data.getYourPort();
	unsigned int myPort   = data.getMyPort();

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		if (		reflector->m_yourAddress.s_addr == yourAddress.s_addr &&
					reflector->m_yourPort           == yourPort &&
					reflector->m_myPort             == myPort) {
			reflector->processInt(data);
			return;
		}
	}	
}

void CDCSHandler::process(CPollData &poll)
{
	std::string   reflector  = poll.getData1();
	std::string   repeater   = poll.getData2();
	in_addr   yourAddress = poll.getYourAddress();
	unsigned int yourPort = poll.getYourPort();
	unsigned int   myPort = poll.getMyPort();
	unsigned int   length = poll.getLength();

	// Check to see if we already have a link
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *handler = *it;
		if (		0==handler->m_reflector.compare(reflector) &&
					0==handler->m_repeater.compare(repeater) &&
					handler->m_yourAddress.s_addr == yourAddress.s_addr &&
					handler->m_yourPort  == yourPort &&
					handler->m_myPort    == myPort &&
					handler->m_direction == DIR_OUTGOING &&
					handler->m_linkState == DCS_LINKED &&
					length == 22U) {
			handler->m_pollInactivityTimer.start();
			CPollData reply(handler->m_repeater, handler->m_reflector, handler->m_direction, handler->m_yourAddress, handler->m_yourPort);
			handler->m_handler->writePoll(reply);
			return;
		} else if (0==handler->m_reflector.compare(0, LONG_CALLSIGN_LENGTH - 1U, reflector, 0, LONG_CALLSIGN_LENGTH - 1U) &&
				   handler->m_yourAddress.s_addr == yourAddress.s_addr &&
				   handler->m_yourPort  == yourPort &&
				   handler->m_myPort    == myPort &&
				   handler->m_direction == DIR_INCOMING &&
				   handler->m_linkState == DCS_LINKED &&
				   length == 17U) {
			handler->m_pollInactivityTimer.start();
			return;
		}
	}

	printf("Unknown incoming DCS poll from %s\n", reflector.c_str());
}

void CDCSHandler::process(CConnectData &connect)
{
	CD_TYPE type = connect.getType();

	if (type == CT_ACK || type == CT_NAK || type == CT_UNLINK) {
		for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); ) {
			CDCSHandler *reflector = *it;
			bool res = reflector->processInt(connect, type);
			if (res) {
				delete reflector;
				it = m_reflectors.erase(it);
			} else
				it++;
		}
		return;
	}

	// else if type == CT_LINK1 or type == CT_LINK2
	in_addr   yourAddress = connect.getYourAddress();
	unsigned int yourPort = connect.getYourPort();
	unsigned int   myPort = connect.getMyPort();

	std::string repeaterCallsign = connect.getRepeater();
	std::string reflectorCallsign = connect.getReflector();

	// Check that it isn't a duplicate
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		if (		reflector->m_direction          == DIR_INCOMING &&
					reflector->m_yourAddress.s_addr == yourAddress.s_addr &&
					reflector->m_yourPort           == yourPort &&
					reflector->m_myPort             == myPort &&
					0==reflector->m_repeater.compare(reflectorCallsign) &&
					0==reflector->m_reflector.compare(repeaterCallsign))
			return;
	}

	// Check the validity of our repeater callsign
	IReflectorCallback *handler = CRepeaterHandler::findDVRepeater(reflectorCallsign);
	if (handler == NULL) {
		printf("DCS connect to unknown reflector %s from %s\n", reflectorCallsign.c_str(), repeaterCallsign.c_str());
		CConnectData reply(repeaterCallsign, reflectorCallsign, CT_NAK, connect.getYourAddress(), connect.getYourPort());
		m_incoming->writeConnect(reply);
		return;
	}

	// A new connect packet indicates the need for a new entry
	printf("New incoming DCS link to %s from %s\n", reflectorCallsign.c_str(), repeaterCallsign.c_str());

	CDCSHandler *dcs = new CDCSHandler(handler, repeaterCallsign, reflectorCallsign, m_incoming, yourAddress, yourPort, DIR_INCOMING);
	if (dcs) {
		m_reflectors.push_back(dcs);
		CConnectData reply(repeaterCallsign, reflectorCallsign, CT_ACK, yourAddress, yourPort);
		m_incoming->writeConnect(reply);
	}
}

void CDCSHandler::link(IReflectorCallback *handler, const std::string &repeater, const std::string &gateway, const in_addr &address)
{
	// if the handler is currently unlinking, quit!
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		if (reflector->m_direction == DIR_OUTGOING && reflector->m_destination == handler && reflector->m_linkState != DCS_UNLINKING)
			return;
	}

	CDCSProtocolHandler *protoHandler = m_pool->getHandler();
	if (protoHandler == NULL)
		return;

	CDCSHandler *dcs = new CDCSHandler(handler, gateway, repeater, protoHandler, address, DCS_PORT, DIR_OUTGOING);
	if (dcs) {
		m_reflectors.push_back(dcs);
		CConnectData reply(m_gatewayType, repeater, gateway, CT_LINK1, address, DCS_PORT);
		protoHandler->writeConnect(reply);
	}
}

void CDCSHandler::unlink(IReflectorCallback *handler, const std::string &callsign, bool exclude)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;

		if (reflector != NULL) {
			bool found = false;

			if (exclude) {
				if (reflector->m_direction == DIR_OUTGOING && reflector->m_destination == handler && reflector->m_reflector.compare(callsign)) {
					printf("Removing outgoing DCS link %s, %s\n", reflector->m_repeater.c_str(), reflector->m_reflector.c_str());

					if (reflector->m_linkState == DCS_LINKING || reflector->m_linkState == DCS_LINKED) {
						CConnectData connect(reflector->m_repeater, reflector->m_reflector, CT_UNLINK, reflector->m_yourAddress, reflector->m_yourPort);
						reflector->m_handler->writeConnect(connect);

						reflector->m_linkState = DCS_UNLINKING;
						reflector->m_tryTimer.start(1U);
						reflector->m_tryCount = 0U;
					}

					found = true;
				}
			} else {
				if (reflector->m_destination == handler && 0==reflector->m_reflector.compare(callsign)) {
					printf("Removing DCS link %s, %s\n", reflector->m_repeater.c_str(), reflector->m_reflector.c_str());

					if (reflector->m_linkState == DCS_LINKING || reflector->m_linkState == DCS_LINKED) {
						CConnectData connect(reflector->m_repeater, reflector->m_reflector, CT_UNLINK, reflector->m_yourAddress, reflector->m_yourPort);
						reflector->m_handler->writeConnect(connect);

						reflector->m_linkState = DCS_UNLINKING;
						reflector->m_tryTimer.start(1U);
						reflector->m_tryCount = 0U;
					}

					found = true;
				}
			}

			// If an active link with incoming traffic, send an EOT to the repeater
			if (found) {
				if (reflector->m_dcsId != 0x00U) {
					unsigned int seq = reflector->m_dcsSeq + 1U;
					if (seq == 21U)
						seq = 0U;

					CAMBEData data;
					data.setData(END_PATTERN_BYTES, DV_FRAME_LENGTH_BYTES);
					data.setSeq(seq);
					data.setEnd(true);
					data.setId(reflector->m_dcsId);

					reflector->m_destination->process(data, reflector->m_direction, AS_DCS);
				}

				m_stateChange = true;
			}
		}
	}	
}

void CDCSHandler::unlink(CDCSHandler *reflector)
{
	if (reflector != NULL) {
		if (reflector->m_repeater.size()) {
			printf("Unlinking from DCS reflector %s\n", reflector->m_reflector.c_str());

			CConnectData connect(reflector->m_repeater, reflector->m_reflector, CT_UNLINK, reflector->m_yourAddress, reflector->m_yourPort);
			reflector->m_handler->writeConnect(connect);

			reflector->m_linkState = DCS_UNLINKING;
			reflector->m_tryTimer.start(1U);
			reflector->m_tryCount = 0U;
		}
	}

}

void CDCSHandler::unlink()
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler* reflector = *it;
		CDCSHandler::unlink(reflector);
	}	
}

void CDCSHandler::writeHeader(IReflectorCallback *handler, CHeaderData &header, DIRECTION direction)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		reflector->writeHeaderInt(handler, header, direction);
	}	
}

void CDCSHandler::writeAMBE(IReflectorCallback *handler, CAMBEData &data, DIRECTION direction)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		reflector->writeAMBEInt(handler, data, direction);
	}	
}

void CDCSHandler::gatewayUpdate(const std::string &reflector, const std::string &address)
{
	std::string gateway = reflector;
	gateway.resize(LONG_CALLSIGN_LENGTH - 1U, ' ');

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		if (0 == reflector->m_reflector.compare(0, LONG_CALLSIGN_LENGTH - 1U, gateway)) {
			if (address.size()) {
				// A new address, change the value
				printf("Changing IP address of DCS gateway or reflector %s to %s\n", reflector->m_reflector.c_str(), address.c_str());
				reflector->m_yourAddress.s_addr = ::inet_addr(address.c_str());
			} else {
				printf("IP address for DCS gateway or reflector %s has been removed\n", reflector->m_reflector.c_str());

				// No address, this probably shouldn't happen....
				if (reflector->m_direction == DIR_OUTGOING && reflector->m_destination != NULL)
					reflector->m_destination->linkFailed(DP_DCS, reflector->m_reflector, false);

				m_stateChange = true;

				delete reflector;
				it = m_reflectors.erase(it);
				it--;
			}
		}
	}
}

void CDCSHandler::clock(unsigned int ms)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); ) {
		CDCSHandler *handler = *it;
		bool ret = handler->clockInt(ms);
		if (ret) {
			delete handler;
			it = m_reflectors.erase(it);
		} else
			it++;
	}
}

void CDCSHandler::finalise()
{
	while (m_reflectors.end() != m_reflectors.begin()) {
		delete *(m_reflectors.begin());
		m_reflectors.erase(m_reflectors.begin());
	}
}

void CDCSHandler::processInt(CAMBEData &data)
{
	// Make a copy of the AMBE data so that any changes made here don't modify the original
	CAMBEData temp(data);

	unsigned int id = temp.getId();
	CHeaderData& header = temp.getHeader();
	unsigned int seqNo = temp.getSeq();

	std::string   my = header.getMyCall1();
	std::string rpt2 = header.getRptCall2();

	if (m_whiteList != NULL) {
		bool res = m_whiteList->isInList(my);
		if (!res) {
			printf("%s rejected from DCS as not found in the white list\n", my.c_str());
			m_dcsId = 0x00U;
			return;
		}
	}

	if (m_blackList != NULL) {
		bool res = m_blackList->isInList(my);
		if (res) {
			printf("%s rejected from DCS as found in the black list\n", my.c_str());
			m_dcsId = 0x00U;
			return;
		}
	}

	if (m_linkState != DCS_LINKED)
		return;

	switch (m_direction) {
		case DIR_OUTGOING:
			if (m_reflector.compare(rpt2))
				return;

			if (m_dcsId == 0x00U && seqNo != 0U)
				return;

			if (m_dcsId == 0x00U) {		// && seqNo == 0U) {
				m_dcsId  = id;
				m_dcsSeq = 0x00U;
				m_inactivityTimer.start();

				header.setCQCQCQ();
				header.setFlags(0x00U, 0x00U, 0x00U);

				m_destination->process(header, m_direction, AS_DCS);
			}
			
			if (id == m_dcsId) {
				m_pollInactivityTimer.start();
				m_inactivityTimer.start();

				m_dcsSeq = seqNo;

				if (m_dcsSeq == 0U) {
					// Send the header every 21 frames
					header.setCQCQCQ();
					header.setFlags(0x00U, 0x00U, 0x00U);

					m_destination->process(header, m_direction, AS_DUP);
				}

				m_destination->process(temp, m_direction, AS_DCS);

				if (temp.isEnd()) {
					m_dcsId  = 0x00U;
					m_dcsSeq = 0x00U;
					m_inactivityTimer.stop();
				}
			}
			break;

		case DIR_INCOMING:
			if (m_repeater.compare(rpt2))
				return;

			if (m_dcsId == 0x00U && seqNo != 0U)
				return;

			if (m_dcsId == 0x00U) {		// && seqNo == 0U) {
				m_dcsId  = id;
				m_dcsSeq = 0x00U;
				m_inactivityTimer.start();

				header.setCQCQCQ();
				header.setFlags(0x00U, 0x00U, 0x00U);

				m_destination->process(header, m_direction, AS_DCS);
			}

			if (id == m_dcsId) {
				m_pollInactivityTimer.start();
				m_inactivityTimer.start();

				m_dcsSeq = seqNo;

				if (m_dcsSeq == 0U) {
					// Send the header every 21 frames
					header.setCQCQCQ();
					header.setFlags(0x00U, 0x00U, 0x00U);

					m_destination->process(header, m_direction, AS_DUP);
				}

				m_destination->process(temp, m_direction, AS_DCS);

				if (temp.isEnd()) {
					m_dcsId  = 0x00U;
					m_dcsSeq = 0x00U;
					m_inactivityTimer.stop();
				}
			}
			break;
	}
}

bool CDCSHandler::processInt(CConnectData &connect, CD_TYPE type)
{
	in_addr   yourAddress = connect.getYourAddress();
	unsigned int yourPort = connect.getYourPort();
	unsigned int   myPort = connect.getMyPort();
	std::string  repeater = connect.getRepeater();

	if (m_yourAddress.s_addr != yourAddress.s_addr || m_yourPort != yourPort || m_myPort != myPort)
		return false;

	switch (type) {
		case CT_ACK:
			if (m_repeater.compare(repeater))
				return false;

			if (m_linkState == DCS_LINKING) {
				printf("DCS ACK message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkUp(DP_DCS, m_reflector);

				m_tryTimer.stop();
				m_stateChange = true;
				m_linkState   = DCS_LINKED;
			}

			return false;

		case CT_NAK:
			if (m_repeater.compare(repeater))
				return false;

			if (m_linkState == DCS_LINKING) {
				printf("DCS NAK message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkRefused(DP_DCS, m_reflector);

				return true;
			}

			if (m_linkState == DCS_UNLINKING) {
				printf("DCS NAK message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkFailed(DP_DCS, m_reflector, false);

				return true;
			}

			return false;

		case CT_UNLINK:
			if (m_reflector.compare(repeater))
				return false;

			if (m_linkState == DCS_LINKED) {
				printf("DCS disconnect message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkFailed(DP_DCS, m_reflector, false);

				m_stateChange = true;
			}

			return true;

		default:
			return false;
	}
}

bool CDCSHandler::clockInt(unsigned int ms)
{
	m_pollInactivityTimer.clock(ms);
	m_inactivityTimer.clock(ms);
	m_pollTimer.clock(ms);
	m_tryTimer.clock(ms);

	if (m_pollInactivityTimer.isRunning() && m_pollInactivityTimer.hasExpired()) {
		m_pollInactivityTimer.start();

		m_stateChange = true;
		m_dcsId       = 0x00U;
		m_dcsSeq      = 0x00U;

		switch (m_linkState) {
			case DCS_LINKING:
				printf("DCS link to %s has failed to connect\n", m_reflector.c_str());
				break;
			case DCS_LINKED:
				printf("DCS link to %s has failed (poll inactivity)\n", m_reflector.c_str());
				break;
			case DCS_UNLINKING:
				printf("DCS link to %s has failed to disconnect cleanly\n", m_reflector.c_str());
				break;
			default:
				break;
		}

		if (m_direction == DIR_OUTGOING) {
			bool reconnect = m_destination->linkFailed(DP_DCS, m_reflector, true);
			if (reconnect) {
				CConnectData reply(m_gatewayType, m_repeater, m_reflector, CT_LINK1, m_yourAddress, m_yourPort);
				m_handler->writeConnect(reply);
				m_linkState = DCS_LINKING;
				m_tryTimer.start(1U);
				m_tryCount = 0U;
				return false;
			}
		}

		return true;
	}

	if (m_inactivityTimer.isRunning() && m_inactivityTimer.hasExpired()) {
		m_dcsId  = 0x00U;
		m_dcsSeq = 0x00U;
		m_inactivityTimer.stop();
	}

	if (m_pollTimer.isRunning() && m_pollTimer.hasExpired()) {
		m_pollTimer.start();

		CPollData poll(m_repeater, m_reflector, m_direction, m_yourAddress, m_yourPort);
		m_handler->writePoll(poll);
	}

	if (m_linkState == DCS_LINKING) {
		if (m_tryTimer.isRunning() && m_tryTimer.hasExpired()) {
			CConnectData reply(m_gatewayType, m_repeater, m_reflector, CT_LINK1, m_yourAddress, m_yourPort);
			m_handler->writeConnect(reply);

			unsigned int timeout = calcBackoff();
			m_tryTimer.start(timeout);
		}
	}

	if (m_linkState == DCS_UNLINKING) {
		if (m_tryTimer.isRunning() && m_tryTimer.hasExpired()) {
			CConnectData connect(m_repeater, m_reflector, CT_UNLINK, m_yourAddress, m_yourPort);
			m_handler->writeConnect(connect);

			unsigned int timeout = calcBackoff();
			m_tryTimer.start(timeout);
		}
	}

	return false;
}

void CDCSHandler::writeHeaderInt(IReflectorCallback *handler, CHeaderData& header, DIRECTION direction)
{
	if (m_linkState != DCS_LINKED)
		return;

	// Is it link in the right direction
	if (m_direction != direction)
		return;

	if (m_destination != handler)
		return;

	// Already in use?
	if (m_dcsId != 0x00)
		return;

	m_seqNo    = 0U;

	m_myCall1  = header.getMyCall1();
	m_myCall2  = header.getMyCall2();
	m_yourCall = header.getYourCall();
	m_rptCall1 = header.getRptCall1();
	m_rptCall2 = header.getRptCall2();
}

void CDCSHandler::writeAMBEInt(IReflectorCallback *handler, CAMBEData &data, DIRECTION direction)
{
	if (m_linkState != DCS_LINKED)
		return;

	// Is it link in the right direction
	if (m_direction != direction)
		return;

	if (m_destination != handler)
		return;

	// Already in use?
	if (m_dcsId != 0x00)
		return;

	CHeaderData& header = data.getHeader();
	header.setMyCall1(m_myCall1);
	header.setMyCall2(m_myCall2);
	header.setRptCall1(m_rptCall1);
	header.setRptCall2(m_rptCall2);
	header.setCQCQCQ();

	data.setRptSeq(m_seqNo++);
	data.setDestination(m_yourAddress, m_yourPort);
	m_handler->writeData(data);
}

bool CDCSHandler::stateChange()
{
	bool stateChange = m_stateChange;

	m_stateChange = false;

	return stateChange;
}

void CDCSHandler::writeStatus(FILE *file)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDCSHandler *reflector = *it;
		struct tm *tm = ::gmtime(&reflector->m_time);

		switch (reflector->m_direction) {
			case DIR_OUTGOING:
				if (reflector->m_linkState == DCS_LINKED) {
					fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: DCS link - Type: Repeater Rptr: %s Refl: %s Dir: Outgoing\n",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, 
						reflector->m_repeater.c_str(), reflector->m_reflector.c_str());
				}
				break;

			case DIR_INCOMING:
				if (reflector->m_linkState == DCS_LINKED) {
					fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: DCS link - Type: Repeater Rptr: %s Refl: %s Dir: Incoming\n",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, 
						reflector->m_repeater.c_str(), reflector->m_reflector.c_str());
				}
				break;
		}
	}
}

unsigned int CDCSHandler::calcBackoff()
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
