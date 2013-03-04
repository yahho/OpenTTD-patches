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

/** Yes, simply reading from a file. */
struct FileReader : LoadFilter {
	FILE *file; ///< The file to read from.
	long begin; ///< The begin of the file.

	/**
	 * Create the file reader, so it reads from a specific file.
	 * @param file The file to read from.
	 */
	FileReader(FILE *file) : LoadFilter(), file(file), begin(ftell(file))
	{
	}

	/** Make sure everything is cleaned up. */
	~FileReader()
	{
		if (this->file != NULL) fclose(this->file);
		this->file = NULL;
	}

	/* virtual */ size_t Read(byte *buf, size_t size)
	{
		/* We're in the process of shutting down, i.e. in "failure" mode. */
		if (this->file == NULL) return 0;

		return fread(buf, 1, size, this->file);
	}

	/* virtual */ void Reset()
	{
		clearerr(this->file);
		fseek(this->file, this->begin, SEEK_SET);
	}
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
	virtual void Write(const byte *buf, size_t len) = 0;

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


ChainSaveFilter *GetSavegameWriter(char *s, uint version, SaveFilter *writer);
ChainLoadFilter* (*GetSavegameLoader(uint32 tag))(LoadFilter *chain);
ChainLoadFilter* (*GetOTTDSavegameLoader(uint32 tag))(LoadFilter *chain);
ChainLoadFilter* (*GetLZO0SavegameLoader())(LoadFilter *chain);

#endif /* SAVELOAD_FILTER_H */
