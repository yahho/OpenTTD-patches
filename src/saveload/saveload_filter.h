/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_filter.h Declaration of filters used for saving and loading savegames. */

#ifndef SAVELOAD_FILTER_H
#define SAVELOAD_FILTER_H

/** Interface for filtering a savegame till it is loaded. */
struct LoadFilter {
	/** Make sure the writers are properly closed. */
	virtual ~LoadFilter()
	{
	}

	/**
	 * Read a given number of bytes from the savegame.
	 * @param buf The bytes to read.
	 * @param len The number of bytes to read.
	 * @return The number of actually read bytes.
	 */
	virtual size_t Read(byte *buf, size_t len) = 0;

	/**
	 * Reset this filter to read from the beginning of the file.
	 */
	virtual void Reset() = 0;
};

/** Filter for chaining into another filter. */
struct ChainLoadFilter : LoadFilter {
	/** Chained to the (savegame) filters. */
	LoadFilter *chain;

	/**
	 * Initialise this filter.
	 * @param chain The next filter in this chain.
	 */
	ChainLoadFilter(LoadFilter *chain) : LoadFilter(), chain(chain)
	{
	}

	/** Make sure the writers are properly closed. */
	~ChainLoadFilter()
	{
		delete this->chain;
	}

	/**
	 * Reset this filter to read from the beginning of the file.
	 */
	void Reset() OVERRIDE
	{
		this->chain->Reset();
	}
};

/**
 * Instantiator for a chain load filter.
 * @param chain The next filter in this chain.
 * @tparam T    The type of load filter to create.
 */
template <typename T> ChainLoadFilter *CreateLoadFilter(LoadFilter *chain)
{
	return new T(chain);
}

/** Interface for filtering a savegame till it is written. */
struct SaveFilter {
	/** Make sure the writers are properly closed. */
	virtual ~SaveFilter()
	{
	}

	/**
	 * Write a given number of bytes into the savegame.
	 * @param buf The bytes to write.
	 * @param len The number of bytes to write.
	 */
	virtual void Write(byte *buf, size_t len) = 0;

	/**
	 * Prepare everything to finish writing the savegame.
	 */
	virtual void Finish() = 0;
};

/** Filter for chaining into another filter. */
struct ChainSaveFilter : SaveFilter {
	/** Chained to the (savegame) filters. */
	SaveFilter *chain;

	/**
	 * Initialise this filter.
	 * @param chain The next filter in this chain.
	 */
	ChainSaveFilter(SaveFilter *chain) : SaveFilter(), chain(chain)
	{
	}

	/** Make sure the writers are properly closed. */
	~ChainSaveFilter()
	{
		delete this->chain;
	}

	/**
	 * Prepare everything to finish writing the savegame.
	 */
	void Finish() OVERRIDE
	{
		if (this->chain != NULL) this->chain->Finish();
	}
};

/**
 * Instantiator for a chain save filter.
 * @param chain             The next filter in this chain.
 * @param compression_level The requested level of compression.
 * @tparam T                The type of save filter to create.
 */
template <typename T> ChainSaveFilter *CreateSaveFilter(SaveFilter *chain, byte compression_level)
{
	return new T(chain, compression_level);
}


/** The format for a reader/writer type of a savegame */
struct SaveLoadFormat {
	const char *name;                     ///< name of the compressor/decompressor (debug-only)
	uint32 tag;                           ///< the 4-letter tag by which it is identified in the savegame

	ChainLoadFilter *(*init_load)(LoadFilter *chain);                    ///< Constructor for the load filter.
	ChainSaveFilter *(*init_write)(SaveFilter *chain, byte compression); ///< Constructor for the save filter.

	byte min_compression;                 ///< the minimum compression level of this format
	byte default_compression;             ///< the default compression level of this format
	byte max_compression;                 ///< the maximum compression level of this format
};

const SaveLoadFormat *GetSavegameFormat(char *s, byte *compression_level);
const SaveLoadFormat *GetSavegameFormatByTag(uint32 tag);
const SaveLoadFormat *GetLZO0SavegameFormat();

#endif /* SAVELOAD_FILTER_H */
