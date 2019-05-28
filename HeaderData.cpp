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

#include <ctime>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include "HeaderData.h"

#include "CCITTChecksum.h"
#include "DStarDefines.h"
#include "Utils.h"

void CHeaderData::initialise()
{
	srand(time(NULL));
}

void CHeaderData::finalise()
{
}

unsigned short CHeaderData::createId()
{
    int rval = 0;
    while (0 == rval) {
        rval = rand() % 0xffff;
    }
	return (unsigned short)rval;
}

CHeaderData::CHeaderData() :
m_rptSeq(0U),
m_id(0U),
m_band1(0x00U),
m_band2(0x02U),
m_band3(0x01U),
m_flag1(0U),
m_flag2(0U),
m_flag3(0U),
m_myCall1(NULL),
m_myCall2(NULL),
m_yourCall(NULL),
m_rptCall1(NULL),
m_rptCall2(NULL),
m_yourAddress(),
m_yourPort(0U),
m_myPort(0U),
m_errors(0U)
{
	m_myCall1  = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_myCall2  = new unsigned char[SHORT_CALLSIGN_LENGTH];
	m_yourCall = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_rptCall1 = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_rptCall2 = new unsigned char[LONG_CALLSIGN_LENGTH];

	::memset(m_rptCall1, ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_rptCall2, ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_yourCall, ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_myCall1,  ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_myCall2,  ' ', SHORT_CALLSIGN_LENGTH);
}

CHeaderData::CHeaderData(const CHeaderData& header) :
m_rptSeq(header.m_rptSeq),
m_id(header.m_id),
m_band1(header.m_band1),
m_band2(header.m_band2),
m_band3(header.m_band3),
m_flag1(header.m_flag1),
m_flag2(header.m_flag2),
m_flag3(header.m_flag3),
m_myCall1(NULL),
m_myCall2(NULL),
m_yourCall(NULL),
m_rptCall1(NULL),
m_rptCall2(NULL),
m_yourAddress(header.m_yourAddress),
m_yourPort(header.m_yourPort),
m_myPort(header.m_myPort),
m_errors(header.m_errors)
{
	m_myCall1  = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_myCall2  = new unsigned char[SHORT_CALLSIGN_LENGTH];
	m_yourCall = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_rptCall1 = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_rptCall2 = new unsigned char[LONG_CALLSIGN_LENGTH];

	::memcpy(m_myCall1,  header.m_myCall1,  LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall2,  header.m_myCall2,  SHORT_CALLSIGN_LENGTH);
	::memcpy(m_yourCall, header.m_yourCall, LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall1, header.m_rptCall1, LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall2, header.m_rptCall2, LONG_CALLSIGN_LENGTH);
}

CHeaderData::CHeaderData(const std::string& myCall1,  const std::string& myCall2, const std::string& yourCall, const std::string& rptCall1, const std::string& rptCall2, unsigned char flag1, unsigned char flag2, unsigned char flag3) :
m_rptSeq(0U),
m_id(0U),
m_band1(0U),
m_band2(0U),
m_band3(0U),
m_flag1(flag1),
m_flag2(flag2),
m_flag3(flag3),
m_myCall1(NULL),
m_myCall2(NULL),
m_yourCall(NULL),
m_rptCall1(NULL),
m_rptCall2(NULL),
m_yourAddress(),
m_yourPort(0U),
m_myPort(0U),
m_errors(0U)
{
	m_myCall1  = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_myCall2  = new unsigned char[SHORT_CALLSIGN_LENGTH];
	m_yourCall = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_rptCall1 = new unsigned char[LONG_CALLSIGN_LENGTH];
	m_rptCall2 = new unsigned char[LONG_CALLSIGN_LENGTH];

	::memset(m_myCall1,  ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_myCall2,  ' ', SHORT_CALLSIGN_LENGTH);
	::memset(m_yourCall, ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_rptCall1, ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_rptCall2, ' ', LONG_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < myCall1.size() && i < LONG_CALLSIGN_LENGTH; i++)
		m_myCall1[i] = myCall1[i];

	for (unsigned int i = 0U; i < myCall2.size() && i < SHORT_CALLSIGN_LENGTH; i++)
		m_myCall2[i] = myCall2[i];

	for (unsigned int i = 0U; i < yourCall.size() && i < LONG_CALLSIGN_LENGTH; i++)
		m_yourCall[i] = yourCall[i];

	for (unsigned int i = 0U; i < rptCall1.size() && i < LONG_CALLSIGN_LENGTH; i++)
		m_rptCall1[i] = rptCall1[i];

	for (unsigned int i = 0U; i < rptCall2.size() && i < LONG_CALLSIGN_LENGTH; i++)
		m_rptCall2[i] = rptCall2[i];
}

CHeaderData::~CHeaderData()
{
	delete[] m_myCall1;
	delete[] m_myCall2;
	delete[] m_yourCall;
	delete[] m_rptCall1;
	delete[] m_rptCall2;
}

void CHeaderData::setDCSData(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort)
{
	assert(data != NULL);
	assert(length >= 100U);

	m_id = data[44U] * 256U + data[43U];

	m_flag1 = data[4U];
	m_flag2 = data[5U];
	m_flag3 = data[6U];

	::memcpy(m_rptCall2, data + 7U,  LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall1, data + 15U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_yourCall, data + 23U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall1,  data + 31U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall2,  data + 39U, SHORT_CALLSIGN_LENGTH);

	m_yourAddress = yourAddress;
	m_yourPort    = yourPort;
	m_myPort      = myPort;
}

bool CHeaderData::setG2Data(const unsigned char *data, unsigned int length, bool check, const std::string &yourAddress, unsigned short yourPort)
{
	assert(data != NULL);
	assert(length >= 56U);

	m_band1 = data[9];
	m_band2 = data[10];
	m_band3 = data[11];
	m_id    = data[12] * 256U + data[13];

	m_flag1 = data[15U];
	m_flag2 = data[16U];
	m_flag3 = data[17U];

	::memcpy(m_rptCall2, data + 18U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall1, data + 26U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_yourCall, data + 34U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall1,  data + 42U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall2,  data + 50U, SHORT_CALLSIGN_LENGTH);

	m_yourAddress = yourAddress;
	m_yourPort    = yourPort;

	if (check) {
		CCCITTChecksum cksum;
		cksum.update(data + 15U, RADIO_HEADER_LENGTH_BYTES - 2U);
		bool valid = cksum.check(data + 15U + RADIO_HEADER_LENGTH_BYTES - 2U);

		if (!valid)
			dump("Header checksum failure from G2", data + 15U, RADIO_HEADER_LENGTH_BYTES);

		return valid;
	} else {
		return true;
	}
}

bool CHeaderData::setDExtraData(const unsigned char *data, unsigned int length, bool check, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort)
{
	assert(data != NULL);
	assert(length >= 56U);

	m_band1  = data[9];
	m_band2  = data[10];
	m_band3  = data[11];
	m_id     = data[12] * 256U + data[13];

	m_flag1 = data[15U];
	m_flag2 = data[16U];
	m_flag3 = data[17U];

	::memcpy(m_rptCall2, data + 18U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall1, data + 26U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_yourCall, data + 34U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall1,  data + 42U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall2,  data + 50U, SHORT_CALLSIGN_LENGTH);

	m_yourAddress = yourAddress;
	m_yourPort    = yourPort;
	m_myPort      = myPort;

	if (check) {
		CCCITTChecksum cksum;
		cksum.update(data + 15U, RADIO_HEADER_LENGTH_BYTES - 2U);
		bool valid = cksum.check(data + 15U + RADIO_HEADER_LENGTH_BYTES - 2U);

		if (!valid)
			dump("Header checksum failure from DExtra", data + 15U, RADIO_HEADER_LENGTH_BYTES);

		return valid;
	} else {
		return true;
	}
}

bool CHeaderData::setDVTOOLData(const unsigned char *data, unsigned int length, bool check)
{
	assert(data != NULL);
	assert(length >= RADIO_HEADER_LENGTH_BYTES);

	m_flag1 = data[0U];
	m_flag2 = data[1U];
	m_flag3 = data[2U];

	::memcpy(m_rptCall2, data + 3U,  LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall1, data + 11U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_yourCall, data + 19U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall1,  data + 27U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall2,  data + 35U, SHORT_CALLSIGN_LENGTH);

	if (check) {
		CCCITTChecksum cksum;
		cksum.update(data, RADIO_HEADER_LENGTH_BYTES - 2U);
		bool valid = cksum.check(data + RADIO_HEADER_LENGTH_BYTES - 2U);

		if (!valid)
			dump("Header checksum failure from DVTOOL", data, RADIO_HEADER_LENGTH_BYTES);

		return valid;
	} else {
		return true;
	}
}

void CHeaderData::getDCSData(unsigned char *data, unsigned int length) const
{
	assert(data != NULL);
	assert(length >= 100U);

	data[4] = m_flag1;				// Flags 1, 2, and 3
	data[5] = m_flag2;
	data[6] = m_flag3;

	::memcpy(data + 7U,  m_rptCall2, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 15U, m_rptCall1, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 23U, m_yourCall, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 31U, m_myCall1,  LONG_CALLSIGN_LENGTH);
	::memcpy(data + 39U, m_myCall2,  SHORT_CALLSIGN_LENGTH);
}

unsigned int CHeaderData::getG2Data(unsigned char *data, unsigned int length, bool check) const
{
	assert(data != NULL);
	assert(length >= 56U);

	data[0] = 'D';
	data[1] = 'S';
	data[2] = 'V';
	data[3] = 'T';

	data[4]  = 0x10;
	data[5]  = 0x00;
	data[6]  = 0x15;
	data[7]  = 0x09;
	data[8]  = 0x20;

	data[9]  = m_band1;
	data[10] = m_band2;
	data[11] = m_band3;

	data[12] = m_id / 256U;			// Unique session id
	data[13] = m_id % 256U;

	data[14] = 0x80;

	data[15] = m_flag1;				// Flags 1, 2, and 3
	data[16] = m_flag2;
	data[17] = m_flag3;

	::memcpy(data + 18U, m_rptCall2, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 26U, m_rptCall1, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 34U, m_yourCall, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 42U, m_myCall1,  LONG_CALLSIGN_LENGTH);
	::memcpy(data + 50U, m_myCall2,  SHORT_CALLSIGN_LENGTH);

	if (check) {
		CCCITTChecksum csum;
		csum.update(data + 15, 4U * LONG_CALLSIGN_LENGTH + SHORT_CALLSIGN_LENGTH + 3U);
		csum.result(data + 54);
	} else {
		data[54] = 0xFF;
		data[55] = 0xFF;
	}

	return 56U;
}

unsigned int CHeaderData::getDExtraData(unsigned char* data, unsigned int length, bool check) const
{
	assert(data != NULL);
	assert(length >= 56U);

	data[0]  = 'D';
	data[1]  = 'S';
	data[2]  = 'V';
	data[3]  = 'T';

	data[4]  = 0x10;
	data[5]  = 0x00;
	data[6]  = 0x00;
	data[7]  = 0x00;
	data[8]  = 0x20;

	data[9]  = m_band1;
	data[10] = m_band2;
	data[11] = m_band3;

	data[12] = m_id % 256U;			// Unique session id
	data[13] = m_id / 256U;

	data[14] = 0x80;

	data[15] = 0x00;				// Flags 1, 2, and 3
	data[16] = 0x00;
	data[17] = 0x00;

	::memcpy(data + 18U, m_rptCall2, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 26U, m_rptCall1, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 34U, m_yourCall, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 42U, m_myCall1,  LONG_CALLSIGN_LENGTH);
	::memcpy(data + 50U, m_myCall2,  SHORT_CALLSIGN_LENGTH);

	if (check) {
		CCCITTChecksum csum;
		csum.update(data + 15, 4U * LONG_CALLSIGN_LENGTH + SHORT_CALLSIGN_LENGTH + 3U);
		csum.result(data + 54);
	} else {
		data[54] = 0xFF;
		data[55] = 0xFF;
	}

	return 56U;
}

unsigned short CHeaderData::getId() const
{
	return m_id;
}

void CHeaderData::setId(unsigned int id)
{
	m_id = id;
}

unsigned char CHeaderData::getBand1() const
{
	return m_band1;
}

unsigned char CHeaderData::getBand2() const
{
	return m_band2;
}

unsigned char CHeaderData::getBand3() const
{
	return m_band3;
}

void CHeaderData::setBand1(unsigned char band)
{
	m_band1 = band;
}

void CHeaderData::setBand2(unsigned char band)
{
	m_band2 = band;
}

void CHeaderData::setBand3(unsigned char band)
{
	m_band3 = band;
}

unsigned int CHeaderData::getRptSeq() const
{
	return m_rptSeq;
}

void CHeaderData::setRptSeq(unsigned int seqNo)
{
	m_rptSeq = seqNo;
}

unsigned char CHeaderData::getFlag1() const
{
	return m_flag1;
}

unsigned char CHeaderData::getFlag2() const
{
	return m_flag2;
}

unsigned char CHeaderData::getFlag3() const
{
	return m_flag3;
}

void CHeaderData::setFlags(unsigned char flag1, unsigned char flag2, unsigned char flag3)
{
	m_flag1 = flag1;
	m_flag2 = flag2;
	m_flag3 = flag3;
}

std::string CHeaderData::getMyCall1() const
{
	return std::string((const char*)m_myCall1, LONG_CALLSIGN_LENGTH);
}

std::string CHeaderData::getMyCall2() const
{
	return std::string((const char*)m_myCall2, SHORT_CALLSIGN_LENGTH);
}

std::string CHeaderData::getYourCall() const
{
	return std::string((const char*)m_yourCall, LONG_CALLSIGN_LENGTH);
}

std::string CHeaderData::getRptCall1() const
{
	return std::string((const char*)m_rptCall1, LONG_CALLSIGN_LENGTH);
}

std::string CHeaderData::getRptCall2() const
{
	return std::string((const char*)m_rptCall2, LONG_CALLSIGN_LENGTH);
}

void CHeaderData::setFlag1(unsigned char flag)
{
	m_flag1 = flag;
}

void CHeaderData::setFlag2(unsigned char flag)
{
	m_flag2 = flag;
}

void CHeaderData::setFlag3(unsigned char flag)
{
	m_flag3 = flag;
}

void CHeaderData::setMyCall1(const std::string& my1)
{
	::memset(m_myCall1, ' ', LONG_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < my1.size(); i++)
		m_myCall1[i] = my1[i];
}

void CHeaderData::setMyCall2(const std::string& my2)
{
	::memset(m_myCall2, ' ', SHORT_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < my2.size(); i++)
		m_myCall2[i] = my2[i];
}

void CHeaderData::setYourCall(const std::string& your)
{
	::memset(m_yourCall, ' ', LONG_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < your.size(); i++)
		m_yourCall[i] = your[i];
}

void CHeaderData::setRptCall1(const std::string& rpt1)
{
	::memset(m_rptCall1, ' ', LONG_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < rpt1.size(); i++)
		m_rptCall1[i] = rpt1[i];
}

void CHeaderData::setRptCall2(const std::string& rpt2)
{
	::memset(m_rptCall2, ' ', LONG_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < rpt2.size(); i++)
		m_rptCall2[i] = rpt2[i];
}

void CHeaderData::setRepeaters(const std::string& rpt1, const std::string& rpt2)
{
	::memset(m_rptCall1, ' ', LONG_CALLSIGN_LENGTH);
	::memset(m_rptCall2, ' ', LONG_CALLSIGN_LENGTH);

	for (unsigned int i = 0U; i < rpt1.size(); i++)
		m_rptCall1[i] = rpt1[i];

	for (unsigned int i = 0U; i < rpt2.size(); i++)
		m_rptCall2[i] = rpt2[i];
}

void CHeaderData::setCQCQCQ()
{
	::memcpy(m_yourCall, "CQCQCQ  ", LONG_CALLSIGN_LENGTH);
}

bool CHeaderData::setData(const unsigned char *data, unsigned int length, bool check)
{
	assert(data != NULL);
	assert(length >= RADIO_HEADER_LENGTH_BYTES);

	m_flag1 = data[0U];
	m_flag2 = data[1U];
	m_flag3 = data[2U];

	::memcpy(m_rptCall2, data + 3U,  LONG_CALLSIGN_LENGTH);
	::memcpy(m_rptCall1, data + 11U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_yourCall, data + 19U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall1,  data + 27U, LONG_CALLSIGN_LENGTH);
	::memcpy(m_myCall2,  data + 35U, SHORT_CALLSIGN_LENGTH);

	if (check) {
		CCCITTChecksum cksum;
		cksum.update(data, RADIO_HEADER_LENGTH_BYTES - 2U);
		return cksum.check(data + RADIO_HEADER_LENGTH_BYTES - 2U);
	} else {
		return true;
	}
}

unsigned int CHeaderData::getData(unsigned char *data, unsigned int length, bool check) const
{
	assert(data != NULL);
	assert(length >= RADIO_HEADER_LENGTH_BYTES);

	data[0] = m_flag1;				// Flags 1, 2, and 3
	data[1] = m_flag2;
	data[2] = m_flag3;

	::memcpy(data + 3U,  m_rptCall2, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 11U, m_rptCall1, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 19U, m_yourCall, LONG_CALLSIGN_LENGTH);
	::memcpy(data + 27U, m_myCall1,  LONG_CALLSIGN_LENGTH);
	::memcpy(data + 35U, m_myCall2,  SHORT_CALLSIGN_LENGTH);

	if (check) {
		CCCITTChecksum csum;
		csum.update(data, RADIO_HEADER_LENGTH_BYTES - 2U);
		csum.result(data + RADIO_HEADER_LENGTH_BYTES - 2U);

		return RADIO_HEADER_LENGTH_BYTES;
	} else {
		return RADIO_HEADER_LENGTH_BYTES - 2U;
	}
}

void CHeaderData::setDestination(const std::string &address, unsigned short port)
{
	m_yourAddress = address;
	m_yourPort    = port;
}

std::string CHeaderData::getYourAddress() const
{
	return m_yourAddress;
}

unsigned short CHeaderData::getYourPort() const
{
	return m_yourPort;
}

unsigned short CHeaderData::getMyPort() const
{
	return m_myPort;
}

CHeaderData& CHeaderData::operator =(const CHeaderData& header)
{
	if (&header != this) {
		m_rptSeq      = header.m_rptSeq;
		m_id          = header.m_id;
		m_band1       = header.m_band1;
		m_band2       = header.m_band2;
		m_band3       = header.m_band3;
		m_flag1       = header.m_flag1;
		m_flag2       = header.m_flag2;
		m_flag3       = header.m_flag3;
		m_yourAddress = header.m_yourAddress;
		m_yourPort    = header.m_yourPort;
		m_myPort      = header.m_myPort;
		m_errors      = header.m_errors;

		::memcpy(m_myCall1,  header.m_myCall1,  LONG_CALLSIGN_LENGTH);
		::memcpy(m_myCall2,  header.m_myCall2,  SHORT_CALLSIGN_LENGTH);
		::memcpy(m_yourCall, header.m_yourCall, LONG_CALLSIGN_LENGTH);
		::memcpy(m_rptCall1, header.m_rptCall1, LONG_CALLSIGN_LENGTH);
		::memcpy(m_rptCall2, header.m_rptCall2, LONG_CALLSIGN_LENGTH);
	}

	return *this;
}
