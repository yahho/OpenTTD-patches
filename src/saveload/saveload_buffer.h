/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_buffer.h Saveload buffer declarations. */

#ifndef SAVELOAD_BUFFER_H
#define SAVELOAD_BUFFER_H

#include "../core/bitmath_func.hpp"
#include "../core/smallvec_type.hpp"
#include "saveload_filter.h"

/** A buffer for reading (and buffering) savegame data. */
struct LoadBuffer {
	static const size_t MEMORY_CHUNK_SIZE = 128 * 1024;
	byte buf[MEMORY_CHUNK_SIZE]; ///< Buffer we are reading from
	byte *bufp;                  ///< Current position within the buffer
	byte *bufe;                  ///< End of the buffer
	LoadFilter *reader;          ///< Downstream filter to read from
	size_t read;                 ///< Amount of bytes read so far from the filter

	/**
	 * Initialise our variables.
	 * @param reader The filter to read data from.
	 */
	LoadBuffer(LoadFilter *reader) : bufp(NULL), bufe(NULL), reader(reader), read(0)
	{
	}

	/**
	 * Get the amount of data in bytes read so far.
	 * @return The size.
	 */
	size_t GetSize() const
	{
		return this->read - (this->bufe - this->bufp);
	}

	void FillBuffer();

	/**
	 * Read a single byte from the buffer.
	 * @return The read byte.
	 */
	inline byte ReadByte()
	{
		if (this->bufp == this->bufe) {
			this->FillBuffer();
		}

		return *this->bufp++;
	}

	inline int ReadUint16()
	{
		int x = this->ReadByte() << 8;
		return x | this->ReadByte();
	}

	inline uint32 ReadUint32()
	{
		uint32 x = this->ReadUint16() << 16;
		return x | this->ReadUint16();
	}

	inline uint64 ReadUint64()
	{
		uint32 x = this->ReadUint32();
		uint32 y = this->ReadUint32();
		return (uint64)x << 32 | y;
	}

	/**
	 * Read in and discard bytes from the file
	 * @param length The amount of bytes to skip
	 */
	inline void Skip(size_t length)
	{
		while (length > (size_t)(this->bufe - this->bufp)) {
			length -= this->bufe - this->bufp;
			this->FillBuffer();
		}

		this->bufp += length;
	}
};

/** Container for dumping the savegame (quickly) to memory. */
struct SaveDumper {
	static const size_t MEMORY_CHUNK_SIZE = 128 * 1024;
	AutoFreeSmallVector<byte *, 16> blocks; ///< Buffer with blocks of allocated memory
	byte *buf;                              ///< Current position within the buffer
	byte *bufe;                             ///< End of the current buffer block

	/** Initialise our variables. */
	SaveDumper() : buf(NULL), bufe(NULL)
	{
	}

	/**
	 * Get the size of the memory dump made so far.
	 * @return The size.
	 */
	size_t GetSize() const
	{
		return this->blocks.Length() * MEMORY_CHUNK_SIZE - (this->bufe - this->buf);
	}

	void AllocBuffer();

	/**
	 * Write a single byte into the dumper.
	 * @param b The byte to write.
	 */
	inline void WriteByte(byte b)
	{
		/* Are we at the end of the current block? */
		if (this->buf == this->bufe) {
			this->AllocBuffer();
		}

		*this->buf++ = b;
	}

	inline void WriteUint16(uint16 v)
	{
		this->WriteByte(GB(v, 8, 8));
		this->WriteByte(GB(v, 0, 8));
	}

	inline void WriteUint32(uint32 v)
	{
		this->WriteUint16(GB(v, 16, 16));
		this->WriteUint16(GB(v,  0, 16));
	}

	inline void WriteUint64(uint64 x)
	{
		this->WriteUint32((uint32)(x >> 32));
		this->WriteUint32((uint32)x);
	}

	/**
	 * Flush this dumper into a writer.
	 * @param writer The filter we want to use.
	 */
	void Flush(SaveFilter *writer);
};

#endif /* SAVELOAD_BUFFER_H */
