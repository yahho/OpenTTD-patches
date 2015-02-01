/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file network_command.h Command handling over network connections. */

#ifndef NETWORK_COMMAND_H
#define NETWORK_COMMAND_H

#include "../command_type.h"
#include "core/packet.h"

/**
 * Everything we need to know about a command to be able to execute it.
 */
struct CommandPacket : CommandContainer {
	/** Make sure the pointer is NULL. */
	CommandPacket() : next(NULL), company(INVALID_COMPANY), frame(0), cmdsrc(CMDSRC_OTHER) {}
	CommandPacket *next;   ///< the next command packet (if in queue)
	CompanyID company;     ///< company that is executing the command
	uint32 frame;          ///< the frame in which this packet is executed
	CommandSource cmdsrc;  ///< source of the command

	void SendTo (Packet *p, bool from_server) const;

	bool ReceiveFrom (Packet *p, bool from_server, const char **err);
};

#endif /* NETWORK_COMMAND_H */
