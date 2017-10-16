/*
 *	Copyright (C) 2009,2013 Jonathan Naylor, G4KLX
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

#include <cassert>
#include <cmath>
#include "Utils.h"
#include "Utilities.h"

void CUtils::dump(const char* title, const bool* data, unsigned int length)
{
	assert(title != NULL);
	assert(data != NULL);

	lprint("%s", title);

	unsigned int offset = 0U;

	while (offset < length) {
		std::string output;

		unsigned char buffer[16];
		unsigned int bytes = 0U;
		for (unsigned int bits = 0U; bits < 128U && (offset + bits) < length; bits += 8U)
			buffer[bytes++] = bitsToByte(data + offset + bits);

		for (unsigned i = 0U; i < bytes; i++) {
			char temp[4];
			sprintf(temp, "%02X ", buffer[i]);
			output += temp;
		}

		for (unsigned int i = bytes; i < 16U; i++)
			output += "   ";

		output += "   *";

		for (unsigned i = 0U; i < bytes; i++) {
			unsigned char c = buffer[i];

			if (::isprint(c))
				output += char(c);
			else
				output += ".";
		}

		output += "*'";

		lprint("%04X:  %s", offset / 8U, output.c_str());

		offset += 128U;
	}
}

void CUtils::dumpRev(const char* title, const bool* data, unsigned int length)
{
	assert(title != NULL);
	assert(data != NULL);

	lprint("%s", title);

	unsigned int offset = 0U;

	while (offset < length) {
		std::string output;

		unsigned char buffer[16];
		unsigned int bytes = 0U;
		for (unsigned int bits = 0U; bits < 128U && (offset + bits) < length; bits += 8U)
			buffer[bytes++] = bitsToByteRev(data + offset + bits);

		for (unsigned i = 0U; i < bytes; i++) {
			char temp[4];
			sprintf(temp, "%02X ", buffer[i]);
			output += temp;
		}

		for (unsigned int i = bytes; i < 16U; i++)
			output += "   ";

		output += "   *";

		for (unsigned i = 0U; i < bytes; i++) {
			unsigned char c = buffer[i];

			if (::isprint(c))
				output += char(c);
			else
				output += ".";
		}

		output += "*";

		lprint("%04X:  %s", offset / 8U, output.c_str());

		offset += 128U;
	}
}

void CUtils::dump(const char* title, const unsigned char* data, unsigned int length)
{
	assert(title != NULL);
	assert(data != NULL);

	lprint("%s", title);

	unsigned int offset = 0U;

	while (length > 0U) {
		std::string output;

		unsigned int bytes = (length > 16U) ? 16U : length;

		for (unsigned i = 0U; i < bytes; i++) {
			char temp[4];
			sprintf(temp, "%02X ", data[offset + i]);
			output += temp;
		}

		for (unsigned int i = bytes; i < 16U; i++)
			output += "   ";

		output += "   *";

		for (unsigned i = 0U; i < bytes; i++) {
			unsigned char c = data[offset + i];

			if (::isprint(c))
				output += char(c);
			else
				output += ".";
		}

		output += "*";

		lprint("%04X:  %s", offset, output.c_str());

		offset += 16U;

		if (length >= 16U)
			length -= 16U;
		else
			length = 0U;
	}
}

unsigned char CUtils::bitsToByte(const bool* bits)
{
	assert(bits != NULL);

	unsigned char val = 0x00;

	for (unsigned int i = 0U; i < 8U; i++) {
		val <<= 1;

		if (bits[i])
			val |= 0x01;
	}

	return val;
}

unsigned char CUtils::bitsToByteRev(const bool* bits)
{
	assert(bits != NULL);

	unsigned char val = 0x00;

	for (unsigned int i = 0U; i < 8U; i++) {
		val >>= 1;

		if (bits[i])
			val |= 0x80;
	}

	return val;
}

void CUtils::byteToBits(unsigned char byte, bool* data)
{
	assert(data != NULL);

	unsigned char mask = 0x80U;
	for (unsigned int i = 0U; i < 8U; i++, mask >>= 1)
		data[i] = byte & mask ? true : false;
}

void CUtils::byteToBitsRev(unsigned char byte, bool* data)
{
	assert(data != NULL);

	unsigned char mask = 0x01U;
	for (unsigned int i = 0U; i < 8U; i++, mask <<= 1)
		data[i] = byte & mask ? true : false;
}

std::string CUtils::latLonToLoc(double latitude, double longitude)
{
	if (latitude < -90.0 || latitude > 90.0)
		return std::string();

	if (longitude < -360.0 || longitude > 360.0)
		return std::string();

	latitude += 90.0;

	if (longitude > 180.0)
		longitude -= 360.0;

	if (longitude < -180.0)
		longitude += 360.0;

	longitude += 180.0;

	char locator[6U];

	double lon = floor(longitude / 20.0);
	double lat = floor(latitude / 10.0);

	locator[0U] = 'A' + (unsigned int)lon;
	locator[1U] = 'A' + (unsigned int)lat;

	longitude -= lon * 20.0;
	latitude  -= lat * 10.0;

	lon = ::floor(longitude / 2.0);
	lat = ::floor(latitude / 1.0);

	locator[2U] = '0' + (unsigned int)lon;
	locator[3U] = '0' + (unsigned int)lat;

	longitude -= lon * 2.0;
	latitude  -= lat * 1.0;

	lon = ::floor(longitude / (2.0 / 24.0));
	lat = ::floor(latitude / (1.0 / 24.0));

	locator[4U] = 'A' + (unsigned int)lon;
	locator[5U] = 'A' + (unsigned int)lat;

	return std::string(locator);
}

void CUtils::clean(std::string &str, const std::string& allowed)
{
	for (unsigned int i = 0U; i < str.size(); i++) {
		int n = allowed.find(str[i]);
		if (n < 0)
			str[i] = ' ';
	}
}
