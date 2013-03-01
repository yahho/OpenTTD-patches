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
#include "saveload_data.h"

/** Type and flags of a chunk. */
enum ChunkType {
	CH_RIFF         =  0,
	CH_ARRAY        =  1,
	CH_SPARSE_ARRAY =  2,
	CH_TYPE_MASK    =  3,
	CH_LAST         =  8, ///< Last chunk in this array.
};

/** A buffer for reading (and buffering) savegame data. */
struct LoadBuffer {
	static const size_t MEMORY_CHUNK_SIZE = 128 * 1024;
	byte buf[MEMORY_CHUNK_SIZE]; ///< Buffer we are reading from
	byte *bufp;                  ///< Current position within the buffer
	byte *bufe;                  ///< End of the buffer
	LoadFilter *reader;          ///< Downstream filter to read from
	size_t read;                 ///< Amount of bytes read so far from the filter
	uint chunk_type;             ///< The type of the current chunk
	union {
		struct {
			size_t length; ///< The length of the current chunk
			size_t end;    ///< End offset of the current chunk
		} riff;
		struct {
			size_t size;   ///< Current array element size
			size_t next;   ///< Next element offset
			int index;     ///< Current array index for (non-sparse) arrays
		} array;
	};
	const SavegameTypeVersion *stv; ///< type and version of the savegame

	/**
	 * Initialise our variables.
	 * @param reader The filter to read data from.
	 */
	LoadBuffer(LoadFilter *reader, const SavegameTypeVersion *stv)
		: bufp(NULL), bufe(NULL), reader(reader), read(0), chunk_type(-1), stv(stv)
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

	inline bool IsOTTDVersionBefore(uint16 major, byte minor = 0) const
	{
		return IsOTTDSavegameVersionBefore(this->stv, major, minor);
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

	uint ReadGamma();

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

	void CopyBytes(void *ptr, size_t length);

	inline size_t ReadRef()
	{
		return this->IsOTTDVersionBefore(69) ? this->ReadUint16() : this->ReadUint32();
	}

	void ReadVar(void *ptr, VarType conv);
	void ReadString(void *ptr, size_t length, StrType conv);
	void ReadArray(void *ptr, size_t length, VarType conv);
	void ReadList(void *ptr, SLRefType conv);

	void BeginChunk();
	void EndChunk();

	/**
	 * Return the size of the current RIFF chunk.
	 * @return The size of the current RIFF chunk
	 */
	inline size_t GetChunkSize() const
	{
		assert(this->chunk_type == CH_RIFF);
		return this->riff.length;
	}

	int IterateChunk(bool skip = false);

	/**
	 * Return the size of the current array element.
	 * @return The size of the current array element
	 */
	inline size_t GetElementSize() const
	{
		assert((this->chunk_type == CH_ARRAY) || (this->chunk_type == CH_SPARSE_ARRAY));
		return this->array.size;
	}

	void SkipChunk();

	bool ReadObjectMember(void *object, const SaveLoad *sld);

	/**
	 * Main load function.
	 * @param ptr The object that is being loaded
	 * @param sld The SaveLoad description of the object so we know how to manipulate it
	 */
	void ReadObject(void *object, const SaveLoad *sld)
	{
		for (; sld->type != SL_END; sld++) {
			this->ReadObjectMember(object, sld);
		}
	}
};

/** Container for dumping the savegame (quickly) to memory. */
struct SaveDumper {
	static const size_t DEFAULT_ALLOC_SIZE = 128 * 1024;
	AutoFreeSmallVector<byte *, 16> blocks; ///< Buffer with blocks of allocated memory
	byte *buf;                              ///< Current position within the buffer
	byte *bufe;                             ///< End of the current buffer block
	const size_t alloc_size;                ///< Block allocation size
	uint chunk_type;                        ///< The type of the current save chunk
	uint array_index;                       ///< Next array index for (non-sparse) arrays

	/** Initialise our variables. */
	SaveDumper(size_t as = DEFAULT_ALLOC_SIZE) : buf(NULL), bufe(NULL), alloc_size(as), chunk_type(-1)
	{
	}

	/**
	 * Get the size of the memory dump made so far.
	 * @return The size.
	 */
	size_t GetSize() const
	{
		return this->blocks.Length() * this->alloc_size - (this->bufe - this->buf);
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

	void WriteGamma(size_t i);

	void CopyBytes(const void *ptr, size_t length);

	void WriteRef(const void *ptr, SLRefType ref);

	void WriteVar(const void *ptr, VarType conv);
	void WriteString(const void *ptr, size_t length, StrType conv);
	void WriteArray(const void *ptr, size_t length, VarType conv);
	void WriteList(const void *ptr, SLRefType conv);

	void BeginChunk(uint type);
	void EndChunk();

	void WriteRIFFSize(size_t length)
	{
		assert_compile(CH_RIFF == 0);
		assert(this->chunk_type == CH_RIFF);
		/* Ugly encoding of >16M RIFF chunks
		 * The lower 24 bits are normal
		 * The uppermost 4 bits are bits 24:27 */
		assert(length < (1 << 28));
		this->WriteUint32((uint32)((length & 0xFFFFFF) | ((length >> 24) << 28)));
	}

	void WriteElementHeader(uint index, size_t length);

	void WriteObjectMember(const void *object, const SaveLoad *sld);

	/**
	 * Main save function.
	 * @param ptr The object that is being saved
	 * @param sld The SaveLoad description of the object so we know how to manipulate it
	 */
	void WriteObject(const void *object, const SaveLoad *sld)
	{
		for (; sld->type != SL_END; sld++) {
			this->WriteObjectMember(object, sld);
		}
	}

	/**
	 * Write a single object as a RIFF chunk.
	 * @param object The object that is being saved
	 * @param sld The SaveLoad description of the object so we know how to manipulate it
	 */
	void WriteRIFFObject(const void *object, const SaveLoad *sld)
	{
		this->WriteRIFFSize(SlCalcObjLength(object, sld));
		this->WriteObject(object, sld);
	}

	/**
	 * Write an element of a (sparse) array as an object.
	 * @param index The index of this element
	 * @param object The object that is being saved
	 * @param sld The SaveLoad description of the object so we know how to manipulate it
	 */
	void WriteElement(uint index, const void *object, const SaveLoad *sld)
	{
		this->WriteElementHeader(index, SlCalcObjLength(object, sld));
		this->WriteObject(object, sld);
	}

	/**
	 * Flush this dumper into another one.
	 * @param dumper The dumper to dump to.
	 */
	void Dump(SaveDumper *target);

	/**
	 * Flush this dumper into a writer.
	 * @param writer The filter we want to use.
	 */
	void Flush(SaveFilter *writer);
};

/** Return how many bytes used to encode a gamma value */
static inline uint GetGammaLength(size_t i)
{
	return 1 + (i >= (1 << 7)) + (i >= (1 << 14)) + (i >= (1 << 21));
}

typedef void ChunkSaveProc(SaveDumper*);
typedef void ChunkLoadProc(LoadBuffer*);
typedef void ChunkPtrsProc(const SavegameTypeVersion*);

/** Handlers and description of chunk. */
struct ChunkHandler {
	uint32 id;                      ///< Unique ID (4 letters).
	ChunkSaveProc *save_proc;       ///< Save procedure of the chunk.
	ChunkLoadProc *load_proc;       ///< Load procedure of the chunk.
	ChunkPtrsProc *ptrs_proc;       ///< Manipulate pointers in the chunk.
	ChunkLoadProc *load_check_proc; ///< Load procedure for game preview.
	uint32 flags;                   ///< Flags of the chunk. @see ChunkType
};

#endif /* SAVELOAD_BUFFER_H */
