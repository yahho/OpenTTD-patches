/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_buffer.cpp Saveload buffer definitions. */

#include "../stdafx.h"

#include "saveload_buffer.h"
#include "saveload.h"


void ReadBuffer::FillBuffer()
{
	assert(this->bufp == this->bufe);

	size_t len = this->reader->Read(this->buf, lengthof(this->buf));
	if (len == 0) SlErrorCorrupt("Unexpected end of stream");

	this->read += len;
	this->bufp = this->buf;
	this->bufe = this->buf + len;
}


void MemoryDumper::AllocBuffer()
{
	this->buf = CallocT<byte>(MEMORY_CHUNK_SIZE);
	*this->blocks.Append() = this->buf;
	this->bufe = this->buf + MEMORY_CHUNK_SIZE;
}

void MemoryDumper::Flush(SaveFilter *writer)
{
	uint i = 0;
	size_t t = this->GetSize();

	while (t > 0) {
		size_t to_write = min(MEMORY_CHUNK_SIZE, t);

		writer->Write(this->blocks[i++], to_write);
		t -= to_write;
	}

	writer->Finish();
}
