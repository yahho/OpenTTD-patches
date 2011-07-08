/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file misctile_cmd.cpp Handling of misc tiles. */

#include "stdafx.h"
#include "tile_cmd.h"


static void DrawTile_Misc(TileInfo *ti)
{
	NOT_REACHED();
}


static int GetSlopePixelZ_Misc(TileIndex tile, uint x, uint y)
{
	NOT_REACHED();
}


static CommandCost ClearTile_Misc(TileIndex tile, DoCommandFlag flags)
{
	NOT_REACHED();
}


static void GetTileDesc_Misc(TileIndex tile, TileDesc *td)
{
	NOT_REACHED();
}


static TrackStatus GetTileTrackStatus_Misc(TileIndex tile, TransportType mode, uint sub_mode, DiagDirection side)
{
	NOT_REACHED();
}


static bool ClickTile_Misc(TileIndex tile)
{
	NOT_REACHED();
}


static void TileLoop_Misc(TileIndex tile)
{
	NOT_REACHED();
}


static void ChangeTileOwner_Misc(TileIndex tile, Owner old_owner, Owner new_owner)
{
	NOT_REACHED();
}


static VehicleEnterTileStatus VehicleEnter_Misc(Vehicle *u, TileIndex tile, int x, int y)
{
	NOT_REACHED();
}


static Foundation GetFoundation_Misc(TileIndex tile, Slope tileh)
{
	NOT_REACHED();
}


static CommandCost TerraformTile_Misc(TileIndex tile, DoCommandFlag flags, int z_new, Slope tileh_new)
{
	NOT_REACHED();
}


extern const TileTypeProcs _tile_type_misc_procs = {
	DrawTile_Misc,           // draw_tile_proc
	GetSlopePixelZ_Misc,     // get_slope_z_proc
	ClearTile_Misc,          // clear_tile_proc
	NULL,                    // add_accepted_cargo_proc
	GetTileDesc_Misc,        // get_tile_desc_proc
	GetTileTrackStatus_Misc, // get_tile_track_status_proc
	ClickTile_Misc,          // click_tile_proc
	NULL,                    // animate_tile_proc
	TileLoop_Misc,           // tile_loop_proc
	ChangeTileOwner_Misc,    // change_tile_owner_proc
	NULL,                    // add_produced_cargo_proc
	VehicleEnter_Misc,       // vehicle_enter_tile_proc
	GetFoundation_Misc,      // get_foundation_proc
	TerraformTile_Misc,      // terraform_tile_proc
};
