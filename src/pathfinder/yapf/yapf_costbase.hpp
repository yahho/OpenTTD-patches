/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_costbase.hpp Handling of cost determination. */

#ifndef YAPF_COSTBASE_HPP
#define YAPF_COSTBASE_HPP

#include "../../map/slope.h"

/** Base implementation for cost accounting. */
struct CYapfCostBase {
	/**
	 * Does the given track direction on the given tile yield an uphill penalty?
	 * @param tile The tile to check.
	 * @param td   The track direction to check.
	 * @return True if there's a slope, otherwise false.
	 */
	inline static bool stSlopeCost(const PFPos &pos)
	{
		if (!pos.InWormhole() && IsDiagonalTrackdir(pos.td)) {
			if (IsBridgeHeadTile(pos.tile)) {
				/* it is bridge ramp, check if we are entering the bridge */
				if (GetTunnelBridgeDirection(pos.tile) != TrackdirToExitdir(pos.td)) return false; // no, we are leaving it, no penalty
				/* we are entering the bridge */
				Slope tile_slope = GetTileSlope(pos.tile);
				Axis axis = DiagDirToAxis(GetTunnelBridgeDirection(pos.tile));
				return !HasBridgeFlatRamp(tile_slope, axis);
			} else {
				/* not bridge ramp */
				if (IsTunnelTile(pos.tile)) return false; // tunnel entry/exit doesn't slope
				Slope tile_slope = GetTileSlope(pos.tile);
				return IsUphillTrackdir(tile_slope, pos.td); // slopes uphill => apply penalty
			}
		}
		return false;
	}
};

#endif /* YAPF_COSTBASE_HPP */
