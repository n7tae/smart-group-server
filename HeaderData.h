/*
 *   Copyright (C) 2010-2014 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017,2020 by Thomas A. Early N7TAE
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
#include "DStarDefines.h"

class CHeaderData {
public:
	CHeaderData();
	CHeaderData(const CHeaderData &header);
	CHeaderData(const std::string &myCall1,  const std::string &myCall2, const std::string &yourCall, const std::string &rptCall1, const std::string &rptCall2, unsigned char flag1 = 0x00, unsigned char flag2 = 0x00, unsigned char flag3 = 0x00);
	~CHeaderData() {}

	bool setG2Data(const unsigned char *data, unsigned int length, bool check, const std::string &yourAddress, unsigned short yourPort);
	bool setDExtraData(const unsigned char *data, unsigned int length, bool check, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort);
	void setDCSData(const unsigned char *data, unsigned int length, const std::string &yourAddress, unsigned short yourPort, unsigned short myPort);

	unsigned int getDExtraData(unsigned char *data, unsigned int length, bool check) const;
	unsigned int getG2Data(unsigned char *data, unsigned int length, bool check) const;
	void         getDCSData(unsigned char *data, unsigned int length) const;

	bool setDVTOOLData(const unsigned char *data, unsigned int length, bool check);

	unsigned short getId() const;
	void setId(unsigned int id);

	unsigned char getBand1() const;
	unsigned char getBand2() const;
	unsigned char getBand3() const;
	void setBand1(unsigned char band);
	void setBand2(unsigned char band);
	void setBand3(unsigned char band);

	unsigned int getRptSeq() const;
	void setRptSeq(unsigned int seqNo);

	unsigned char getFlag1() const;
	unsigned char getFlag2() const;
	unsigned char getFlag3() const;

	std::string getMyCall1() const;
	std::string getMyCall2() const;
	std::string getYourCall() const;
	std::string getRptCall1() const;
	std::string getRptCall2() const;

	void setFlag1(unsigned char flag);
	void setFlag2(unsigned char flag);
	void setFlag3(unsigned char flag);
	void setFlags(unsigned char flag1, unsigned char flag2, unsigned char flag3);

	void setMyCall1(const std::string &callsign);
	void setMyCall2(const std::string &callsign);
	void setYourCall(const std::string &callsign);
	void setRptCall1(const std::string &callsign);
	void setRptCall2(const std::string &callsign);
	void setCQCQCQ();

	void setRepeaters(const std::string &rpt1, const std::string &rpt2);
	void setDestination(const std::string &address, unsigned short port);

	bool setData(const unsigned char *data, unsigned int length, bool check);
	unsigned int getData(unsigned char *data, unsigned int length, bool check) const;

	std::string    getYourAddress() const;
	unsigned short getYourPort() const;
	unsigned short getMyPort() const;

	unsigned int getErrors() const;

	static void initialise();
	static void finalise();
	static unsigned short createId();

	CHeaderData& operator=(const CHeaderData& header);

private:
	unsigned int   m_rptSeq;
	unsigned int   m_id;
	unsigned char  m_band1;
	unsigned char  m_band2;
	unsigned char  m_band3;
	unsigned char  m_flag1;
	unsigned char  m_flag2;
	unsigned char  m_flag3;
	std::string    m_yourAddress;
	unsigned short m_yourPort;
	unsigned short m_myPort;
	unsigned int   m_errors;
	unsigned char  m_myCall1[LONG_CALLSIGN_LENGTH];
	unsigned char  m_myCall2[SHORT_CALLSIGN_LENGTH];
	unsigned char  m_yourCall[LONG_CALLSIGN_LENGTH];
	unsigned char  m_rptCall1[LONG_CALLSIGN_LENGTH];	// this is the gateway
	unsigned char  m_rptCall2[LONG_CALLSIGN_LENGTH];	// this is the repeater
};
