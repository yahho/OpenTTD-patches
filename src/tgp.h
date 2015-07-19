/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tgp.h Functions for the Perlin noise enhanced map generator. */

#ifndef TGP_H
#define TGP_H

static const uint TGP_HEIGHT_FACTOR = 8;

/**
 * Maximum index into array of noise amplitudes.
 */
static const int TGP_FREQUENCY_MAX = 9;

enum Smoothness {
	SMOOTHNESS_VERY_SMOOTH      = 0,
	SMOOTHNESS_SMOOTH           = 1,
	SMOOTHNESS_ROUGH            = 2,
	SMOOTHNESS_VERY_ROUGH       = 3,
	SMOOTHNESS_CEREALLY_ROUGH   = 4,
	SMOOTHNESS_COMMUTORZ        = 5,
	SMOOTHNESS_FLOWING          = 6,
};

void GenerateTerrainPerlin();

#endif /* TGP_H */
