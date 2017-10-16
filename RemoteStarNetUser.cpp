// Copyright (c) 2017 by Thomas A. Early N7TAE

#include "RemoteStarNetUser.h"

CRemoteStarNetUser::CRemoteStarNetUser(const std::string& callsign, uint32_t timer, uint32_t timeout) :
m_callsign(callsign),
m_timer(timer),
m_timeout(timeout)
{
}

CRemoteStarNetUser::~CRemoteStarNetUser()
{
}

std::string CRemoteStarNetUser::getCallsign() const
{
	return m_callsign;
}

uint32_t CRemoteStarNetUser::getTimer() const
{
	return (uint32_t)m_timer;
}

uint32_t CRemoteStarNetUser::getTimeout() const
{
	return (uint32_t)m_timeout;
}
