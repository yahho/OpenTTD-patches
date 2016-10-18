/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file flexarray.h Flexible array support. */

#ifndef FLEXARRAY_H
#define FLEXARRAY_H

/** Flexible array base struct to delete several methods. */
struct FlexArrayBase {
protected:
	FlexArrayBase (void)
	{
	}

private:
	FlexArrayBase (const FlexArrayBase &) DELETED;

	FlexArrayBase & operator = (const FlexArrayBase &) DELETED;

	void *operator new (size_t size) DELETED;
};

/** Flexible array base struct to provide custom operator new. */
template <typename T>
struct FlexArray : FlexArrayBase {
protected:
	/** Custom operator new to account for the variable-length buffer. */
	void *operator new (size_t size, size_t extra1, size_t extra2 = 1)
	{
		return ::operator new (size + extra1 * extra2 * sizeof(T));
	}
};

#endif /* FLEXARRAY_H */
