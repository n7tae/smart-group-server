// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include <string>
#include <map>

class CUserRecord {
public:
	CUserRecord(const std::string& user, const std::string& repeater, const std::string& timestamp) :
	m_user(user),
	m_repeater(repeater),
	m_timestamp(timestamp)
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

	std::string getTimeStamp() const
	{
		return m_timestamp;
	}

	void setRepeater(const std::string& repeater)
	{
		m_repeater = repeater;
	}

	void setTimestamp(const std::string& timestamp)
	{
		m_timestamp = timestamp;
	}

private:
	std::string m_user;
	std::string m_repeater;
	std::string m_timestamp;
};

class CUserCache {
public:
	CUserCache();
	~CUserCache();

	CUserRecord* find(const std::string& user);

	void update(const std::string& user, const std::string& repeater, const std::string& timestamp);

	unsigned int getCount() const;

private:
	std::map<std::string, CUserRecord *> m_cache;
};
