/*
 *   Copyright (C) 2010-2015 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017-2018 by Thomas A. Early
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
#include "DExtraHandler.h"
#include "Utils.h"

unsigned int                CDExtraHandler::m_maxDongles = 0U;
std::list<CDExtraHandler *> CDExtraHandler::m_reflectors;

std::string                 CDExtraHandler::m_callsign;
CDExtraProtocolHandlerPool *CDExtraHandler::m_pool = NULL;
CDExtraProtocolHandler     *CDExtraHandler::m_incoming = NULL;

bool                        CDExtraHandler::m_stateChange = false;

CCallsignList              *CDExtraHandler::m_whiteList = NULL;
CCallsignList              *CDExtraHandler::m_blackList = NULL;


CDExtraHandler::CDExtraHandler(IReflectorCallback *handler, const std::string &reflector, const std::string &repeater, CDExtraProtocolHandler *protoHandler, const in_addr &address, unsigned int port, DIRECTION direction) :
m_reflector(reflector),
m_repeater(repeater),
m_handler(protoHandler),
m_yourAddress(address),
m_yourPort(port),
m_direction(direction),
m_linkState(DEXTRA_LINKING),
m_destination(handler),
m_time(),
m_pollTimer(1000U, 10U),
m_pollInactivityTimer(1000U, 60U),
m_tryTimer(1000U, 1U),
m_tryCount(0U),
m_dExtraId(0x00U),
m_dExtraSeq(0x00U),
m_inactivityTimer(1000U, NETWORK_TIMEOUT),
m_header(NULL)
{
	assert(protoHandler != NULL);
	assert(handler != NULL);
	assert(port > 0U);

	m_pollInactivityTimer.start();

	m_time = ::time(NULL);

	if (direction == DIR_INCOMING) {
		m_pollTimer.start();
		m_stateChange = true;
		m_linkState = DEXTRA_LINKED;
	} else {
		m_linkState = DEXTRA_LINKING;
		m_tryTimer.start();
	}
}

CDExtraHandler::CDExtraHandler(CDExtraProtocolHandler *protoHandler, const std::string &reflector, const in_addr &address, unsigned int port, DIRECTION direction) :
m_reflector(reflector),
m_repeater(),
m_handler(protoHandler),
m_yourAddress(address),
m_yourPort(port),
m_direction(direction),
m_linkState(DEXTRA_LINKING),
m_destination(NULL),
m_time(),
m_pollTimer(1000U, 10U),
m_pollInactivityTimer(1000U, 60U),
m_tryTimer(1000U, 1U),
m_tryCount(0U),
m_dExtraId(0x00U),
m_dExtraSeq(0x00U),
m_inactivityTimer(1000U, NETWORK_TIMEOUT),
m_header(NULL)
{
	assert(protoHandler != NULL);
	assert(port > 0U);

	m_pollInactivityTimer.start();

	m_time = ::time(NULL);

	if (direction == DIR_INCOMING) {
		m_pollTimer.start();
		m_stateChange = true;
		m_linkState = DEXTRA_LINKED;
	} else {
		m_linkState = DEXTRA_LINKING;
		m_tryTimer.start();
	}
}

CDExtraHandler::~CDExtraHandler()
{
	if (m_direction == DIR_OUTGOING)
		m_pool->release(m_handler);

	delete m_header;
}

void CDExtraHandler::setCallsign(const std::string& callsign)
{
	m_callsign.assign(callsign);
	m_callsign.resize(LONG_CALLSIGN_LENGTH, ' ');
	m_callsign[LONG_CALLSIGN_LENGTH - 1U] = ' ';
}

void CDExtraHandler::setDExtraProtocolIncoming(CDExtraProtocolHandler *incoming)
{
	assert(incoming != NULL);

	m_incoming = incoming;
}

void CDExtraHandler::setDExtraProtocolHandlerPool(CDExtraProtocolHandlerPool *pool)
{
	assert(pool != NULL);

	m_pool = pool;
}

void CDExtraHandler::setMaxDongles(unsigned int maxDongles)
{
	m_maxDongles = maxDongles;
}

void CDExtraHandler::setWhiteList(CCallsignList *list)
{
	assert(list != NULL);

	m_whiteList = list;
}

void CDExtraHandler::setBlackList(CCallsignList *list)
{
	assert(list != NULL);

	m_blackList = list;
}

std::string CDExtraHandler::getIncoming(const std::string &callsign)
{
	std::string incoming;

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (reflector->m_direction==DIR_INCOMING && 0==reflector->m_repeater.compare(callsign)) {
			incoming.append(reflector->m_reflector);
			incoming.append("  ");
		}
	}

	return incoming;
}

void CDExtraHandler::getInfo(IReflectorCallback *handler, CRemoteRepeaterData &data)
{
	assert(handler != NULL);

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (reflector->m_destination == handler) {
			if (reflector->m_direction == DIR_INCOMING && 0 == reflector->m_repeater.size()) {
				if (reflector->m_linkState != DEXTRA_UNLINKING)
					data.addLink(reflector->m_reflector, PROTO_DEXTRA, reflector->m_linkState == DEXTRA_LINKED, DIR_INCOMING, true);
			} else {
				if (reflector->m_linkState != DEXTRA_UNLINKING)
					data.addLink(reflector->m_reflector, PROTO_DEXTRA, reflector->m_linkState == DEXTRA_LINKED, reflector->m_direction, false);
			}
		}
	}
}

std::string CDExtraHandler::getDongles()
{
	std::string dongles;

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (reflector->m_direction==DIR_INCOMING && 0==reflector->m_repeater.size()) {
			dongles.append("X:");
			dongles.append(reflector->m_reflector);
			dongles.append("  ");
		}
	}

	return dongles;
}

void CDExtraHandler::process(CHeaderData &header)
{
	in_addr   yourAddress = header.getYourAddress();
	unsigned int yourPort = header.getYourPort();

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (reflector->m_yourAddress.s_addr==yourAddress.s_addr && reflector->m_yourPort==yourPort)
			reflector->processInt(header);
	}
}

void CDExtraHandler::process(CAMBEData &data)
{
	in_addr   yourAddress = data.getYourAddress();
	unsigned int yourPort = data.getYourPort();

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (yourAddress.s_addr==reflector->m_yourAddress.s_addr && yourPort==reflector->m_yourPort)
			reflector->processInt(data);
	}
}

void CDExtraHandler::process(const CPollData &poll)
{
	std::string reflector = poll.getData1();
	in_addr   yourAddress = poll.getYourAddress();
	unsigned int yourPort = poll.getYourPort();
	// Check to see if we already have a link
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *handler = *it;
		if (		0==handler->m_reflector.compare(0, LONG_CALLSIGN_LENGTH-1, reflector, 0, LONG_CALLSIGN_LENGTH-1) &&
					handler->m_yourAddress.s_addr == yourAddress.s_addr &&
					handler->m_yourPort           == yourPort &&
					handler->m_linkState          == DEXTRA_LINKED) {
			handler->m_pollInactivityTimer.start();
			return;
		}
	}

	// A repeater poll arriving here is an error
	if (!poll.isDongle())
		return;

	// Check to see if we are allowed to accept it
	unsigned int count = 0U;
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *handler = *it;
		if (handler->m_direction == DIR_INCOMING && 0==handler->m_repeater.size())
			count++;
	}

	if (count >= m_maxDongles)
		return;

	// An unmatched poll indicates the need for a new entry
	printf("New incoming DExtra Dongle from %s\n", reflector.c_str());

	CDExtraHandler *handler = new CDExtraHandler(m_incoming, reflector, yourAddress, yourPort, DIR_INCOMING);
	if (handler) {
		m_reflectors.push_back(handler);
		CPollData poll(m_callsign, yourAddress, yourPort);
		m_incoming->writePoll(poll);
	}
}

void CDExtraHandler::process(CConnectData &connect)
{
	CD_TYPE type = connect.getType();

	if (type == CT_ACK || type == CT_NAK || type == CT_UNLINK) {
		for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); ) {
			CDExtraHandler *reflector = *it;
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

	std::string repeaterCallsign = connect.getRepeater();

	char band = connect.getReflector().at(LONG_CALLSIGN_LENGTH - 1U);

	std::string reflectorCallsign = m_callsign;
	reflectorCallsign[LONG_CALLSIGN_LENGTH - 1U] = band;

	// Check that it isn't a duplicate
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (		reflector->m_direction          == DIR_INCOMING &&
					reflector->m_yourAddress.s_addr == yourAddress.s_addr &&
					reflector->m_yourPort           == yourPort &&
					0==reflector->m_repeater.compare(reflectorCallsign) &&
					0==reflector->m_reflector.compare(repeaterCallsign))
			return;
	}

	// Check the validity of our repeater callsign
	IReflectorCallback *handler = CRepeaterHandler::findDVRepeater(reflectorCallsign);
	if (handler == NULL) {
		printf("DExtra connect to unknown reflector %s from %s\n", reflectorCallsign.c_str(), repeaterCallsign.c_str());
		CConnectData reply(repeaterCallsign, reflectorCallsign, CT_NAK, yourAddress, yourPort);
		m_incoming->writeConnect(reply);
		return;
	}

	// A new connect packet indicates the need for a new entry
	printf("New incoming DExtra link to %s from %s\n", reflectorCallsign.c_str(), repeaterCallsign.c_str());

	CDExtraHandler *dextra = new CDExtraHandler(handler, repeaterCallsign, reflectorCallsign, m_incoming, yourAddress, yourPort, DIR_INCOMING);
	if (dextra) {
		m_reflectors.push_back(dextra);
		CConnectData reply(repeaterCallsign, reflectorCallsign, CT_ACK, yourAddress, yourPort);
		m_incoming->writeConnect(reply);

		std::string callsign = repeaterCallsign;
		callsign[LONG_CALLSIGN_LENGTH - 1U] = ' ';
		CPollData poll(callsign, yourAddress, yourPort);
		m_incoming->writePoll(poll);
	} else {
		CConnectData reply(repeaterCallsign, reflectorCallsign, CT_NAK, yourAddress, yourPort);
		m_incoming->writeConnect(reply);

		printf("Could not create new CDExtraHandler, ignoring\n");
	}
}

void CDExtraHandler::link(IReflectorCallback *handler, const std::string &repeater, const std::string &gateway, const in_addr &address)
{
	CDExtraProtocolHandler *protoHandler = m_pool->getHandler();
	if (protoHandler == NULL)
		return;

	CDExtraHandler *dextra = new CDExtraHandler(handler, gateway, repeater, protoHandler, address, DEXTRA_PORT, DIR_OUTGOING);
	if (dextra) {
		m_reflectors.push_back(dextra);
		CConnectData reply(repeater, gateway, CT_LINK1, address, DEXTRA_PORT);
		protoHandler->writeConnect(reply);
	}
}

void CDExtraHandler::unlink(IReflectorCallback *handler, const std::string &callsign, bool exclude)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;

		bool found = false;

		if (exclude) {
			if (reflector->m_direction == DIR_OUTGOING && reflector->m_destination == handler && reflector->m_reflector.compare(callsign)) {
				printf("Removing outgoing DExtra link %s, %s\n", reflector->m_repeater.c_str(), reflector->m_reflector.c_str());

				if (reflector->m_linkState == DEXTRA_LINKING || reflector->m_linkState == DEXTRA_LINKED) {
					CConnectData connect(reflector->m_repeater, reflector->m_yourAddress, reflector->m_yourPort);
					reflector->m_handler->writeConnect(connect);

					reflector->m_linkState = DEXTRA_UNLINKING;

					reflector->m_destination->linkFailed(DP_DEXTRA, reflector->m_reflector, false);
				}

				found = true;
			}
		} else {
			if (reflector->m_destination == handler && 0==reflector->m_reflector.compare(callsign)) {
				printf("Removing DExtra link %s, %s\n", reflector->m_repeater.c_str(), reflector->m_reflector.c_str());

				if (reflector->m_linkState == DEXTRA_LINKING || reflector->m_linkState == DEXTRA_LINKED) {
					CConnectData connect(reflector->m_repeater, reflector->m_yourAddress, reflector->m_yourPort);
					reflector->m_handler->writeConnect(connect);

					reflector->m_linkState = DEXTRA_UNLINKING;

					reflector->m_destination->linkFailed(DP_DEXTRA, reflector->m_reflector, false);
				}

				found = true;
			}
		}

			// If an active link with incoming traffic, send an EOT to the repeater
		if (found) {
			if (reflector->m_dExtraId != 0x00U) {
				unsigned int seq = reflector->m_dExtraSeq + 1U;
				if (seq == 21U)
					seq = 0U;

				CAMBEData data;
				data.setData(END_PATTERN_BYTES, DV_FRAME_LENGTH_BYTES);
				data.setSeq(seq);
				data.setEnd(true);
				data.setId(reflector->m_dExtraId);

				reflector->m_destination->process(data, reflector->m_direction, AS_DEXTRA);
			}

			m_stateChange = true;

			delete reflector;
			it = m_reflectors.erase(it);
			it--;
		}
	}
}

void CDExtraHandler::unlink(CDExtraHandler *reflector)
{
	if (reflector != NULL) {
		if (reflector->m_repeater.size()) {
			printf("Unlinking from DExtra reflector %s\n", reflector->m_reflector.c_str());

			CConnectData connect(reflector->m_repeater, reflector->m_yourAddress, reflector->m_yourPort);
			reflector->m_handler->writeConnect(connect);

			reflector->m_linkState = DEXTRA_UNLINKING;
		}
	}
}

void CDExtraHandler::unlink()
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		CDExtraHandler::unlink(reflector);
	}
}

void CDExtraHandler::writeHeader(IReflectorCallback *handler, CHeaderData &header, DIRECTION direction)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		reflector->writeHeaderInt(handler, header, direction);
	}
}

void CDExtraHandler::writeAMBE(IReflectorCallback *handler, CAMBEData &data, DIRECTION direction)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		reflector->writeAMBEInt(handler, data, direction);
	}
}

void CDExtraHandler::gatewayUpdate(const std::string &reflector, const std::string &address)
{
	std::string gateway = reflector;
	gateway.resize(LONG_CALLSIGN_LENGTH - 1U, ' ');

	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		if (0==reflector->m_reflector.compare(0, LONG_CALLSIGN_LENGTH-1, gateway)) {
			if (address.size()) {
				// A new address, change the value
				printf("Changing IP address of DExtra gateway or reflector %s to %s\n", reflector->m_reflector.c_str(), address.c_str());
				reflector->m_yourAddress.s_addr = ::inet_addr(address.c_str());
			} else {
				printf("IP address for DExtra gateway or reflector %s has been removed\n", reflector->m_reflector.c_str());

				// No address, this probably shouldn't happen....
				if (reflector->m_direction == DIR_OUTGOING && reflector->m_destination != NULL)
					reflector->m_destination->linkFailed(DP_DEXTRA, reflector->m_reflector, false);

				m_stateChange = true;

				delete reflector;
				it = m_reflectors.erase(it);
				it--;
			}
		}
	}
}

void CDExtraHandler::clock(unsigned int ms)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); ) {
		CDExtraHandler *reflector = *it;
		bool ret = reflector->clockInt(ms);
		if (ret) {
			delete reflector;
			it = m_reflectors.erase(it);
		} else
			it++;
	}
}

void CDExtraHandler::finalise()
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); ) {
		CDExtraHandler *handler = *it;
		delete handler;
		it = m_reflectors.erase(it);
	}
}

void CDExtraHandler::processInt(CHeaderData& header)
{
	std::string     my = header.getMyCall1();
	std::string   rpt1 = header.getRptCall1();
	std::string   rpt2 = header.getRptCall2();
	unsigned int id = header.getId();

	if (m_whiteList != NULL) {
		bool res = m_whiteList->isInList(my);
		if (!res) {
			printf("%s rejected from DExtra as not found in the white list\n", my.c_str());
			m_dExtraId = 0x00U;
			return;
		}
	}

	if (m_blackList != NULL) {
		bool res = m_blackList->isInList(my);
		if (res) {
			printf("%s rejected from DExtra as found in the black list\n", my.c_str());
			m_dExtraId = 0x00U;
			return;
		}
	}

	if (m_linkState != DEXTRA_LINKED)
		return;

	switch (m_direction) {
		case DIR_OUTGOING: {
				// Always a repeater connection
				if (m_reflector.compare(rpt2) && m_reflector.compare(rpt1))
					return;

				// If we're already processing, ignore the new header
				if (m_dExtraId != 0x00U)
					return;

				m_dExtraId  = id;
				m_dExtraSeq = 0x00U;
				m_inactivityTimer.start();

				delete m_header;

				m_header = new CHeaderData(header);
				m_header->setCQCQCQ();
				m_header->setFlags(0x00U, 0x00U, 0x00U);

				m_destination->process(*m_header, m_direction, AS_DEXTRA);
			}
			break;

		case DIR_INCOMING:
			if (m_repeater.size()) {
				// A repeater connection
				if (m_repeater.compare(rpt2) && m_repeater.compare(rpt1))
					return;

				// If we're already processing, ignore the new header
				if (m_dExtraId != 0x00U)
					return;

				m_dExtraId  = id;
				m_dExtraSeq = 0x00U;
				m_inactivityTimer.start();

				delete m_header;

				m_header = new CHeaderData(header);
				m_header->setCQCQCQ();
				m_header->setFlags(0x00U, 0x00U, 0x00U);

				m_destination->process(*m_header, m_direction, AS_DEXTRA);
			} else {
				// A Dongle connection
				// Check the destination callsign
				m_destination = CRepeaterHandler::findDVRepeater(rpt2);
				if (m_destination == NULL) {
					m_destination = CRepeaterHandler::findDVRepeater(rpt1);
					if (m_destination == NULL)
						return;
				}

				// If we're already processing, ignore the new header
				if (m_dExtraId != 0x00U)
					return;

				m_dExtraId  = id;
				m_dExtraSeq = 0x00U;
				m_inactivityTimer.start();

				delete m_header;

				m_header = new CHeaderData(header);
				m_header->setCQCQCQ();
				m_header->setFlags(0x00U, 0x00U, 0x00U);

				m_destination->process(*m_header, m_direction, AS_DEXTRA);
			}
			break;
	}
}

void CDExtraHandler::processInt(CAMBEData &data)
{
	if (m_linkState != DEXTRA_LINKED)
		return;

	if (m_dExtraId != data.getId())
		return;

	m_pollInactivityTimer.start();
	m_inactivityTimer.start();

	m_dExtraSeq = data.getSeq();

	// Send the header every 21 frames, if we have it
	if (m_dExtraSeq == 0U && m_header != NULL)
		m_destination->process(*m_header, m_direction, AS_DUP);

	// Copy the data to ensure it remains unchanged
	CAMBEData temp(data);

	m_destination->process(temp, m_direction, AS_DEXTRA);

	if (temp.isEnd()) {
		delete m_header;
		m_header = NULL;

		m_dExtraId  = 0x00U;
		m_dExtraSeq = 0x00U;

		m_inactivityTimer.stop();
	}
}

bool CDExtraHandler::processInt(CConnectData &connect, CD_TYPE type)
{
	in_addr yourAddress   = connect.getYourAddress();
	unsigned int yourPort = connect.getYourPort();
	std::string  repeater = connect.getRepeater();

	if (m_yourAddress.s_addr != yourAddress.s_addr || m_yourPort != yourPort)
		return false;

	switch (type) {
		case CT_ACK:
			if (m_repeater.compare(repeater))
				return false;

			if (m_linkState == DEXTRA_LINKING) {
				printf("DExtra ACK message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkUp(DP_DEXTRA, m_reflector);

				m_tryTimer.stop();
				m_pollTimer.start();
				m_stateChange = true;
				m_linkState   = DEXTRA_LINKED;
			}

			return false;

		case CT_NAK:
			if (m_repeater.compare(repeater))
				return false;

			if (m_linkState == DEXTRA_LINKING) {
				printf("DExtra NAK message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkRefused(DP_DEXTRA, m_reflector);

				return true;
			}

			return false;

		case CT_UNLINK:
			if (m_reflector.compare(repeater))
				return false;

			if (m_linkState == DEXTRA_LINKED) {
				printf("DExtra disconnect message received from %s\n", m_reflector.c_str());

				if (m_direction == DIR_OUTGOING && m_destination != NULL)
					m_destination->linkFailed(DP_DEXTRA, m_reflector, false);

				m_stateChange = true;
			}

			return true;

		default:
			return false;
	}
}

bool CDExtraHandler::clockInt(unsigned int ms)
{
	m_tryTimer.clock(ms);
	m_pollTimer.clock(ms);
	m_inactivityTimer.clock(ms);
	m_pollInactivityTimer.clock(ms);

	if (m_pollInactivityTimer.isRunning() && m_pollInactivityTimer.hasExpired()) {
		m_pollInactivityTimer.start();

		delete m_header;
		m_header = NULL;

		m_stateChange = true;
		m_dExtraId    = 0x00U;
		m_dExtraSeq   = 0x00U;

		switch (m_linkState) {
			case DEXTRA_LINKING:
				printf("DExtra link to %s has failed to connect\n", m_reflector.c_str());
				break;
			case DEXTRA_LINKED:
				printf("DExtra link to %s has failed (poll inactivity)\n", m_reflector.c_str());
				break;
			case DEXTRA_UNLINKING:
				printf("DExtra link to %s has failed to disconnect cleanly\n", m_reflector.c_str());
				break;
			default:
				break;
		}

		if (m_direction == DIR_OUTGOING) {
			bool reconnect = m_destination->linkFailed(DP_DEXTRA, m_reflector, true);
			if (reconnect) {
				CConnectData reply(m_repeater, m_reflector, CT_LINK1, m_yourAddress, m_yourPort);
				m_handler->writeConnect(reply);
				m_linkState = DEXTRA_LINKING;
				m_tryTimer.start(1U);
				m_tryCount = 0U;
				return false;
			}
		}

		return true;
	}

	if (m_pollTimer.isRunning() && m_pollTimer.hasExpired()) {
		if (m_linkState == DEXTRA_LINKED) {
			if (m_repeater.size()) {
				std::string callsign = m_repeater;
				callsign[LONG_CALLSIGN_LENGTH - 1U] =' ';
				CPollData poll(callsign, m_yourAddress, m_yourPort);
				m_handler->writePoll(poll);
			} else {
				CPollData poll(m_callsign, m_yourAddress, m_yourPort);
				m_handler->writePoll(poll);
			}
		}

		m_pollTimer.start();
	}

	if (m_inactivityTimer.isRunning() && m_inactivityTimer.hasExpired()) {
		delete m_header;
		m_header = NULL;

		m_dExtraId  = 0x00U;
		m_dExtraSeq = 0x00U;

		m_inactivityTimer.stop();
	}

	if (m_linkState == DEXTRA_LINKING) {
		if (m_tryTimer.isRunning() && m_tryTimer.hasExpired()) {
			CConnectData reply(m_repeater, m_reflector, CT_LINK1, m_yourAddress, m_yourPort);
			m_handler->writeConnect(reply);

			unsigned int timeout = calcBackoff();
			m_tryTimer.start(timeout);
		}
	}

	return false;
}

void CDExtraHandler::writeHeaderInt(IReflectorCallback *handler, CHeaderData &header, DIRECTION direction)
{
	if (m_linkState != DEXTRA_LINKED)
		return;

	// Is it link in the right direction
	if (m_direction != direction)
		return;

	// Already in use?
	if (m_dExtraId != 0x00)
		return;

	switch (m_direction) {
		case DIR_OUTGOING:
			if (m_destination == handler) {
				header.setDestination(m_yourAddress, m_yourPort);
				m_handler->writeHeader(header);
			}
			break;

		case DIR_INCOMING:
			if (0==m_repeater.size() || m_destination == handler) {
				header.setDestination(m_yourAddress, m_yourPort);
				m_handler->writeHeader(header);
			}
			break;
	}
}

void CDExtraHandler::writeAMBEInt(IReflectorCallback *handler, CAMBEData &data, DIRECTION direction)
{
	if (m_linkState != DEXTRA_LINKED)
		return;

	// Is it link in the right direction
	if (m_direction != direction)
		return;

	// Already in use?
	if (m_dExtraId != 0x00)
		return;

	switch (m_direction) {
		case DIR_OUTGOING:
			if (m_destination == handler) {
				data.setDestination(m_yourAddress, m_yourPort);
				m_handler->writeAMBE(data);
			}
			break;

		case DIR_INCOMING:
			if (0==m_repeater.size() || m_destination == handler) {
				data.setDestination(m_yourAddress, m_yourPort);
				m_handler->writeAMBE(data);
			}
			break;
	}
}

bool CDExtraHandler::stateChange()
{
	bool stateChange = m_stateChange;

	m_stateChange = false;

	return stateChange;
}

void CDExtraHandler::writeStatus(FILE *file)
{
	for (auto it=m_reflectors.begin(); it!=m_reflectors.end(); it++) {
		CDExtraHandler *reflector = *it;
		struct tm *tm = ::gmtime(&reflector->m_time);

		switch (reflector->m_direction) {
			case DIR_OUTGOING:
				if (reflector->m_linkState == DEXTRA_LINKED) {
					fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: DExtra link - Type: Repeater Rptr: %s Refl: %s Dir: Outgoing\n",
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
						reflector->m_repeater.c_str(), reflector->m_reflector.c_str());
				}
				break;

			case DIR_INCOMING:
				if (reflector->m_linkState == DEXTRA_LINKED) {
					if (0==reflector->m_repeater.size())
						fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: DExtra link - Type: Dongle User: %s Dir: Incoming\n",
							tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
							reflector->m_reflector.c_str());
					else
						fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d: DExtra link - Type: Repeater Rptr: %s Refl: %s Dir: Incoming\n",
							tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
							reflector->m_repeater.c_str(), reflector->m_reflector.c_str());
				}
				break;
		}
	}
}

unsigned int CDExtraHandler::calcBackoff()
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
