/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file squirrel_std.hpp defines the Squirrel Standard Function class */

#ifndef SQUIRREL_STD_HPP
#define SQUIRREL_STD_HPP

#include "squirrel.hpp"

#if defined(__APPLE__)
/* Which idiotic system makes 'require' a macro? :s Oh well.... */
#undef require
#endif /* __APPLE__ */

/**
 * Register all standard functions we want to give to a script.
 */
void squirrel_register_std(Squirrel *engine);

/**
 * Register all standard functions that are available on first startup.
 * @note this set is very limited, and is only meant to load other scripts and things like that.
 */
void squirrel_register_global_std(Squirrel *engine);

#endif /* SQUIRREL_STD_HPP */
