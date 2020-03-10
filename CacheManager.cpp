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
	mux.lock();
	auto itr = UserRptr.find(user);
	if (itr == UserRptr.end()) {
		mux.unlock();
		return false;
	}

	auto itg = RptrGate.find(itr->second);
	if (itg == RptrGate.end()) {
		mux.unlock();
		return false;
	}

	auto ita = GateAddr.find(itg->second);
	if (ita == GateAddr.end()) {
		mux.unlock();
		return false;
	}

	userdata.rptr.assign(itr->second);
	userdata.gate.assign(itg->second);
	userdata.addr.assign(ita->second);
	mux.unlock();

	return true;
}

std::string CCacheManager::findUserAddr(const std::string &user)
{
	std::string rval;
	mux.lock();
	auto itr = UserRptr.find(user);
	if (itr == UserRptr.end()) {
		mux.unlock();
		return rval;
	}

	auto itg = RptrGate.find(itr->second);
	if (itg == RptrGate.end()) {
		mux.unlock();
		return rval;
	}

	auto ita = GateAddr.find(itg->second);
	if (ita == GateAddr.end()) {
		mux.unlock();
		return rval;
	}

	rval.assign(ita->second);
	mux.unlock();
	return rval;
}

std::string CCacheManager::findUserRptr(const std::string &user)
{
	std::string rval;
	mux.lock();
	auto it = UserRptr.find(user);
	if (it == UserRptr.end()) {
		mux.unlock();
		return rval;
	}
	rval.assign(it->second);
	mux.unlock();
	return rval;
}

std::string CCacheManager::findRptrGate(const std::string &rptr)
{
	std::string rval;
	mux.lock();
	auto it = RptrGate.find(rptr);
	if (it == RptrGate.end()) {
		mux.unlock();
		return rval;
	}
	rval.assign(it->second);
	mux.unlock();
	return rval;
}

std::string CCacheManager::findGateAddr(const std::string &gate)
{
	std::string rval;
	mux.lock();
	auto it = RptrGate.find(gate);
	if (it == RptrGate.end()) {
		mux.unlock();
		return rval;
	}
	rval.assign(it->second);
	mux.unlock();
	return rval;
}

void CCacheManager::updateUser(const std::string &user, const std::string &rptr, const std::string &gate, const std::string &addr, const std::string &time)
{
	mux.lock();
	UserRptr[user] = rptr;
	UserTime[user] = time;
	RptrGate[rptr] = gate;
	GateAddr[gate] = addr;
	mux.unlock();
}

void CCacheManager::updateRptr(const std::string &rptr, const std::string &gate, const std::string &addr)
{
	mux.lock();
	RptrGate[rptr] = gate;
	GateAddr[gate] = addr;
	mux.unlock();
}

void CCacheManager::updateGate(const std::string &gate, const std::string &addr)
{
	mux.lock();
	GateAddr[gate] = addr;
	mux.unlock();
}
