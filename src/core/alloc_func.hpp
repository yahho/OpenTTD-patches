/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file alloc_func.hpp Functions related to the allocation of memory */

#ifndef ALLOC_FUNC_HPP
#define ALLOC_FUNC_HPP

char *xmalloc (size_t size);
void *xmalloc (size_t n, size_t size);

/** Allocate uninitialised dynamic memory for a given type, and error out on failure. */
template <typename T>
static inline T *xmalloct (size_t n = 1)
{
	if (sizeof(T) == 1) {
		return (T*) xmalloc (n);
	} else {
		return (T*) xmalloc (n, sizeof(T));
	}
}

void *xcalloc (size_t n, size_t size);

/** Allocate zeroed dynamic memory for a given type, and error out on failure. */
template <typename T>
static inline T *xcalloct (size_t n = 1)
{
	return (T*) xcalloc (n, sizeof(T));
}

char *xrealloc (void *p, size_t size);
void *xrealloc (void *p, size_t n, size_t size);

/** Reallocate dynamic memory for a given type, and error out on failure. */
template <typename T>
static inline T *xrealloct (T *p, size_t n)
{
	if (sizeof(T) == 1) {
		return (T*) xrealloc (p, n);
	} else {
		return (T*) xrealloc (p, n, sizeof(T));
	}
}

/**
 * Checks whether allocating memory would overflow size_t.
 *
 * @param element_size Size of the structure to allocate.
 * @param num_elements Number of elements to allocate.
 */
static inline void alloca_check (size_t element_size, size_t num_elements)
{
	/* alloca is not the right thing to use way before we reach this limit */
	assert (num_elements < SIZE_MAX / element_size);
}

/** alloca() has to be called in the parent function, so define AllocaM() as a macro */
#define AllocaM(T, num_elements) \
	(alloca_check (sizeof(T), num_elements), \
	(T*)alloca((num_elements) * sizeof(T)))

#endif /* ALLOC_FUNC_HPP */
