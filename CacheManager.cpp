/*
 *   Copyright (c) 2020 by Thomas A. Early N7TAE
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

#include "CacheManager.h"
#include "DStarDefines.h"

bool CCacheManager::findUserData(const std::string &user, SUSERDATA &userdata)
{
	if (user.empty())
		return false;

	mux.lock();
	std::string rptr = findUserRptr(user);
	if (rptr.empty()) {
		mux.unlock();
		return false;
	}

	std::string gate = findRptrGate(rptr);
	std::string addr = findGateAddr(gate);
	mux.unlock();

	if (addr.empty())
		return false;

	userdata.rptr.assign(rptr);
	userdata.gate.assign(gate);
	userdata.addr.assign(addr);
	return true;
}

std::string CCacheManager::findUserAddr(const std::string &user)
{
	std::string addr;
	if (user.empty())
		return addr;

	mux.lock();
	std::string rptr(findUserRptr(user));
	if (rptr.empty()) {
		mux.unlock();
		return addr;
	}
	std::string gate(findRptrGate(rptr));
	addr.assign(findGateAddr(gate));
	mux.unlock();

	return addr;
}

std::string CCacheManager::findUserTime(const std::string &user)
{
	std::string utime;
	mux.lock();
	auto itt = UserTime.find(user);
	if (itt != UserTime.end())
		utime.assign(itt->second);
	mux.unlock();
	return utime;
}

std::string CCacheManager::findUserRptr(const std::string &user)
{
	std::string rptr;
	auto it = UserRptr.find(user);
	if (it != UserRptr.end())
		rptr.assign(it->second);
	return rptr;
}

std::string CCacheManager::findUserRepeater(const std::string &user)
{
	mux.lock();
	std::string rptr(findUserRptr(user));
	mux.unlock();
	return rptr;
}

std::string CCacheManager::findGateAddress(const std::string &gate)
{
	mux.lock();
	std::string addr(findGateAddr(gate));
	mux.unlock();
	return addr;
}

std::string CCacheManager::findRptrGate(const std::string &rptr)
{
	std::string gate;
	auto it = RptrGate.find(rptr);
	if (it == RptrGate.end()) {
		gate.assign(rptr);
		gate[7] = 'G';
	} else
		gate.assign(it->second);
	return gate;
}

std::string CCacheManager::findGateAddr(const std::string &gate)
{
	std::string addr;
	auto it6 = GateIPV6.find(gate);
	if (it6 == GateIPV6.end()) {
		auto it4 = GateIPV4.find(gate);
		if (it4 == GateIPV4.end()) {
			return addr;
		} else {
			addr.assign(it4->second);
		}
	} else
		addr.assign(it6->second);
	return addr;
}

void CCacheManager::updateUser(const std::string &user, const std::string &rptr, const std::string &gate, const std::string &addr, const std::string &time)
{
	if (user.empty())
		return;

	mux.lock();
	if (! time.empty())
		UserTime[user] = time;

	if (rptr.empty()) {
		mux.unlock();
		return;
	}

	UserRptr[user] = rptr;

	if (gate.empty() || addr.empty()) {
		mux.unlock();
		return;
	}

	if (rptr.compare(0, 7, gate, 0, 7))
		RptrGate[rptr] = gate;	// only do this if they differ

	if (addr.npos == addr.find(':'))
		GateIPV4[gate] = addr;
	else
		GateIPV6[gate] = addr;
	mux.unlock();
}

void CCacheManager::updateRptr(const std::string &rptr, const std::string &gate, const std::string &addr)
{
	if (rptr.empty() || gate.empty())
		return;

	mux.lock();
	RptrGate[rptr] = gate;
	if (addr.empty()) {
		mux.unlock();
		return;
	}
	if (addr.npos == addr.find(':'))
		GateIPV6[gate] = addr;
	else
		GateIPV4[gate] = addr;
	mux.unlock();
}

void CCacheManager::updateGate(const std::string &gate, const std::string &addr)
{
	if (gate.empty() || addr.empty())
		return;
	mux.lock();
	if (addr.npos == addr.find(':'))
		GateIPV4[gate] = addr;
	else
		GateIPV6[gate] = addr;
	mux.unlock();
}
