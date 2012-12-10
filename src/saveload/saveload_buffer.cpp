/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_buffer.cpp Saveload buffer definitions. */

#include "../stdafx.h"
#include "../debug.h"
#include "../string_func.h"

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

/**
 * Load a string.
 * @param ptr the string being manipulated
 * @param length of the string (full length)
 * @param conv StrType type of the current element of the struct
 */
void LoadBuffer::ReadString(void *ptr, size_t length, StrType conv)
{
	size_t len = this->ReadGamma();

	if ((conv & SLS_POINTER) != 0) { // Malloc'd string, free previous incarnation, and allocate
		char **pptr = (char **)ptr;
		free(*pptr);
		if (len == 0) {
			*pptr = NULL;
			return;
		} else {
			*pptr = MallocT<char>(len + 1); // terminating '\0'
			ptr = *pptr;
			this->CopyBytes(ptr, len);
		}
	} else {
		if (len >= length) {
			DEBUG(sl, 1, "String length in savegame is bigger than buffer, truncating");
			this->CopyBytes(ptr, length);
			this->Skip(len - length);
			len = length - 1;
		} else {
			this->CopyBytes(ptr, len);
		}
	}

	((char *)ptr)[len] = '\0'; // properly terminate the string
	StringValidationSettings settings = SVS_REPLACE_WITH_QUESTION_MARK;
	if ((conv & SLS_ALLOW_CONTROL) != 0) {
		settings = settings | SVS_ALLOW_CONTROL_CODE;
		if (IsSavegameVersionBefore(169)) {
			str_fix_scc_encoded((char *)ptr, (char *)ptr + len);
		}
	}
	if ((conv & SLS_ALLOW_NEWLINE) != 0) {
		settings = settings | SVS_ALLOW_NEWLINE;
	}
	str_validate((char *)ptr, (char *)ptr + len, settings);
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

/**
 * Read in the actual value from the struct and then write them to file,
 * endian safely.
 * @param ptr The object being saved
 * @param conv VarType type of the current element of the struct
 */
void SaveDumper::WriteVar(const void *ptr, VarType conv)
{
	int64 x = ReadValue(ptr, conv);

	/* Write the value to the file and check if its value is in the desired range */
	switch (GetVarFileType(conv)) {
		case SLE_FILE_I8: assert(x >= -128 && x <= 127);     this->WriteByte(x);break;
		case SLE_FILE_U8: assert(x >= 0 && x <= 255);        this->WriteByte(x);break;
		case SLE_FILE_I16:assert(x >= -32768 && x <= 32767); this->WriteUint16(x);break;
		case SLE_FILE_STRINGID:
		case SLE_FILE_U16:assert(x >= 0 && x <= 65535);      this->WriteUint16(x);break;
		case SLE_FILE_I32:
		case SLE_FILE_U32:                                   this->WriteUint32((uint32)x);break;
		case SLE_FILE_I64:
		case SLE_FILE_U64:                                   this->WriteUint64(x);break;
		default: NOT_REACHED();
	}
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
