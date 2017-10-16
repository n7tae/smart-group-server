// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include <string>
#include <vector>

#include "RemoteStarNetUser.h"

class CRemoteStarNetGroup {
public:
	CRemoteStarNetGroup(const std::string& callsign, const std::string& logoff, uint32_t timer, uint32_t timeout);
	~CRemoteStarNetGroup();

	void addUser(const std::string& callsign, uint32_t timer, uint32_t timeout);

	std::string getCallsign() const;
	std::string getLogoff() const;
	uint32_t getTimer() const;
	uint32_t getTimeout() const;

	uint32_t getUserCount() const;
	CRemoteStarNetUser *getUser(uint32_t n) const;

private:
	std::string m_callsign;
	std::string m_logoff;
	uint32_t m_timer;
	uint32_t m_timeout;
	std::vector<CRemoteStarNetUser *>  m_users;
};
