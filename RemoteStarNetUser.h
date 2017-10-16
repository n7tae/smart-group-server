// Copyright (c) 2017 by Thomas A. Early N7TAE

#pragma once

#include <string>

class CRemoteStarNetUser {
public:
	CRemoteStarNetUser(const std::string& callsign, uint32_t timer, uint32_t timeout);
	~CRemoteStarNetUser();

	std::string getCallsign() const;
	uint32_t getTimer() const;
	uint32_t getTimeout() const;

private:
	std::string  m_callsign;
	uint32_t m_timer;
	uint32_t m_timeout;
};
