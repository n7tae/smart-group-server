/*
 *   Copyright (C) 2010-2014 by Jonathan Naylor G4KLX
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

#include <cassert>

#include "GroupHandler.h"
#include "G2Handler.h"
#include "Utils.h"
#include "Defs.h"

CG2Handler::CG2Handler()
{
}

CG2Handler::~CG2Handler()
{
}

void CG2Handler::process(CHeaderData& header)
{
	// Is this a busy reply?
	unsigned char flag1 = header.getFlag1();
	if (flag1 == 0x01) {
		// Don't check the incoming stream
		// printf("G2 busy message received\n"));
		return;
	}

	// Check to see if this is for Smart Group
	CGroupHandler* handler = CGroupHandler::findGroup(header);
	if (handler != NULL) {
		handler->process(header);
		return;
	}
}

void CG2Handler::process(CAMBEData& data)
{
	// Check to see if this is for Smart Group
	CGroupHandler* handler = CGroupHandler::findGroup(data);
	if (handler != NULL) {
		handler->process(data);
		return;
	}
}
