/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_data.h Saveload data declarations. */

#ifndef SAVELOAD_DATA_H
#define SAVELOAD_DATA_H

#include "../core/bitmath_func.hpp"

/** Types of save games. */
enum SavegameType {
	SGT_TTD,    ///< TTD  savegame (can be detected incorrectly)
	SGT_TTDP1,  ///< TTDP savegame ( -//- ) (data at NW border)
	SGT_TTDP2,  ///< TTDP savegame in new format (data at SE border)
	SGT_OTTD,   ///< OTTD savegame
	SGT_TTO,    ///< TTO savegame
	SGT_INVALID = 0xFF, ///< broken savegame (used internally)
};

/** Type and version of a savegame. */
struct SavegameTypeVersion {
	SavegameType type;  ///< type of savegame
	union {
		struct {
			uint version; ///< version of TTDP savegame (if applicable)
		} ttdp;
		struct {
			uint version;       ///< the major savegame version
			uint minor_version; ///< the minor savegame version
		} ottd;
	};
};

/**
 * Checks whether the given savegame version is below \a major.\a minor.
 * @param stv Savegame version to check.
 * @param major Major number of the version to check against.
 * @param minor Minor number of the version to check against. If \a minor is 0 or not specified, only the major number is checked.
 * @return Savegame version is earlier than the specified version.
 */
static inline bool IsOTTDSavegameVersionBefore(const SavegameTypeVersion *stv, uint16 major, byte minor = 0)
{
	return stv->type != SGT_OTTD || stv->ottd.version < major || (minor > 0 && stv->ottd.version == major && stv->ottd.minor_version < minor);
}


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

/** Flags directing saving/loading of a variable */
enum SaveLoadFlags {
	SLF_GLOBAL          = 1 << 0, ///< global variable, instead of a struct field
	SLF_NOT_IN_SAVE     = 1 << 1, ///< do not save with savegame, basically client-based
	SLF_NOT_IN_CONFIG   = 1 << 2, ///< do not save to config file
	SLF_NO_NETWORK_SYNC = 1 << 3, ///< do not synchronize over network (but it is saved if SLF_NOT_IN_SAVE is not set)
};

/** Version range for SaveLoad objects */
struct SaveLoadRange {
	uint16 from; ///< save/load the variable starting from this savegame version
	uint16 to;   ///< save/load the variable until this savegame version
};

/** SaveLoad type struct. Do NOT use this directly but use the SLE_ macros defined just below! */
struct SaveLoad {
	SaveLoadType type;     ///< object type
	byte conv;             ///< object subtype/conversion
	byte flags;            ///< save/load flags
	uint16 length;         ///< (conditional) length of the variable (eg. arrays) (max array size is 65536 elements)
	SaveLoadRange version; ///< save/load the variable in this version range
	/* NOTE: This element either denotes the address of the variable for a global
	 * variable, or the offset within a struct which is then bound to a variable
	 * during runtime. Decision on which one to use is controlled by the function
	 * that is called to save it. address: global=true, offset: global=false.
	 * For SL_INCLUDE, this points to the SaveLoad object to be included. */
	void *address;         ///< address of variable OR offset of variable in the struct (max offset is 65536)
};

/** Highest possible savegame version. */
#define SL_MAX_VERSION UINT16_MAX

/** Workaround for MSVC broken variadic macro support. */
#define SLE_EXPAND(x) x

/** Substitute first paramenter if non-empty, else second. */
#define SLE_DEFAULT_IF_EMPTY(value, default) ((sizeof(#value) != 1) ? (value + 0) : (default))

/**
 * Generic SaveLoad object, with version range.
 * @param type     Load/save type. @see SaveLoadType
 * @param address  Address of variable or offset of variable in the struct
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field, empty for maximum possible value.
 * @note This macro should not be used directly.
 */
#define SLE_ANY_1(type, address, conv, flags, length, from, to) {type, conv, flags, length, {from, SLE_DEFAULT_IF_EMPTY(to, SL_MAX_VERSION)}, address}

/**
 * Generic SaveLoad object, without version range.
 * @param type     Load/save type. @see SaveLoadType
 * @param address  Address of variable or offset of variable in the struct
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @note This macro should not be used directly.
 */
#define SLE_ANY_0(type, address, conv, flags, length, ...) SLE_ANY_1(type, address, conv, flags, length, 0, )

/**
 * Generic SaveLoad object, with or without version range.
 * @param type     Load/save type. @see SaveLoadType
 * @param address  Address of variable or offset of variable in the struct
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 * @note There should be a trailing empty argument to this macro.
 * @note This macro should not be used directly.
 */
#define SLE_ANY_(type, address, conv, flags, ...) SLE_EXPAND(SLE_ANY(type, address, conv, flags, __VA_ARGS__ 1, INVALID, 0, INVALID, ))
#define SLE_ANY(type, address, conv, flags, length, from, to, n, ...) SLE_ANY_##n(type, address, conv, flags, length, from, to)

/**
 * Storage of simple variables, references (pointers), and arrays.
 * @param type     Load/save type. @see SaveLoadType
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 * @note In general, it is better to use one of the SLE_* macros below.
 */
#define SLE_GENERAL(...) SLE_EXPAND(SLE_GENERAL_(__VA_ARGS__, ))
#define SLE_GENERAL_(type, base, variable, conv, flags, length, ...) SLE_EXPAND(SLE_ANY_(type, ((void*)cpp_offsetof(base, variable)), conv, flags, length, __VA_ARGS__))

/**
 * Storage of a variable.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLE_VAR(...) SLE_EXPAND(SLE_VAR_(__VA_ARGS__, ))
#define SLE_VAR_(base, variable, conv, ...) SLE_EXPAND(SLE_GENERAL_(SL_VAR, base, variable, conv, 0, 0, __VA_ARGS__))
#define SLE_CONDVAR SLE_VAR

/**
 * Storage of a reference.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLE_REF(...) SLE_EXPAND(SLE_REF_(__VA_ARGS__, ))
#define SLE_REF_(base, variable, reftype, ...) SLE_EXPAND(SLE_GENERAL_(SL_REF, base, variable, reftype, 0, 0, __VA_ARGS__))
#define SLE_CONDREF SLE_REF

/**
 * Storage of an array.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array (optional).
 * @param to       Last savegame version that has the array (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLE_ARR(...) SLE_EXPAND(SLE_ARR_(__VA_ARGS__, ))
#define SLE_ARR_(base, variable, conv, length, ...) SLE_EXPAND(SLE_GENERAL_(SL_ARR, base, variable, conv, 0, length, __VA_ARGS__))
#define SLE_CONDARR SLE_ARR

/**
 * Storage of a string.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 * @param from     First savegame version that has the string (optional).
 * @param to       Last savegame version that has the string (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLE_STR(...) SLE_EXPAND(SLE_STR_(__VA_ARGS__, ))
#define SLE_STR_(base, variable, conv, length, ...) SLE_EXPAND(SLE_GENERAL_(SL_STR, base, variable, conv, 0, length, __VA_ARGS__))
#define SLE_CONDSTR SLE_STR

/**
 * Storage of a list.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the list (optional).
 * @param to       Last savegame version that has the list (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLE_LST(...) SLE_EXPAND(SLE_LST_(__VA_ARGS__, ))
#define SLE_LST_(base, variable, reftype, ...) SLE_EXPAND(SLE_GENERAL_(SL_LST, base, variable, reftype, 0, 0, __VA_ARGS__))
#define SLE_CONDLST SLE_LST

/**
 * Empty space.
 * @param length Length of the empty space.
 * @param from   First savegame version that has the empty space (optional).
 * @param to     Last savegame version that has the empty space (optional).
 */
#define SLE_NULL(...) SLE_EXPAND(SLE_NULL_(__VA_ARGS__, ))
#define SLE_NULL_(length, ...) SLE_EXPAND(SLE_ANY_(SL_ARR, (void*)NULL, SLE_FILE_U8 | SLE_VAR_NULL, SLF_NOT_IN_CONFIG, length, __VA_ARGS__))
#define SLE_CONDNULL SLE_NULL

/** Translate values ingame to different values in the savegame and vv. */
#define SLE_WRITEBYTE(base, variable, value) SLE_GENERAL(SL_WRITEBYTE, base, variable, value, 0, 0, 0, )

/** Include another SaveLoad object. */
#define SLE_INCLUDE(include) {SL_INCLUDE, 0, 0, 0, {0, SL_MAX_VERSION}, const_cast<SaveLoad *>(include)}

/** End marker of a struct/class save or load. */
#define SLE_END() {SL_END, 0, 0, 0, {0, 0}, NULL}

/**
 * Storage of global simple variables, references (pointers), and arrays.
 * @param type     Load/save type. @see SaveLoadType
 * @param variable Name of the global variable.
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param flags    Save/load flags
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 * @note In general, it is better to use one of the SLEG_* macros below.
 */
#define SLEG_GENERAL(...) SLE_EXPAND(SLEG_GENERAL_(__VA_ARGS__, ))
#define SLEG_GENERAL_(type, variable, conv, flags, length, ...) SLE_EXPAND(SLE_ANY_(type, (void*)&variable, conv, (flags) | SLF_GLOBAL, length, __VA_ARGS__))

/**
 * Storage of a global variable.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLEG_VAR(...) SLE_EXPAND(SLEG_VAR_(__VA_ARGS__, ))
#define SLEG_VAR_(variable, conv, ...) SLE_EXPAND(SLEG_GENERAL_(SL_VAR, variable, conv, 0, 0, __VA_ARGS__))
#define SLEG_CONDVAR SLEG_VAR

/**
 * Storage of a global reference.
 * @param variable Name of the global variable.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLEG_REF(...) SLE_EXPAND(SLEG_REF_(__VA_ARGS__, ))
#define SLEG_REF_(variable, reftype, ...) SLE_EXPAND(SLEG_GENERAL_(SL_REF, variable, reftype, 0, 0, __VA_ARGS__))
#define SLEG_CONDREF SLEG_REF

/**
 * Storage of a global array.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array (optional).
 * @param to       Last savegame version that has the array (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLEG_ARR(...) SLE_EXPAND(SLEG_ARR_(__VA_ARGS__, ))
#define SLEG_ARR_(variable, conv, length, ...) SLE_EXPAND(SLEG_GENERAL_(SL_ARR, variable, conv, 0, length, __VA_ARGS__))
#define SLEG_CONDARR SLEG_ARR

/**
 * Storage of a global string.
 * @param variable Name of the global variable.
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 * @param from     First savegame version that has the string (optional).
 * @param to       Last savegame version that has the string (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLEG_STR(...) SLE_EXPAND(SLEG_STR_(__VA_ARGS__, ))
#define SLEG_STR_(variable, conv, length, ...) SLE_EXPAND(SLEG_GENERAL_(SL_STR, variable, conv, 0, length, __VA_ARGS__))
#define SLEG_CONDSTR SLEG_STR

/**
 * Storage of a global list.
 * @param variable Name of the global variable.
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the list (optional).
 * @param to       Last savegame version that has the list (optional).
 * @note Either both version arguments or neither of them should be present.
 */
#define SLEG_LST(...) SLE_EXPAND(SLEG_LST_(__VA_ARGS__, ))
#define SLEG_LST_(variable, reftype, ...) SLE_EXPAND(SLEG_GENERAL_(SL_LST, variable, reftype, 0, 0, __VA_ARGS__))
#define SLEG_CONDLST SLEG_LST

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
 * Return the size in bytes of a certain type of normal/atomic variable
 * as it appears in memory. See VarTypes
 * @param conv VarType type of variable that is used for calculating the size
 * @return Return the size of this type in bytes
 */
static inline uint SlCalcConvMemLen(VarType conv)
{
	extern const byte _conv_mem_size[];
	byte length = GB(conv, 4, 4);
	assert(length <= SLE_VAR_NULL);
	return _conv_mem_size[length];
}

/**
 * Return the size in bytes of a certain type of normal/atomic variable
 * as it appears in a saved game. See VarTypes
 * @param conv VarType type of variable that is used for calculating the size
 * @return Return the size of this type in bytes
 */
static inline byte SlCalcConvFileLen(VarType conv)
{
	extern const byte _conv_file_size[];
	byte length = GB(conv, 0, 4);
	assert(length <= SLE_FILE_STRINGID);
	return _conv_file_size[length];
}

int64 ReadValue(const void *ptr, VarType conv);
void WriteValue(void *ptr, VarType conv, int64 val);

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

/** Is this object valid in a certain savegame version? */
static inline bool SlIsObjectValidInSavegame(const SavegameTypeVersion *stv, const SaveLoad *sld)
{
	if (stv->type == SGT_OTTD) {
		if (stv->ottd.version < sld->version.from || stv->ottd.version > sld->version.to) return false;
	} else {
		if (0 < sld->version.from) return false;
	}
	if (sld->flags & SLF_NOT_IN_SAVE) return false;

	return true;
}

/**
 * Checks if a SaveLoad object is active in the current savegame version.
 * @param sld SaveLoad object to check
 * @return Whether the object is active in the current savegame version.
 */
static inline bool SlIsObjectCurrentlyValid(const SaveLoad *sld)
{
	extern const uint16 SAVEGAME_VERSION;
	if (SAVEGAME_VERSION < sld->version.from || SAVEGAME_VERSION > sld->version.to) return false;

	return true;
}

size_t SlCalcObjLength(const void *object, const SaveLoad *sld);

void SlObjectPtrs(void *object, const SaveLoad *sld, const SavegameTypeVersion *stv);

#endif /* SAVELOAD_DATA_H */
