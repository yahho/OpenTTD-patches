/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file command_func.h Functions related to commands. */

#ifndef COMMAND_FUNC_H
#define COMMAND_FUNC_H

#include "command_type.h"
#include "company_type.h"

/**
 * Define a default return value for a failed command.
 *
 * This variable contains a CommandCost object with is declared as "failed".
 * Other functions just need to return this error if there is an error,
 * which doesn't need to specific by a StringID.
 */
static const CommandCost CMD_ERROR = CommandCost(INVALID_STRING_ID);

/**
 * Returns from a function with a specific StringID as error.
 *
 * This macro is used to return from a function. The parameter contains the
 * StringID which will be returned.
 *
 * @param errcode The StringID to return
 */
#define return_cmd_error(errcode) return CommandCost(errcode);

/*!
 * This function executes a given command with the parameters from the #CommandProc parameter list.
 * Depending on the flags parameter it execute or test a command.
 *
 * @param tile The tile to apply the command on (for the #CommandProc)
 * @param p1 Additional data for the command (for the #CommandProc)
 * @param p2 Additional data for the command (for the #CommandProc)
 * @param flags Flags for the command and how to execute the command
 * @param cmd The command-id to execute (a value of the CMD_* enums)
 * @param text The text to pass
 * @see CommandProc
 * @return the cost
 */
static inline CommandCost DoCommand(TileIndex tile, uint32 p1, uint32 p2, DoCommandFlag flags, CommandID cmd, const char *text = NULL)
{
	const Command c (tile, p1, p2, cmd, text);

	return c.exec (flags);
}

/*!
 * Toplevel network safe docommand function for the current company. Must not be called recursively.
 * The parameters \a tile, \a p1, and \a p2 are from the #CommandProc function.
 * The parameter \a cmd is the command to execute.
 *
 * @param tile The tile to perform a command on (see #CommandProc)
 * @param p1 Additional data for the command (see #CommandProc)
 * @param p2 Additional data for the command (see #CommandProc)
 * @param cmd The command to execute (a CMD_* value)
 * @param text The text to pass
 * @return \c true if the command succeeded, else \c false.
 */
static inline bool DoCommandP(TileIndex tile, uint32 p1, uint32 p2, CommandID cmd, const char *text = NULL)
{
	Command c (tile, p1, p2, cmd, text);

	return c.execp ();
}

/*!
 * Helper function for the toplevel network safe docommand function for the current company.
 *
 * @param tile The tile to perform a command on (see #CommandProc)
 * @param p1 Additional data for the command (see #CommandProc)
 * @param p2 Additional data for the command (see #CommandProc)
 * @param cmd The command to execute (a CMD_* value)
 * @param text The text to pass
 * @param estimate_only whether to give only the estimate or also execute the command
 * @param cmdsrc Source of the command
 * @return the command cost of this function.
 */
static inline CommandCost DoCommandPInternal(TileIndex tile, uint32 p1, uint32 p2, CommandID cmd, const char *text, bool estimate_only, CommandSource cmdsrc = CMDSRC_SELF)
{
	Command c (tile, p1, p2, cmd, text);

	return c.execp_internal (estimate_only, cmdsrc);
}

#ifdef ENABLE_NETWORK
void NetworkSendCommand (const Command *cc, CompanyID company, CommandSource cmdsrc = CMDSRC_SELF);

/**
 * Prepare a DoCommand to be send over the network
 * @param tile The tile to perform a command on (see #CommandProc)
 * @param p1 Additional data for the command (see #CommandProc)
 * @param p2 Additional data for the command (see #CommandProc)
 * @param cmd The command to execute (a CMD_* value)
 * @param text The text to pass
 * @param company The company that wants to send the command
 */
static inline void NetworkSendCommand (TileIndex tile, uint32 p1, uint32 p2, CommandID cmd, const char *text, CompanyID company)
{
	Command c (tile, p1, p2, cmd, text);

	NetworkSendCommand (&c, company);
}
#endif /* ENABLE_NETWORK */

extern Money _additional_cash_required;

bool IsValidCommand(CommandID cmd);
CommandFlags GetCommandFlags(CommandID cmd);
const char *GetCommandName(CommandID cmd);
Money GetAvailableMoneyForCommand();
bool IsCommandAllowedWhilePaused(CommandID cmd);

/**
 * Extracts the DC flags needed for DoCommand from the flags returned by GetCommandFlags
 * @param cmd_flags Flags from GetCommandFlags
 * @return flags for DoCommand
 */
static inline DoCommandFlag CommandFlagsToDCFlags(CommandFlags cmd_flags)
{
	DoCommandFlag flags = DC_NONE;
	if (cmd_flags & CMDF_NO_WATER) flags |= DC_NO_WATER;
	if (cmd_flags & CMDF_AUTO) flags |= DC_AUTO;
	if (cmd_flags & CMDF_ALL_TILES) flags |= DC_ALL_TILES;
	return flags;
}

/*** All command callbacks that exist ***/

/* airport_gui.cpp */
CommandCallback CcBuildAirport;

/* bridge_gui.cpp */
CommandCallback CcBuildBridge;

/* dock_gui.cpp */
CommandCallback CcBuildDocks;
CommandCallback CcBuildCanal;

/* depot_gui.cpp */
CommandCallback CcCloneVehicle;

/* group_gui.cpp */
CommandCallback CcCreateGroup;
CommandCallback CcAddVehicleGroup;

/* industry_gui.cpp */
CommandCallback CcBuildIndustry;

/* main_gui.cpp */
CommandCallback CcPlaySound10;
CommandCallback CcPlaceSign;
CommandCallback CcTerraform;
CommandCallback CcTerraformLand;
CommandCallback CcGiveMoney;

/* object_gui.cpp */
CommandCallback CcBuildObject;

/* rail_gui.cpp */
CommandCallback CcPlaySound1E;
CommandCallback CcSingleRail;
CommandCallback CcRailDepot;
CommandCallback CcStation;

/* road_gui.cpp */
CommandCallback CcPlaySound1D;
CommandCallback CcBuildTunnel;
CommandCallback CcRoadDepot;
CommandCallback CcRoadStop;

/* town_gui.cpp */
CommandCallback CcFoundTown;

/* vehicle_gui.cpp */
CommandCallback CcBuildVehicle;
CommandCallback CcStartStopVehicle;

#endif /* COMMAND_FUNC_H */
