/*
 *   Copyright (C) 2010-2013 by Jonathan Naylor G4KLX
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
#include <string>
#include <cstring>
#include "AMBEData.h"
#include "Utils.h"

CAMBEData::CAMBEData() :
m_rptSeq(0U),
m_outSeq(0U),
m_id(0U),
m_band1(0x00U),
m_band2(0x02U),
m_band3(0x01U),
m_yourAddress(),
m_yourPort(0U),
m_myPort(0U),
m_errors(0U),
m_text(),
m_header()
{
}

CAMBEData::CAMBEData(const CAMBEData &data) :
m_rptSeq(data.m_rptSeq),
m_outSeq(data.m_outSeq),
m_id(data.m_id),
m_band1(data.m_band1),
m_band2(data.m_band2),
m_band3(data.m_band3),
m_yourAddress(data.m_yourAddress),
m_yourPort(data.m_yourPort),
m_myPort(data.m_myPort),
m_errors(data.m_errors),
m_text(data.m_text),
m_header(data.m_header)
{
	memcpy(m_data, data.m_data, DV_FRAME_LENGTH_BYTES);
}

CAMBEData::~CAMBEData()
{
}

bool CAMBEData::setG2Data(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort)
{
	assert(data != NULL);
	assert(length >= 27U);

	m_band1  = data[9];
	m_band2  = data[10];
	m_band3  = data[11];
	m_id     = data[12] * 256U + data[13];
	m_outSeq = data[14];

	memcpy(m_data, data + 15U, DV_FRAME_LENGTH_BYTES);

	m_yourAddress = yourAddress;
	m_yourPort    = yourPort;

	return true;
}

bool CAMBEData::setDExtraData(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort)
{
	assert(data != NULL);
	assert(length >= 27U);

	m_band1  = data[9];
	m_band2  = data[10];
	m_band3  = data[11];
	m_id     = data[12] * 256U + data[13];
	m_outSeq = data[14];

	memcpy(m_data, data + 15U, DV_FRAME_LENGTH_BYTES);

	m_yourAddress = yourAddress;
	m_yourPort    = yourPort;
	m_myPort      = myPort;

	return true;
}

bool CAMBEData::setDCSData(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort)
{
	assert(data != NULL);
	assert(length >= 100U);

	m_header.setDCSData(data, length, yourAddress, yourPort, myPort);

	m_id     = data[44] * 256U + data[43];

	m_outSeq = data[45];

	::memcpy(m_data, data + 46U, DV_FRAME_LENGTH_BYTES);

	m_rptSeq = data[60] * 65536U + data[59] * 256U + data[58];

	m_yourAddress = yourAddress;
	m_yourPort    = yourPort;
	m_myPort      = myPort;

	return true;
}

unsigned int CAMBEData::getG2Data(unsigned char *data, unsigned int length) const
{
	assert(data != NULL);
	assert(length >= 30U);

	data[0] = 'D';
	data[1] = 'S';
	data[2] = 'V';
	data[3] = 'T';

	data[4]  = 0x20;
	data[5]  = 0x00;
	data[6]  = 0x15;
	data[7]  = 0x09;
	data[8]  = 0x20;

	data[9]  = m_band1;
	data[10] = m_band2;
	data[11] = m_band3;

	data[12] = m_id / 256U;		// Unique session id
	data[13] = m_id % 256U;

	data[14] = m_outSeq;

	memcpy(data + 15U, m_data, DV_FRAME_LENGTH_BYTES);

	return 15U + DV_FRAME_LENGTH_BYTES;
}

unsigned int CAMBEData::getDExtraData(unsigned char* data, unsigned int length) const
{
	assert(data != NULL);
	assert(length >= 30U);

	data[0]  = 'D';
	data[1]  = 'S';
	data[2]  = 'V';
	data[3]  = 'T';

	data[4]  = 0x20;
	data[5]  = 0x00;
	data[6]  = 0x00;
	data[7]  = 0x00;
	data[8]  = 0x20;

	data[9]  = m_band1;
	data[10] = m_band2;
	data[11] = m_band3;

	data[12] = m_id % 256U;			// Unique session id
	data[13] = m_id / 256U;

	data[14] = m_outSeq;

	memcpy(data + 15U, m_data, DV_FRAME_LENGTH_BYTES);

	return 15U + DV_FRAME_LENGTH_BYTES;
}

unsigned int CAMBEData::getDCSData(unsigned char *data, unsigned int length) const
{
	assert(data != NULL);
	assert(length >= 100U);

	memset(data, 0x00U, 100U);

	data[0]  = '0';
	data[1]  = '0';
	data[2]  = '0';
	data[3]  = '1';

	data[43] = m_id % 256U;			// Unique session id
	data[44] = m_id / 256U;

	data[45] = m_outSeq;

	memcpy(data + 46U, m_data, DV_FRAME_LENGTH_BYTES);

	if (isEnd()) {
		data[55] = 0x55U;
		data[56] = 0x55U;
		data[57] = 0x55U;
	}

	data[58] = (m_rptSeq >> 0)  & 0xFFU;
	data[59] = (m_rptSeq >> 8)  & 0xFFU;
	data[60] = (m_rptSeq >> 16) & 0xFFU;

	data[61] = 0x01U;
	data[62] = 0x00U;

	data[63] = 0x21U;

	for (unsigned int i = 0U; i < m_text.size(); i++)
		data[64 + i] = m_text[i];

	m_header.getDCSData(data, 100U);

	return 100U;
}

unsigned short CAMBEData::getId() const
{
	return m_id;
}

void CAMBEData::setId(unsigned short id)
{
	m_id = id;
}

unsigned char CAMBEData::getBand1() const
{
	return m_band1;
}

unsigned char CAMBEData::getBand2() const
{
	return m_band2;
}

unsigned char CAMBEData::getBand3() const
{
	return m_band3;
}

void CAMBEData::setBand1(unsigned char band)
{
	m_band1 = band;
}

void CAMBEData::setBand2(unsigned char band)
{
	m_band2 = band;
}

void CAMBEData::setBand3(unsigned char band)
{
	m_band3 = band;
}

unsigned int CAMBEData::getRptSeq() const
{
	return m_rptSeq;
}

void CAMBEData::setRptSeq(unsigned int seqNo)
{
	m_rptSeq = seqNo;
}

unsigned int CAMBEData::getSeq() const
{
	return m_outSeq & 0x1FU;
}

void CAMBEData::setSeq(unsigned int seqNo)
{
	m_outSeq = seqNo;
}

bool CAMBEData::isEnd() const
{
	return (m_outSeq & 0x40U) == 0x40U;
}

void CAMBEData::setEnd(bool end)
{
	if (end)
		m_outSeq |= 0x40U;
	else
		m_outSeq &= ~0x40U;
}

bool CAMBEData::isSync() const
{
	return (m_outSeq & 0x1FU) == 0x00U;
}

void CAMBEData::setDestination(const std::string &address, unsigned short port)
{
	m_yourAddress = address;
	m_yourPort    = port;
}

void CAMBEData::setText(const std::string& text)
{
	m_text = text;
}

std::string CAMBEData::getYourAddress() const
{
	return m_yourAddress;
}

unsigned short CAMBEData::getYourPort() const
{
	return m_yourPort;
}

unsigned short CAMBEData::getMyPort() const
{
	return m_myPort;
}

CHeaderData& CAMBEData::getHeader()
{
	return m_header;
}

unsigned int CAMBEData::getErrors() const
{
	return m_errors;
}

void CAMBEData::setData(const unsigned char *data, unsigned int length)
{
	assert(data != NULL);
	assert(length >= DV_FRAME_LENGTH_BYTES);

	::memcpy(m_data, data, DV_FRAME_LENGTH_BYTES);
}

unsigned int CAMBEData::getData(unsigned char *data, unsigned int length) const
{
	assert(data != NULL);
	assert(length >= DV_FRAME_LENGTH_BYTES);

	::memcpy(data, m_data, DV_FRAME_LENGTH_BYTES);

	return DV_FRAME_LENGTH_BYTES;
}

CAMBEData& CAMBEData::operator=(const CAMBEData& data)
{
	if (&data != this) {
		m_rptSeq      = data.m_rptSeq;
		m_outSeq      = data.m_outSeq;
		m_id          = data.m_id;
		m_band1       = data.m_band1;
		m_band2       = data.m_band2;
		m_band3       = data.m_band3;
		m_yourAddress = data.m_yourAddress;
		m_yourPort    = data.m_yourPort;
		m_myPort      = data.m_myPort;
		m_errors      = data.m_errors;
		m_text        = data.m_text;
		m_header      = data.m_header;

		::memcpy(m_data, data.m_data, DV_FRAME_LENGTH_BYTES);
	}

	return *this;
}
