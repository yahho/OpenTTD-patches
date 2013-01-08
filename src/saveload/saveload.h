/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload.h Functions/types related to saving and loading games. */

#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "../fileio_type.h"
#include "../strings_type.h"

/** Save or load result codes. */
enum SaveOrLoadResult {
	SL_OK     = 0, ///< completed successfully
	SL_ERROR  = 1, ///< error that was caught before internal structures were modified
	SL_REINIT = 2, ///< error that was caught in the middle of updating game state, need to clear it. (can only happen during load)
};

/** Save or load mode. @see SaveOrLoad */
enum SaveOrLoadMode {
	SL_INVALID    = -1, ///< Invalid mode.
	SL_LOAD       =  0, ///< Load game.
	SL_SAVE       =  1, ///< Save game.
	SL_OLD_LOAD   =  2, ///< Load old game.
	SL_PNG        =  3, ///< Load PNG file (height map).
	SL_BMP        =  4, ///< Load BMP file (height map).
	SL_LOAD_CHECK =  5, ///< Load for game preview.
};

/** Types of save games. */
enum SavegameType {
	SGT_TTD,    ///< TTD  savegame (can be detected incorrectly)
	SGT_TTDP1,  ///< TTDP savegame ( -//- ) (data at NW border)
	SGT_TTDP2,  ///< TTDP savegame in new format (data at SE border)
	SGT_OTTD,   ///< OTTD savegame
	SGT_TTO,    ///< TTO savegame
	SGT_INVALID = 0xFF, ///< broken savegame (used internally)
};

void GenerateDefaultSaveName(char *buf, const char *last);
void SetSaveLoadError(uint16 str);
const char *GetSaveLoadErrorString();
SaveOrLoadResult SaveOrLoad(const char *filename, int mode, Subdirectory sb, bool threaded = true);
void WaitTillSaved();
void ProcessAsyncSaveFinish();
void DoExitSave();

SaveOrLoadResult SaveWithFilter(struct SaveFilter *writer, bool threaded);
SaveOrLoadResult LoadWithFilter(struct LoadFilter *reader);

typedef void ChunkSaveLoadProc();
typedef void AutolengthProc(void *arg);

/** Handlers and description of chunk. */
struct ChunkHandler {
	uint32 id;                          ///< Unique ID (4 letters).
	ChunkSaveLoadProc *save_proc;       ///< Save procedure of the chunk.
	ChunkSaveLoadProc *load_proc;       ///< Load procedure of the chunk.
	ChunkSaveLoadProc *ptrs_proc;       ///< Manipulate pointers in the chunk.
	ChunkSaveLoadProc *load_check_proc; ///< Load procedure for game preview.
	uint32 flags;                       ///< Flags of the chunk. @see ChunkType
};

/** Type of reference (#SLE_REF, #SLE_CONDREF). */
enum SLRefType {
	REF_ORDER          =  0, ///< Load/save a reference to an order.
	REF_VEHICLE        =  1, ///< Load/save a reference to a vehicle.
	REF_STATION        =  2, ///< Load/save a reference to a station.
	REF_TOWN           =  3, ///< Load/save a reference to a town.
	REF_VEHICLE_OLD    =  4, ///< Load/save an old-style reference to a vehicle (for pre-4.4 savegames).
	REF_ROADSTOPS      =  5, ///< Load/save a reference to a bus/truck stop.
	REF_ENGINE_RENEWS  =  6, ///< Load/save a reference to an engine renewal (autoreplace).
	REF_CARGO_PACKET   =  7, ///< Load/save a reference to a cargo packet.
	REF_ORDERLIST      =  8, ///< Load/save a reference to an orderlist.
	REF_STORAGE        =  9, ///< Load/save a reference to a persistent storage.
	REF_LINK_GRAPH     = 10, ///< Load/save a reference to a link graph.
	REF_LINK_GRAPH_JOB = 11, ///< Load/save a reference to a link graph job.
};

/** Highest possible savegame version. */
#define SL_MAX_VERSION UINT16_MAX

/** Flags of a chunk. */
enum ChunkType {
	CH_RIFF         =  0,
	CH_ARRAY        =  1,
	CH_SPARSE_ARRAY =  2,
	CH_TYPE_MASK    =  3,
	CH_LAST         =  8, ///< Last chunk in this array.
};

/**
 * VarTypes is the general bitmasked magic type that tells us
 * certain characteristics about the variable it refers to. For example
 * SLE_FILE_* gives the size(type) as it would be in the savegame and
 * SLE_VAR_* the size(type) as it is in memory during runtime. These are
 * the first 8 bits (0-3 SLE_FILE, 4-7 SLE_VAR).
 * Bits 8-15 are reserved for various flags as explained below
 */
enum VarTypes {
	/* 4 bits allocated for a maximum of 16 types for NumberType */
	SLE_FILE_I8       = 0,
	SLE_FILE_U8       = 1,
	SLE_FILE_I16      = 2,
	SLE_FILE_U16      = 3,
	SLE_FILE_I32      = 4,
	SLE_FILE_U32      = 5,
	SLE_FILE_I64      = 6,
	SLE_FILE_U64      = 7,
	SLE_FILE_STRINGID = 8, ///< StringID offset into strings-array
	/* 7 more possible file-primitives */

	/* 4 bits allocated for a maximum of 16 types for NumberType */
	SLE_VAR_BL    =  0 << 4,
	SLE_VAR_I8    =  1 << 4,
	SLE_VAR_U8    =  2 << 4,
	SLE_VAR_I16   =  3 << 4,
	SLE_VAR_U16   =  4 << 4,
	SLE_VAR_I32   =  5 << 4,
	SLE_VAR_U32   =  6 << 4,
	SLE_VAR_I64   =  7 << 4,
	SLE_VAR_U64   =  8 << 4,
	SLE_VAR_NULL  =  9 << 4, ///< useful to write zeros in savegame.
	SLE_VAR_NAME  = 10 << 4, ///< old custom name to be converted to a char pointer
	/* 5 more possible memory-primitives */

	/* Shortcut values */
	SLE_VAR_CHAR = SLE_VAR_I8,

	/* Default combinations of variables. As savegames change, so can variables
	 * and thus it is possible that the saved value and internal size do not
	 * match and you need to specify custom combo. The defaults are listed here */
	SLE_BOOL         = SLE_FILE_I8  | SLE_VAR_BL,
	SLE_INT8         = SLE_FILE_I8  | SLE_VAR_I8,
	SLE_UINT8        = SLE_FILE_U8  | SLE_VAR_U8,
	SLE_INT16        = SLE_FILE_I16 | SLE_VAR_I16,
	SLE_UINT16       = SLE_FILE_U16 | SLE_VAR_U16,
	SLE_INT32        = SLE_FILE_I32 | SLE_VAR_I32,
	SLE_UINT32       = SLE_FILE_U32 | SLE_VAR_U32,
	SLE_INT64        = SLE_FILE_I64 | SLE_VAR_I64,
	SLE_UINT64       = SLE_FILE_U64 | SLE_VAR_U64,
	SLE_CHAR         = SLE_FILE_I8  | SLE_VAR_CHAR,
	SLE_STRINGID     = SLE_FILE_STRINGID | SLE_VAR_U16,
	SLE_NAME         = SLE_FILE_STRINGID | SLE_VAR_NAME,

	/* Shortcut values */
	SLE_UINT  = SLE_UINT32,
	SLE_INT   = SLE_INT32,
};

typedef byte VarType;

/**
 * StrTypes encodes information about saving and loading of strings (#SLE_STR).
 */
enum StrTypes {
	SLS_QUOTED        = 1 << 0, ///< string is enclosed in quotes
	SLS_POINTER       = 1 << 1, ///< string is stored as a pointer (as opposed to a fixed buffer)

	SLS_STRB  = 0,                        ///< string (with pre-allocated buffer)
	SLS_STRBQ = SLS_QUOTED,               ///< string enclosed in quotes (with pre-allocated buffer)
	SLS_STR   = SLS_POINTER,              ///< string pointer
	SLS_STRQ  = SLS_POINTER | SLS_QUOTED, ///< string pointer enclosed in quotes

	SLS_ALLOW_CONTROL = 1 << 2, ///< allow control codes in the string
	SLS_ALLOW_NEWLINE = 1 << 3, ///< allow newlines in the string
	/* 4 more possible flags */
};

typedef byte StrType;

/** Type of data saved. */
enum SaveLoadTypes {
	SL_VAR         =  0, ///< Save/load a variable.
	SL_REF         =  1, ///< Save/load a reference.
	SL_ARR         =  2, ///< Save/load an array.
	SL_STR         =  3, ///< Save/load a string.
	SL_LST         =  4, ///< Save/load a list.
	/* non-normal save-load types */
	SL_WRITEBYTE   =  8,
	SL_INCLUDE     =  9,
	SL_END         = 15
};

typedef byte SaveLoadType; ///< Save/load type. @see SaveLoadTypes

/** Flags directing saving/loading of a variable */
enum SaveLoadFlags {
	SLF_GLOBAL          = 1 << 0, ///< global variable, instead of a struct field
	SLF_NOT_IN_SAVE     = 1 << 1, ///< do not save with savegame, basically client-based
	SLF_NOT_IN_CONFIG   = 1 << 2, ///< do not save to config file
	SLF_NO_NETWORK_SYNC = 1 << 3, ///< do not synchronize over network (but it is saved if SLF_NOT_IN_SAVE is not set)
};

/** SaveLoad type struct. Do NOT use this directly but use the SLE_ macros defined just below! */
struct SaveLoad {
	SaveLoadType type;   ///< object type
	byte conv;           ///< object subtype/conversion
	byte flags;          ///< save/load flags
	uint16 length;       ///< (conditional) length of the variable (eg. arrays) (max array size is 65536 elements)
	uint16 version_from; ///< save/load the variable starting from this savegame version
	uint16 version_to;   ///< save/load the variable until this savegame version
	/* NOTE: This element either denotes the address of the variable for a global
	 * variable, or the offset within a struct which is then bound to a variable
	 * during runtime. Decision on which one to use is controlled by the function
	 * that is called to save it. address: global=true, offset: global=false.
	 * For SL_INCLUDE, this points to the SaveLoad object to be included. */
	void *address;       ///< address of variable OR offset of variable in the struct (max offset is 65536)
};

/** Same as #SaveLoad but global variables are used (for better readability); */
typedef SaveLoad SaveLoadGlobVarList;

/**
 * Storage of simple variables, references (pointers), and arrays.
 * @param type     Load/save type. @see SaveLoadType
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 * @note In general, it is better to use one of the SLE_* macros below.
 */
#define SLE_GENERAL(type, base, variable, conv, flags, length, from, to) {type, conv, flags, length, from, to, (void*)cpp_offsetof(base, variable)}

/**
 * Storage of a variable in some savegame versions.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLE_CONDVAR(base, variable, conv, from, to) SLE_GENERAL(SL_VAR, base, variable, conv, 0, 0, from, to)

/**
 * Storage of a reference in some savegame versions.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLE_CONDREF(base, variable, reftype, from, to) SLE_GENERAL(SL_REF, base, variable, reftype, 0, 0, from, to)

/**
 * Storage of an array in some savegame versions.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array.
 * @param to       Last savegame version that has the array.
 */
#define SLE_CONDARR(base, variable, conv, length, from, to) SLE_GENERAL(SL_ARR, base, variable, conv, 0, length, from, to)

/**
 * Storage of a string in some savegame versions.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 * @param from     First savegame version that has the string.
 * @param to       Last savegame version that has the string.
 */
#define SLE_CONDSTR(base, variable, conv, length, from, to) SLE_GENERAL(SL_STR, base, variable, conv, 0, length, from, to)

/**
 * Storage of a list in some savegame versions.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLE_CONDLST(base, variable, reftype, from, to) SLE_GENERAL(SL_LST, base, variable, reftype, 0, 0, from, to)

/**
 * Storage of a variable in every version of a savegame.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 */
#define SLE_VAR(base, variable, conv) SLE_CONDVAR(base, variable, conv, 0, SL_MAX_VERSION)

/**
 * Storage of a reference in every version of a savegame.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param reftype  Type of the reference, a value from #SLRefType.
 */
#define SLE_REF(base, variable, reftype) SLE_CONDREF(base, variable, reftype, 0, SL_MAX_VERSION)

/**
 * Storage of an array in every version of a savegame.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 */
#define SLE_ARR(base, variable, conv, length) SLE_CONDARR(base, variable, conv, length, 0, SL_MAX_VERSION)

/**
 * Storage of a string in every savegame version.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 */
#define SLE_STR(base, variable, conv, length) SLE_CONDSTR(base, variable, conv, length, 0, SL_MAX_VERSION)

/**
 * Storage of a list in every savegame version.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param reftype  Type of the reference, a value from #SLRefType.
 */
#define SLE_LST(base, variable, reftype) SLE_CONDLST(base, variable, reftype, 0, SL_MAX_VERSION)

/**
 * Empty space in some savegame versions.
 * @param length Length of the empty space.
 * @param from   First savegame version that has the empty space.
 * @param to     Last savegame version that has the empty space.
 */
#define SLE_CONDNULL(length, from, to) {SL_ARR, SLE_FILE_U8 | SLE_VAR_NULL, SLF_NOT_IN_CONFIG, length, from, to, (void*)NULL}

/**
 * Empty space in every savegame version.
 * @param length Length of the empty space.
 */
#define SLE_NULL(length) SLE_CONDNULL(length, 0, SL_MAX_VERSION)

/** Translate values ingame to different values in the savegame and vv. */
#define SLE_WRITEBYTE(base, variable, value) SLE_GENERAL(SL_WRITEBYTE, base, variable, value, 0, 0, 0, SL_MAX_VERSION)

/** Include another SaveLoad object. */
#define SLE_INCLUDE(include) {SL_INCLUDE, 0, 0, 0, 0, SL_MAX_VERSION, const_cast<SaveLoad *>(include)}

/** End marker of a struct/class save or load. */
#define SLE_END() {SL_END, 0, 0, 0, 0, 0, NULL}

/**
 * Storage of global simple variables, references (pointers), and arrays.
 * @param type     Load/save type. @see SaveLoadType
 * @param variable Name of the global variable.
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 * @note In general, it is better to use one of the SLEG_* macros below.
 */
#define SLEG_GENERAL(type, variable, conv, flags, length, from, to) {type, conv, (flags) | SLF_GLOBAL, length, from, to, (void*)&variable}

/**
 * Storage of a global variable in some savegame versions.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLEG_CONDVAR(variable, conv, from, to) SLEG_GENERAL(SL_VAR, variable, conv, 0, 0, from, to)

/**
 * Storage of a global reference in some savegame versions.
 * @param variable Name of the global variable.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLEG_CONDREF(variable, reftype, from, to) SLEG_GENERAL(SL_REF, variable, reftype, 0, 0, from, to)

/**
 * Storage of a global array in some savegame versions.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array.
 * @param to       Last savegame version that has the array.
 */
#define SLEG_CONDARR(variable, conv, length, from, to) SLEG_GENERAL(SL_ARR, variable, conv, 0, length, from, to)

/**
 * Storage of a global string in some savegame versions.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 * @param from     First savegame version that has the string.
 * @param to       Last savegame version that has the string.
 */
#define SLEG_CONDSTR(variable, conv, length, from, to) SLEG_GENERAL(SL_STR, variable, conv, 0, length, from, to)

/**
 * Storage of a global list in some savegame versions.
 * @param variable Name of the global variable.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLEG_CONDLST(variable, reftype, from, to) SLEG_GENERAL(SL_LST, variable, reftype, 0, 0, from, to)

/**
 * Storage of a global variable in every savegame version.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 */
#define SLEG_VAR(variable, conv) SLEG_CONDVAR(variable, conv, 0, SL_MAX_VERSION)

/**
 * Storage of a global reference in every savegame version.
 * @param variable Name of the global variable.
 * @param reftype  Type of the reference, a value from #SLRefType.
 */
#define SLEG_REF(variable, reftype) SLEG_CONDREF(variable, reftype, 0, SL_MAX_VERSION)

/**
 * Storage of a global array in every savegame version.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 */
#define SLEG_ARR(variable, conv) SLEG_CONDARR(variable, conv, lengthof(variable), 0, SL_MAX_VERSION)

/**
 * Storage of a global string in every savegame version.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 */
#define SLEG_STR(variable, conv) SLEG_CONDSTR(variable, conv, lengthof(variable), 0, SL_MAX_VERSION)

/**
 * Storage of a global list in every savegame version.
 * @param variable Name of the global variable.
 * @param reftype  Type of the reference, a value from #SLRefType.
 */
#define SLEG_LST(variable, reftype) SLEG_CONDLST(variable, reftype, 0, SL_MAX_VERSION)

/**
 * Checks whether the savegame is below \a major.\a minor.
 * @param major Major number of the version to check against.
 * @param minor Minor number of the version to check against. If \a minor is 0 or not specified, only the major number is checked.
 * @return Savegame version is earlier than the specified version.
 */
static inline bool IsSavegameVersionBefore(uint16 major, byte minor = 0)
{
	extern uint16 _sl_version;
	extern byte   _sl_minor_version;
	return _sl_version < major || (minor > 0 && _sl_version == major && _sl_minor_version < minor);
}

/**
 * Checks if a SaveLoad object is active in the current savegame version.
 * @param sld SaveLoad object to check
 * @return Whether the object is active in the current savegame version.
 */
static inline bool SlIsObjectCurrentlyValid(const SaveLoad *sld)
{
	extern const uint16 SAVEGAME_VERSION;
	if (SAVEGAME_VERSION < sld->version_from || SAVEGAME_VERSION > sld->version_to) return false;

	return true;
}

/**
 * Get the NumberType of a setting. This describes the integer type
 * as it is represented in memory
 * @param type VarType holding information about the variable-type
 * @return return the SLE_VAR_* part of a variable-type description
 */
static inline VarType GetVarMemType(VarType type)
{
	return type & 0xF0; // GB(type, 4, 4) << 4;
}

/**
 * Get the #FileType of a setting. This describes the integer type
 * as it is represented in a savegame/file
 * @param type VarType holding information about the file-type
 * @param return the SLE_FILE_* part of a variable-type description
 */
static inline VarType GetVarFileType(VarType type)
{
	return type & 0xF; // GB(type, 0, 4);
}

/**
 * Check if the given saveload type is a numeric type.
 * @param conv the type to check
 * @return True if it's a numeric type.
 */
static inline bool IsNumericType(VarType conv)
{
	return GetVarMemType(conv) <= SLE_VAR_U64;
}

/**
 * Get the address of the variable. Which one to pick depends on the object
 * pointer. If it is NULL we are dealing with global variables so the address
 * is taken. If non-null only the offset is stored in the union and we need
 * to add this to the address of the object
 */
static inline void *GetVariableAddress(const SaveLoad *sld, void *object = NULL)
{
	return (sld->flags & SLF_GLOBAL) ? sld->address : ((byte*)object + (ptrdiff_t)sld->address);
}

static inline const void *GetVariableAddress(const SaveLoad *sld, const void *object)
{
	return GetVariableAddress(sld, const_cast<void *>(object));
}

int64 ReadValue(const void *ptr, VarType conv);
void WriteValue(void *ptr, VarType conv, int64 val);

void SlSetArrayIndex(uint index);
int SlIterateArray();

void SlAutolength(AutolengthProc *proc, void *arg);
size_t SlGetFieldLength();
void SlSetLength(size_t length);
size_t SlCalcObjMemberLength(const void *object, const SaveLoad *sld);
size_t SlCalcObjLength(const void *object, const SaveLoad *sld);

byte SlReadByte();
void SlWriteByte(byte b);

void SlGlobList(const SaveLoadGlobVarList *sldg);
void SlArray(void *array, size_t length, VarType conv);
void SlObject(void *object, const SaveLoad *sld);
bool SlObjectMember(void *object, const SaveLoad *sld);
void NORETURN SlError(StringID string, const char *extra_msg = NULL);
void NORETURN SlErrorCorrupt(const char *msg);

bool SaveloadCrashWithMissingNewGRFs();

extern char _savegame_format[8];
extern bool _do_autosave;

#endif /* SAVELOAD_H */
