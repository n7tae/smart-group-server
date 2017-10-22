// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include <string>
#include <unordered_map>

class CRepeaterRecord {
public:
	CRepeaterRecord(const std::string& repeater, const std::string& gateway) :
	m_repeater(repeater),
	m_gateway(gateway)
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

	void setGateway(const std::string& gateway)
	{
		m_gateway = gateway;
	}

private:
	std::string m_repeater;
	std::string m_gateway;
};

class CRepeaterCache {
public:
	CRepeaterCache();
	~CRepeaterCache();

	CRepeaterRecord* find(const std::string& repeater);

	void update(const std::string& repeater, const std::string& gateway);

	unsigned int getCount() const;

private:
	std::unordered_map<std::string, CRepeaterRecord *> m_cache;
};
