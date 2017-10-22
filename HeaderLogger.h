/*
 *   Copyright (C) 2010-2013 by Jonathan Naylor G4KLX
 *   Copyright (c) 2017 by Thomas A. Early N7TAE
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
#include <cstdio>

#include "HeaderData.h"
#include "DDData.h"


class CHeaderLogger {
public:
	CHeaderLogger(const std::string& dir, const std::string& name = std::string(""));
	~CHeaderLogger();

	bool open();

	void write(const char* type, const CHeaderData& header);
	void write(const char* type, const CDDData& header);

	void close();

private:
	std::string m_dir;
	std::string m_name;
	FILE       *m_file;
};
