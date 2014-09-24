/*
	see copyright notice in squirrel.h
*/

#include "../../../stdafx.h"
#include "../../../core/alloc_func.hpp"

#include "sqpcheader.h"

void *sq_vm_malloc (SQUnsignedInteger size)
{
	return xmalloc ((size_t)size);
}

void *sq_vm_realloc (void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{
	return xrealloc (p, (size_t)size);
}

void sq_vm_free (void *p, SQUnsignedInteger size)
{
	free (p);
}
