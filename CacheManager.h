// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include <string>
#include <mutex>

#include "RepeaterCache.h"
#include "GatewayCache.h"
#include "UserCache.h"

class CUserData {
public:
	CUserData(const std::string& user, const std::string& repeater, const std::string& gateway, in_addr address) :
	m_user(user),
	m_repeater(repeater),
	m_gateway(gateway),
	m_address(address)
	{
	}

	std::string getUser() const
	{
		return m_user;
	}

	std::string getRepeater() const
	{
		return m_repeater;
	}

	std::string getGateway() const
	{
		return m_gateway;
	}

	in_addr getAddress() const
	{
		return m_address;
	}

private:
	std::string m_user;
	std::string m_repeater;
	std::string m_gateway;
	in_addr  m_address;
};

class CRepeaterData {
public:
	CRepeaterData(const std::string& repeater, const std::string& gateway, in_addr address, DSTAR_PROTOCOL protocol) :
	m_repeater(repeater),
	m_gateway(gateway),
	m_address(address),
	m_protocol(protocol)
	{
	}

	std::string getRepeater() const
	{
		return m_repeater;
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

private:
	std::string    m_repeater;
	std::string    m_gateway;
	in_addr        m_address;
	DSTAR_PROTOCOL m_protocol;
};

class CGatewayData {
public:
	CGatewayData(const std::string& gateway, in_addr address, DSTAR_PROTOCOL protocol) :
	m_gateway(gateway),
	m_address(address),
	m_protocol(protocol)
	{
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

private:
	std::string    m_gateway;
	in_addr        m_address;
	DSTAR_PROTOCOL m_protocol;
};

class CCacheManager {
public:
	CCacheManager();
	~CCacheManager();

	CUserData*     findUser(const std::string& user);
	CGatewayData*  findGateway(const std::string& gateway);
	CRepeaterData* findRepeater(const std::string& repeater);

	void updateUser(const std::string& user, const std::string& repeater, const std::string& gateway, const std::string& address, const std::string& timeStamp, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock);
	void updateRepeater(const std::string& repeater, const std::string& gateway, const std::string& address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock);
	void updateGateway(const std::string& gateway, const std::string& address, DSTAR_PROTOCOL protocol, bool addrLock, bool protoLock);

private:
	CUserCache     m_userCache;
	CGatewayCache  m_gatewayCache;
	CRepeaterCache m_repeaterCache;
	std::mutex mux;
};
