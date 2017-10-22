// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "DStarDefines.h"
#include "Defs.h"

class CGatewayRecord {
public:
	CGatewayRecord(const std::string& gateway, in_addr address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock) :
	m_gateway(gateway),
	m_address(address),
	m_protocol(DP_UNKNOWN),
	m_addrLock(addrLock),
	m_protoLock(false)
	{
		if (protocol != DP_UNKNOWN) {
			m_protocol  = protocol;
			m_protoLock = protoLock;
		}
	}

	std::string getGateway() const
	{
		return m_gateway;
	}

	in_addr getAddress() const
	{
		return m_address;
	}

	DSTAR_PROTOCOL getProtocol() const
	{
		return m_protocol;
	}

	void setData(in_addr address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock)
	{
		if (!m_addrLock) {
			m_address  = address;
			m_addrLock = addrLock;
		}

		if (!m_protoLock) {
			if (protocol != DP_UNKNOWN) {
				m_protocol  = protocol;
				m_protoLock = protoLock;
			}
		}
	}

private:
	std::string       m_gateway;
	in_addr        m_address;
	DSTAR_PROTOCOL m_protocol;
	bool           m_addrLock;
	bool           m_protoLock;
};

class CGatewayCache {
public:
	CGatewayCache();
	~CGatewayCache();

	CGatewayRecord* find(const std::string& gateway);

	void update(const std::string& gateway, const std::string& address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock);

	unsigned int getCount() const;

private:
	std::unordered_map<std::string, CGatewayRecord *> m_cache;
};
