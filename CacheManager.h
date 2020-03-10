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

#pragma once

#include <string>
#include <mutex>
#include <unordered_map>

using SUSERDATA = struct userdata_tag {
	std::string rptr, gate, addr;
};

class CCacheManager {
public:
	CCacheManager() {}
	~CCacheManager() {}

	bool findUserData(const std::string &user, SUSERDATA &userdata);
	std::string findUserRptr(const std::string &user);
	std::string findUserTime(const std::string &user);
	std::string findRptrGate(const std::string &rptr);
	std::string findGateAddr(const std::string &gate);
	std::string findUserAddr(const std::string &user);

	void updateUser(const std::string &user, const std::string &rptr, const std::string &gate, const std::string &addr, const std::string &time);
	void updateRptr(const std::string &rptr, const std::string &gate, const std::string &addr);
	void updateGate(const std::string &gate, const std::string &addr);

private:
	std::unordered_map<std::string, std::string> UserTime;
	std::unordered_map<std::string, std::string> UserRptr;
	std::unordered_map<std::string, std::string> RptrGate;
	std::unordered_map<std::string, std::string> GateAddr;
	std::mutex mux;
};
