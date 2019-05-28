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

#pragma once

#include <string>

#include <netinet/in.h>
#include "HeaderData.h"

class CAMBEData {
public:
	CAMBEData();
	CAMBEData(const CAMBEData &data);
	~CAMBEData();

	bool setG2Data(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort);
	bool setDExtraData(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort);
	bool setDCSData(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort);

	unsigned int getDExtraData(unsigned char *data, unsigned int length) const;
	unsigned int getDCSData(unsigned char *data, unsigned int length) const;
	unsigned int getG2Data(unsigned char *data, unsigned int length) const;

	unsigned short getId() const;
	void setId(unsigned short id);

	unsigned char getBand1() const;
	unsigned char getBand2() const;
	unsigned char getBand3() const;
	void setBand1(unsigned char band);
	void setBand2(unsigned char band);
	void setBand3(unsigned char band);

	unsigned int getRptSeq() const;
	void setRptSeq(unsigned int seqNo);

	unsigned int getSeq() const;
	void setSeq(unsigned int seqNo);

	bool isEnd() const;
	void setEnd(bool end);

	bool isSync() const;

	void setData(const unsigned char *data, unsigned int length);
	unsigned int getData(unsigned char *data, unsigned int length) const;

	void setDestination(const std::string &address, unsigned short port);

	void setText(const std::string &text);

	std::string    getYourAddress() const;
	unsigned short getYourPort() const;
	unsigned short getMyPort() const;

	unsigned int getErrors() const;

	CHeaderData &getHeader();

	CAMBEData& operator=(const CAMBEData &data);

private:
	unsigned int   m_rptSeq;
	unsigned char  m_outSeq;
	unsigned short m_id;
	unsigned char  m_band1;
	unsigned char  m_band2;
	unsigned char  m_band3;
	unsigned char *m_data;
	std::string    m_yourAddress;
	unsigned short m_yourPort;
	unsigned short m_myPort;
	unsigned int   m_errors;
	std::string    m_text;
	CHeaderData    m_header;
};
