/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_buffer.cpp Saveload buffer definitions. */

#include <list>

#include "../stdafx.h"
#include "../debug.h"
#include "../string_func.h"
#include "../network/network.h"
#include "../town.h"
#include "../vehicle_base.h"
#include "../station_base.h"
#include "../roadstop_base.h"
#include "../autoreplace_base.h"
#include "../linkgraph/linkgraph.h"
#include "../linkgraph/linkgraphjob.h"

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

/**
 * Load an array.
 * @param array The array being manipulated
 * @param length The length of the array in elements
 * @param conv VarType type of the atomic array (int, byte, uint64, etc.)
 */
void LoadBuffer::ReadArray(void *ptr, size_t length, VarType conv)
{
	extern uint16 _sl_version;

	/* NOTICE - handle some buggy stuff, in really old versions everything was saved
	 * as a byte-type. So detect this, and adjust array size accordingly */
	if (_sl_version == 0) {
		/* all arrays except difficulty settings */
		if (conv == SLE_INT16 || conv == SLE_UINT16 || conv == SLE_STRINGID ||
				conv == SLE_INT32 || conv == SLE_UINT32) {
			this->CopyBytes(ptr, length * SlCalcConvFileLen(conv));
			return;
		}
		/* used for conversion of Money 32bit->64bit */
		if (conv == (SLE_FILE_I32 | SLE_VAR_I64)) {
			for (uint i = 0; i < length; i++) {
				((int64*)ptr)[i] = (int32)BSWAP32(this->ReadUint32());
			}
			return;
		}
	}

	/* If the size of elements is 1 byte both in file and memory, no special
	 * conversion is needed, use specialized copy-copy function to speed up things */
	if (conv == SLE_INT8 || conv == SLE_UINT8) {
		this->CopyBytes(ptr, length);
	} else {
		byte *a = (byte*)ptr;
		byte mem_size = SlCalcConvMemLen(conv);

		for (; length != 0; length --) {
			this->ReadVar(a, conv);
			a += mem_size; // get size
		}
	}
}

/**
 * Load a list.
 * @param list The list being manipulated
 * @param conv SLRefType type of the list (Vehicle *, Station *, etc)
 */
void LoadBuffer::ReadList(void *ptr, SLRefType conv)
{
	std::list<void *> *l = (std::list<void *> *) ptr;

	size_t length = IsSavegameVersionBefore(69) ? this->ReadUint16() : this->ReadUint32();

	/* Load each reference and push to the end of the list */
	for (size_t i = 0; i < length; i++) {
		size_t data = IsSavegameVersionBefore(69) ? this->ReadUint16() : this->ReadUint32();
		l->push_back((void *)data);
	}
}

/**
 * Begin reading of a chunk
 */
void LoadBuffer::BeginChunk()
{
	byte m = this->ReadByte();
	switch (m) {
		case CH_ARRAY:
			this->array.index = 0;
			/* fall through */
		case CH_SPARSE_ARRAY:
			this->chunk_type = m;
			this->array.next = this->GetSize();
			break;
		default:
			if ((m & 0xF) != CH_RIFF) {
				SlErrorCorrupt("Invalid chunk type");
			}
			this->chunk_type = CH_RIFF;
			/* Read length */
			this->riff.length = (this->ReadByte() << 16) | ((m >> 4) << 24);
			this->riff.length += this->ReadUint16();
			this->riff.end = this->GetSize() + this->riff.length;
			break;
	}
}

/**
 * End reading of a chunk
 */
void LoadBuffer::EndChunk()
{
	if ((this->chunk_type == CH_RIFF) && (this->GetSize() != this->riff.end)) {
		SlErrorCorrupt("Invalid chunk size");
	}

	this->chunk_type = -1;
}

/**
 * Iterate through the elements of an array chunk
 * @param skip Whether to skip the whole chunk
 * @return The index of the element, or -1 if we have reached the end of current block
 */
int LoadBuffer::IterateChunk(bool skip)
{
	assert((this->chunk_type == CH_ARRAY) || (this->chunk_type == CH_SPARSE_ARRAY));

	/* Check that elements are fully read before going on with the next one. */
	if (this->GetSize() != this->array.next) SlErrorCorrupt("Invalid chunk size");

	for (;;) {
		uint length = this->ReadGamma();
		if (length == 0) {
			return -1;
		}

		this->array.size = --length;
		this->array.next = this->GetSize() + length;

		int index = (this->chunk_type == CH_SPARSE_ARRAY) ?
			(int)this->ReadGamma() : this->array.index++;

		if (length != 0) {
			if (!skip) return index;
			this->Skip(this->array.next - this->GetSize());
		}
	}
}

/**
 * Skip a whole chunk
 */
void LoadBuffer::SkipChunk()
{
	if (this->chunk_type == CH_RIFF) {
		assert(this->GetSize() == this->riff.end - this->riff.length);
		this->Skip(this->riff.length);
	} else {
		this->IterateChunk(true);
	}
}

bool LoadBuffer::ReadObjectMember(void *object, const SaveLoad *sld)
{
	/* CONDITIONAL saveload types depend on the savegame version */
	if (!SlIsObjectValidInSavegame(sld)) return false;

	if ((sld->flags & SLF_NO_NETWORK_SYNC) && _networking && !_network_server) {
		assert((sld->type == SL_ARR) || (sld->type == SL_STR));
		size_t skip = (sld->type == SL_STR) ?
			this->ReadGamma() : SlCalcConvFileLen(sld->conv);
		this->Skip(skip * sld->length);
		return false;
	}

	void *ptr = GetVariableAddress(sld, object);

	switch (sld->type) {
		case SL_VAR: this->ReadVar(ptr, sld->conv); break;
		case SL_REF: *(size_t *)ptr = IsSavegameVersionBefore(69) ? this->ReadUint16() : this->ReadUint32(); break;
		case SL_ARR: this->ReadArray(ptr, sld->length, sld->conv); break;
		case SL_STR: this->ReadString(ptr, sld->length, sld->conv); break;
		case SL_LST: this->ReadList(ptr, (SLRefType)sld->conv); break;

		case SL_WRITEBYTE:
			*(byte *)ptr = sld->conv;
			break;

		case SL_INCLUDE:
			this->ReadObject(object, (SaveLoad*)sld->address);
			break;

		default: NOT_REACHED();
	}
	return true;
}


void SaveDumper::AllocBuffer()
{
	this->buf = CallocT<byte>(this->alloc_size);
	*this->blocks.Append() = this->buf;
	this->bufe = this->buf + this->alloc_size;
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
 * Pointers cannot be saved to a savegame, so this functions gets
 * the index of the item and writes it into the buffer.
 * Remember that a NULL item has value 0, and all
 * indices have +1, so vehicle 0 is saved as index 1.
 * @param ptr The object that we want to get the index of
 * @param ref SLRefType type of the object the index is being sought of
 * @return Return the pointer converted to an index of the type pointed to
 */
void SaveDumper::WriteRef(const void *ptr, SLRefType ref)
{
	uint32 x;

	if (ptr == NULL) {
		x = 0;
	} else {
		switch (ref) {
			case REF_VEHICLE_OLD: // Old vehicles we save as new ones
			case REF_VEHICLE:   x = ((const  Vehicle*)ptr)->index; break;
			case REF_STATION:   x = ((const  Station*)ptr)->index; break;
			case REF_TOWN:      x = ((const     Town*)ptr)->index; break;
			case REF_ORDER:     x = ((const    Order*)ptr)->index; break;
			case REF_ROADSTOPS: x = ((const RoadStop*)ptr)->index; break;
			case REF_ENGINE_RENEWS:  x = ((const       EngineRenew*)ptr)->index; break;
			case REF_CARGO_PACKET:   x = ((const       CargoPacket*)ptr)->index; break;
			case REF_ORDERLIST:      x = ((const         OrderList*)ptr)->index; break;
			case REF_STORAGE:        x = ((const PersistentStorage*)ptr)->index; break;
			case REF_LINK_GRAPH:     x = ((const         LinkGraph*)ptr)->index; break;
			case REF_LINK_GRAPH_JOB: x = ((const      LinkGraphJob*)ptr)->index; break;
			default: NOT_REACHED();
		}
		x++;
	}

	this->WriteUint32(x);
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

/**
 * Save a string.
 * @param ptr the string being manipulated
 * @param length of the string (full length)
 * @param conv StrType type of the current element of the struct
 */
void SaveDumper::WriteString(const void *ptr, size_t length, StrType conv)
{
	const char *s;
	size_t len;

	if (conv & SLS_POINTER) {
		s = *(const char *const *)ptr;
		len = (s != NULL) ? strlen(s) : 0;
	} else {
		s = (const char *)ptr;
		len = ttd_strnlen(s, length - 1);
	}

	this->WriteGamma(len);
	this->CopyBytes(s, len);
}

/**
 * Save an array.
 * @param array The array being manipulated
 * @param length The length of the array in elements
 * @param conv VarType type of the atomic array (int, byte, uint64, etc.)
 */
void SaveDumper::WriteArray(const void *ptr, size_t length, VarType conv)
{
	/* If the size of elements is 1 byte both in file and memory, no special
	 * conversion is needed, use specialized copy-copy function to speed up things */
	if (conv == SLE_INT8 || conv == SLE_UINT8) {
		this->CopyBytes(ptr, length);
	} else {
		const byte *a = (const byte*)ptr;
		byte mem_size = SlCalcConvMemLen(conv);

		for (; length != 0; length --) {
			this->WriteVar(a, conv);
			a += mem_size; // get size
		}
	}
}

/**
 * Save a list.
 * @param list The list being manipulated
 * @param conv SLRefType type of the list (Vehicle *, Station *, etc)
 */
void SaveDumper::WriteList(const void *ptr, SLRefType conv)
{
	typedef const std::list<const void *> PtrList;
	PtrList *l = (PtrList *)ptr;

	this->WriteUint32(l->size());

	PtrList::const_iterator iter;
	for (iter = l->begin(); iter != l->end(); ++iter) {
		this->WriteRef(*iter, conv);
	}
}

/**
 * Begin writing of a chunk
 */
void SaveDumper::BeginChunk(uint type)
{
	this->chunk_type = type;

	switch (type) {
		case CH_RIFF:
			break;
		case CH_ARRAY:
			this->array_index = 0;
			/* fall through */
		case CH_SPARSE_ARRAY:
			this->WriteByte(type);
			break;
		default: NOT_REACHED();
	}
}

/**
 * End writing of a chunk
 */
void SaveDumper::EndChunk()
{
	if (this->chunk_type != CH_RIFF) {
		this->WriteGamma(0); // Terminate arrays
	}

	this->chunk_type = -1;
}

/**
 * Write next array element's header. On non-sparse arrays, it skips to the
 * given index and then writes its length. On sparse arrays, it writes both
 * length and index.
 * @param index Index of the element
 * @param length Size of its raw contents
 */
void SaveDumper::WriteElementHeader(uint index, size_t length)
{
	assert((this->chunk_type == CH_ARRAY) || (this->chunk_type == CH_SPARSE_ARRAY));

	if (this->chunk_type == CH_ARRAY) {
		assert(index >= this->array_index);
		while (++this->array_index <= index) {
			this->WriteGamma(1);
		}
		this->WriteGamma(length + 1);
	} else {
		this->WriteGamma(length + 1 + GetGammaLength(index));
		this->WriteGamma(index);
	}
}

void SaveDumper::WriteObjectMember(const void *object, const SaveLoad *sld)
{
	/* CONDITIONAL saveload types depend on the savegame version */
	if (!SlIsObjectValidInSavegame(sld)) return;

	const void *ptr = GetVariableAddress(sld, object);

	switch (sld->type) {
		case SL_VAR: this->WriteVar(ptr, sld->conv); break;
		case SL_REF: this->WriteRef(*(const void * const*)ptr, (SLRefType)sld->conv); break;
		case SL_ARR: this->WriteArray(ptr, sld->length, sld->conv); break;
		case SL_STR: this->WriteString(ptr, sld->length, sld->conv); break;
		case SL_LST: this->WriteList(ptr, (SLRefType)sld->conv); break;

		case SL_WRITEBYTE:
			this->WriteByte(sld->conv);
			break;

		case SL_INCLUDE:
			this->WriteObject(object, (SaveLoad*)sld->address);
			break;

		default: NOT_REACHED();
	}
}

void SaveDumper::Flush(SaveFilter *writer)
{
	uint i = 0;
	size_t t = this->GetSize();

	while (t > 0) {
		size_t to_write = min(this->alloc_size, t);

		writer->Write(this->blocks[i++], to_write);
		t -= to_write;
	}

	writer->Finish();
}
