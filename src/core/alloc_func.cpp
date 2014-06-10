/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file alloc_func.cpp Functions to 'handle' memory allocation errors */

#include "../stdafx.h"

#include <stdlib.h>

/** Trigger an abort on out of memory. */
static void NORETURN out_of_memory (void)
{
	error ("Out of memory.");
}

/** Allocate uninitialised dynamic memory, and error out on failure. */
char *xmalloc (size_t size)
{
	if (size == 0) return NULL;
	void *p = malloc (size);
	if (p == NULL) out_of_memory();
	return (char*) p;
}

/** Allocate uninitialised dynamic memory, and error out on failure. */
void *xmalloc (size_t n, size_t size)
{
	if (n == 0 || size == 0) return NULL;
	size_t total = n * size;
	if (total / size != n) out_of_memory();
	return xmalloc (total);
}

/** Allocate zeroed dynamic memory, and error out on failure. */
void *xcalloc (size_t n, size_t size)
{
	if (n == 0 || size == 0) return NULL;
	void *p = calloc (n, size);
	if (p == NULL) out_of_memory();
	return p;
}


/**
 * Function to exit with an error message after malloc() or calloc() have failed
 * @param size number of bytes we tried to allocate
 */
void NORETURN MallocError(size_t size)
{
	error("Out of memory. Cannot allocate " PRINTF_SIZE " bytes", size);
}

/**
 * Function to exit with an error message after realloc() have failed
 * @param size number of bytes we tried to allocate
 */
void NORETURN ReallocError(size_t size)
{
	error("Out of memory. Cannot reallocate " PRINTF_SIZE " bytes", size);
}
