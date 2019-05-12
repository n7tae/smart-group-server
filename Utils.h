/*
 *	Copyright (C) 2009,2013 by Jonathan Naylor, G4KLX
 *  Copyright (c) 2017 by Thomas A. Early N7TAE
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#pragma once

#include <sys/socket.h>
#include <string>
#include <vector>

enum TRISTATE {
	STATE_FALSE,
	STATE_TRUE,
	STATE_UNKNOWN
};

void					dump(const char* title, const bool* data, unsigned int length);
void					dumpRev(const char* title, const bool* data, unsigned int length);
void					dump(const char* title, const unsigned char* data, unsigned int length);
unsigned char			bitsToByte(const bool* bits);
unsigned char			bitsToByteRev(const bool* bits);
void					byteToBits(unsigned char byte, bool* bits);
void					byteToBitsRev(unsigned char byte, bool* bits);
std::string				latLonToLoc(double latitude, double longitude);
void					clean(std::string& str, const std::string& allowed);
std::string				ToUpper(std::string &str);
std::string				ToLower(std::string &str);
std::string				Trim(std::string &str);
void					safeStringCopy(char * dest, const char * src, unsigned int buf_size);
std::vector<std::string>	stringTokenizer(const std::string &s);
std::string				getCurrentTime(void);
void					ReplaceChar(std::string &str, char from, char to);
time_t					parseTime(const std::string str);
