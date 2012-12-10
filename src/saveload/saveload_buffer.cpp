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
#include "saveload_internal.h"


void LoadBuffer::FillBuffer()
{
	size_t len = this->reader->Read(this->buf, lengthof(this->buf));
	if (len == 0) SlErrorCorrupt("Unexpected end of stream");

	this->read += len;
	this->bufp = this->buf;
	this->bufe = this->buf + len;
}

/**
 * Read in the header descriptor of an object or an array.
 * If the highest bit is set (7), then the index is bigger than 127
 * elements, so use the next byte to read in the real value.
 * The actual value is then both bytes added with the first shifted
 * 8 bits to the left, and dropping the highest bit (which only indicated a big index).
 * x = ((x & 0x7F) << 8) + ReadByte();
 * @return Return the value of the index
 */
uint LoadBuffer::ReadGamma()
{
	uint i = this->ReadByte();
	if (HasBit(i, 7)) {
		i &= ~0x80;
		if (HasBit(i, 6)) {
			i &= ~0x40;
			if (HasBit(i, 5)) {
				i &= ~0x20;
				if (HasBit(i, 4)) {
					SlErrorCorrupt("Unsupported gamma");
				}
				i = (i << 8) | this->ReadByte();
			}
			i = (i << 8) | this->ReadByte();
		}
		i = (i << 8) | this->ReadByte();
	}
	return i;
}

/**
 * Load a sequence of bytes.
 * @param ptr Load destination
 * @param length Number of bytes to copy
 */
void LoadBuffer::CopyBytes(void *ptr, size_t length)
{
	byte *p = (byte *)ptr;
	size_t diff;

	while (length > (diff = this->bufe - this->bufp)) {
		memcpy(p, this->bufp, diff);
		p += diff;
		length -= diff;
		this->FillBuffer();
	}

	memcpy(p, this->bufp, length);
	this->bufp += length;
}

/**
 * Read a value from the file, endian safely, and store it into the struct.
 * @param ptr The object being filled
 * @param conv VarType type of the current element of the struct
 */
void LoadBuffer::ReadVar(void *ptr, VarType conv)
{
	int64 x;
	/* Read a value from the file */
	switch (GetVarFileType(conv)) {
		case SLE_FILE_I8:  x = (int8  )this->ReadByte();   break;
		case SLE_FILE_U8:  x = (byte  )this->ReadByte();   break;
		case SLE_FILE_I16: x = (int16 )this->ReadUint16(); break;
		case SLE_FILE_U16: x = (uint16)this->ReadUint16(); break;
		case SLE_FILE_I32: x = (int32 )this->ReadUint32(); break;
		case SLE_FILE_U32: x = (uint32)this->ReadUint32(); break;
		case SLE_FILE_I64: x = (int64 )this->ReadUint64(); break;
		case SLE_FILE_U64: x = (uint64)this->ReadUint64(); break;
		case SLE_FILE_STRINGID: x = RemapOldStringID((uint16)this->ReadUint16()); break;
		default: NOT_REACHED();
	}

	/* Write The value to the struct. These ARE endian safe. */
	WriteValue(ptr, conv, x);
}


void SaveDumper::AllocBuffer()
{
	this->buf = CallocT<byte>(MEMORY_CHUNK_SIZE);
	*this->blocks.Append() = this->buf;
	this->bufe = this->buf + MEMORY_CHUNK_SIZE;
}

/**
 * Write the header descriptor of an object or an array.
 * If the element is bigger than 127, use 2 bytes for saving
 * and use the highest byte of the first written one as a notice
 * that the length consists of 2 bytes, etc.. like this:
 * 0xxxxxxx
 * 10xxxxxx xxxxxxxx
 * 110xxxxx xxxxxxxx xxxxxxxx
 * 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
 * @param i Index being written
 */

void SaveDumper::WriteGamma(size_t i)
{
	if (i >= (1 << 7)) {
		if (i >= (1 << 14)) {
			if (i >= (1 << 21)) {
				assert(i < (1 << 28));
				this->WriteByte((byte)(0xE0 | (i >> 24)));
				this->WriteByte((byte)(i >> 16));
			} else {
				this->WriteByte((byte)(0xC0 | (i >> 16)));
			}
			this->WriteByte((byte)(i >> 8));
		} else {
			this->WriteByte((byte)(0x80 | (i >> 8)));
		}
	}
	this->WriteByte((byte)i);
}

/**
 * Save a sequence of bytes.
 * @param ptr Save source
 * @param length Number of bytes to copy
 */
void SaveDumper::CopyBytes(const void *ptr, size_t length)
{
	const byte *p = (const byte *)ptr;
	size_t diff;

	while (length > (diff = this->bufe - this->buf)) {
		memcpy(this->buf, p, diff);
		p += diff;
		length -= diff;
		this->AllocBuffer();
	}

	memcpy(this->buf, p, length);
	this->buf += length;
}

void SaveDumper::Flush(SaveFilter *writer)
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
