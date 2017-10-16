// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include "UDPReaderWriter.h"
#include "DStarDefines.h"
#include "HeaderData.h"
#include "AMBEData.h"

enum G2_TYPE {
	GT_NONE,
	GT_HEADER,
	GT_AMBE
};

class CG2ProtocolHandler {
public:
	CG2ProtocolHandler(unsigned int port, const std::string& addr = std::string(""));
	~CG2ProtocolHandler();

	bool open();

	bool writeHeader(const CHeaderData& header);
	bool writeAMBE(const CAMBEData& data);

	G2_TYPE read();
	CHeaderData* readHeader();
	CAMBEData*   readAMBE();

	void close();

private:
	CUDPReaderWriter m_socket;
	G2_TYPE          m_type;
	unsigned char*   m_buffer;
	unsigned int     m_length;
	in_addr          m_address;
	unsigned int     m_port;

	bool readPackets();
};
