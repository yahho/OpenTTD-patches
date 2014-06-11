/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string.cpp Handling of C-type strings (char*). */

#include "stdafx.h"
#include "debug.h"
#include "core/alloc_func.hpp"
#include "core/math_func.hpp"
#include "string.h"

#include "table/control_codes.h"

#include <stdarg.h>
#include <ctype.h> /* required for tolower() */

#ifdef _MSC_VER
#include <errno.h> // required by vsnprintf implementation for MSVC
#endif

#ifdef WITH_ICU
/* Required by strnatcmp. */
#include <unicode/ustring.h>
#include "language.h"
#include "gfx_func.h"
#endif /* WITH_ICU */


#ifdef WIN32
/* Since version 3.14, MinGW Runtime has snprintf() and vsnprintf() conform to C99 but it's not the case for older versions */
#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 14))
int CDECL snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);
	return ret;
}
#endif /* MinGW Runtime < 3.14 */

#ifdef _MSC_VER
/**
 * Almost POSIX compliant implementation of \c vsnprintf for VC compiler.
 * The difference is in the value returned on output truncation. This
 * implementation returns size whereas a POSIX implementation returns
 * size or more (the number of bytes that would be written to str
 * had size been sufficiently large excluding the terminating null byte).
 */
int CDECL vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	if (size == 0) return 0;

	errno = 0;
	int ret = _vsnprintf(str, size, format, ap);

	if (ret < 0) {
		if (errno != ERANGE) {
			/* There's a formatting error, better get that looked
			 * at properly instead of ignoring it. */
			NOT_REACHED();
		}
	} else if ((size_t)ret < size) {
		/* The buffer is big enough for the number of
		 * characters stored (excluding null), i.e.
		 * the string has been null-terminated. */
		return ret;
	}

	/* The buffer is too small for _vsnprintf to write the
	 * null-terminator at its end and return size. */
	str[size - 1] = '\0';
	return (int)size;
}
#endif /* _MSC_VER */

#endif /* WIN32 */

/**
 * Copies characters from one buffer to another.
 *
 * Copies the source string to the destination buffer with respect of the
 * terminating null-character and the maximum size of the destination
 * buffer.
 *
 * @note usage ttd_strlcpy(dst, src, lengthof(dst));
 * @note lengthof() applies only to fixed size arrays
 *
 * @param dst The destination buffer
 * @param src The buffer containing the string to copy
 * @param size The maximum size of the destination buffer
 */
void ttd_strlcpy(char *dst, const char *src, size_t size)
{
	assert(size > 0);
	while (--size > 0 && *src != '\0') {
		*dst++ = *src++;
	}
	*dst = '\0';
}


/** Allocate a copy of a given string, and error out on failure. */
char *xstrdup (const char *s)
{
	return (char*) xmemdup (s, strlen(s) + 1);
}

/**
 * Allocate a copy of a given string, with bounded size, and error out
 * on failure.
 *
 * Note! This is not the same as strndup, because it assumes that the
 * string passed in is at least of the required size, unlike strndup,
 * which will check if there is a null in the requested initial segment.
 */
char *xstrmemdup (const char *s, size_t n)
{
	char *p = xmalloc (n + 1);
	memcpy (p, s, n);
	p[n] = '\0';
	return p;
}

/** Allocate a copy of a given string, with bounded size, and error out on failure. */
char *xstrndup (const char *s, size_t n)
{
	return xstrmemdup (s, ttd_strnlen (s, n));
}

/** Allocate a formatted string. */
static char *str_vfmt (const char *fmt, va_list args)
{
#ifdef _GNU_SOURCE
	char *s;
	if (vasprintf (&s, fmt, args) == -1) out_of_memory();
	return s;
#else
	char buf[4096];
	int len = vsnprintf (buf, lengthof(buf), fmt, args);
	return (char*) xmemdup (buf, len + 1);
#endif
}

/**
 * Format, "printf", into a newly allocated string.
 * @param str The formatting string.
 * @return The formatted string. You must free this!
 */
char *CDECL str_fmt(const char *str, ...)
{
	va_list va;
	va_start(va, str);
	char *s = str_vfmt (str, va);
	va_end(va);
	return s;
}


#ifdef DEFINE_STRCASESTR
char *strcasestr(const char *haystack, const char *needle)
{
	size_t hay_len = strlen(haystack);
	size_t needle_len = strlen(needle);
	while (hay_len >= needle_len) {
		if (strncasecmp(haystack, needle, needle_len) == 0) return const_cast<char *>(haystack);

		haystack++;
		hay_len--;
	}

	return NULL;
}
#endif /* DEFINE_STRCASESTR */

/**
 * Skip some of the 'garbage' in the string that we don't want to use
 * to sort on. This way the alphabetical sorting will work better as
 * we would be actually using those characters instead of some other
 * characters such as spaces and tildes at the begin of the name.
 * @param str The string to skip the initial garbage of.
 * @return The string with the garbage skipped.
 */
static const char *SkipGarbage(const char *str)
{
	while (*str != '\0' && (*str < '0' || IsInsideMM(*str, ';', '@' + 1) || IsInsideMM(*str, '[', '`' + 1) || IsInsideMM(*str, '{', '~' + 1))) str++;
	return str;
}

/**
 * Compares two strings using case insensitive natural sort.
 *
 * @param s1 First string to compare.
 * @param s2 Second string to compare.
 * @param ignore_garbage_at_front Skip punctuation characters in the front
 * @return Less than zero if s1 < s2, zero if s1 == s2, greater than zero if s1 > s2.
 */
int strnatcmp(const char *s1, const char *s2, bool ignore_garbage_at_front)
{
	if (ignore_garbage_at_front) {
		s1 = SkipGarbage(s1);
		s2 = SkipGarbage(s2);
	}
#ifdef WITH_ICU
	if (_current_collator != NULL) {
		UErrorCode status = U_ZERO_ERROR;
		int result;

		/* We want to use the new faster method for ICU 4.2 and higher. */
#if U_ICU_VERSION_MAJOR_NUM > 4 || (U_ICU_VERSION_MAJOR_NUM == 4 && U_ICU_VERSION_MINOR_NUM >= 2)
		/* The StringPiece parameter gets implicitly constructed from the char *. */
		result = _current_collator->compareUTF8(s1, s2, status);
#else /* The following for 4.0 and lower. */
		UChar buffer1[DRAW_STRING_BUFFER];
		u_strFromUTF8Lenient(buffer1, lengthof(buffer1), NULL, s1, -1, &status);
		UChar buffer2[DRAW_STRING_BUFFER];
		u_strFromUTF8Lenient(buffer2, lengthof(buffer2), NULL, s2, -1, &status);

		result = _current_collator->compare(buffer1, buffer2, status);
#endif /* ICU version check. */
		if (U_SUCCESS(status)) return result;
	}

#endif /* WITH_ICU */

	/* Do a normal comparison if ICU is missing or if we cannot create a collator. */
	return strcasecmp(s1, s2);
}

/**
 * Convert a given ASCII string to lowercase.
 * NOTE: only support ASCII characters, no UTF8 fancy. As currently
 * the function is only used to lowercase data-filenames if they are
 * not found, this is sufficient. If more, or general functionality is
 * needed, look to r7271 where it was removed because it was broken when
 * using certain locales: eg in Turkish the uppercase 'I' was converted to
 * '?', so just revert to the old functionality
 * @param str string to convert
 * @return String has changed.
 */
bool strtolower(char *str)
{
	bool changed = false;
	for (; *str != '\0'; str++) {
		char new_str = tolower(*str);
		changed |= new_str != *str;
		*str = new_str;
	}
	return changed;
}


/* UTF-8 handling */

/**
 * Decode and consume the next UTF-8 encoded character.
 * @param c Buffer to place decoded character.
 * @param s Character stream to retrieve character from.
 * @return Number of characters in the sequence.
 */
size_t Utf8Decode(WChar *c, const char *s)
{
	assert(c != NULL);

	if (!HasBit(s[0], 7)) {
		/* Single byte character: 0xxxxxxx */
		*c = s[0];
		return 1;
	} else if (GB(s[0], 5, 3) == 6) {
		if (IsUtf8Part(s[1])) {
			/* Double byte character: 110xxxxx 10xxxxxx */
			*c = GB(s[0], 0, 5) << 6 | GB(s[1], 0, 6);
			if (*c >= 0x80) return 2;
		}
	} else if (GB(s[0], 4, 4) == 14) {
		if (IsUtf8Part(s[1]) && IsUtf8Part(s[2])) {
			/* Triple byte character: 1110xxxx 10xxxxxx 10xxxxxx */
			*c = GB(s[0], 0, 4) << 12 | GB(s[1], 0, 6) << 6 | GB(s[2], 0, 6);
			if (*c >= 0x800) return 3;
		}
	} else if (GB(s[0], 3, 5) == 30) {
		if (IsUtf8Part(s[1]) && IsUtf8Part(s[2]) && IsUtf8Part(s[3])) {
			/* 4 byte character: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
			*c = GB(s[0], 0, 3) << 18 | GB(s[1], 0, 6) << 12 | GB(s[2], 0, 6) << 6 | GB(s[3], 0, 6);
			if (*c >= 0x10000 && *c <= 0x10FFFF) return 4;
		}
	}

	/* DEBUG(misc, 1, "[utf8] invalid UTF-8 sequence"); */
	*c = '?';
	return 1;
}


/**
 * Encode a unicode character and place it in the buffer.
 * @param buf Buffer to place character.
 * @param c   Unicode character to encode.
 * @return Number of characters in the encoded sequence.
 */
size_t Utf8Encode(char *buf, WChar c)
{
	if (c < 0x80) {
		*buf = c;
		return 1;
	} else if (c < 0x800) {
		*buf++ = 0xC0 + GB(c,  6, 5);
		*buf   = 0x80 + GB(c,  0, 6);
		return 2;
	} else if (c < 0x10000) {
		*buf++ = 0xE0 + GB(c, 12, 4);
		*buf++ = 0x80 + GB(c,  6, 6);
		*buf   = 0x80 + GB(c,  0, 6);
		return 3;
	} else if (c < 0x110000) {
		*buf++ = 0xF0 + GB(c, 18, 3);
		*buf++ = 0x80 + GB(c, 12, 6);
		*buf++ = 0x80 + GB(c,  6, 6);
		*buf   = 0x80 + GB(c,  0, 6);
		return 4;
	}

	/* DEBUG(misc, 1, "[utf8] can't UTF-8 encode value 0x%X", c); */
	*buf = '?';
	return 1;
}

/**
 * Properly terminate an UTF8 string to some maximum length
 * @param s string to check if it needs additional trimming
 * @param maxlen the maximum length the buffer can have.
 * @return the new length in bytes of the string (eg. strlen(new_string))
 * @note maxlen is the string length _INCLUDING_ the terminating '\0'
 */
size_t Utf8TrimString(char *s, size_t maxlen)
{
	size_t length = 0;

	for (const char *ptr = strchr(s, '\0'); *s != '\0';) {
		size_t len = Utf8EncodedCharLen(*s);
		/* Silently ignore invalid UTF8 sequences, our only concern trimming */
		if (len == 0) len = 1;

		/* Take care when a hard cutoff was made for the string and
		 * the last UTF8 sequence is invalid */
		if (length + len >= maxlen || (s + len > ptr)) break;
		s += len;
		length += len;
	}

	*s = '\0';
	return length;
}

/**
 * Get the length of an UTF-8 encoded string in number of characters
 * and thus not the number of bytes that the encoded string contains.
 * @param s The string to get the length for.
 * @return The length of the string in characters.
 */
size_t Utf8StringLength(const char *s)
{
	size_t len = 0;
	const char *t = s;
	while (Utf8Consume(&t) != 0) len++;
	return len;
}

/**
 * Only allow certain keys. You can define the filter to be used. This makes
 *  sure no invalid keys can get into an editbox, like BELL.
 * @param key character to be checked
 * @param afilter the filter to use
 * @return true or false depending if the character is printable/valid or not
 */
bool IsValidChar(WChar key, CharSetFilter afilter)
{
	switch (afilter) {
		case CS_ALPHANUMERAL:  return IsPrintable(key);
		case CS_NUMERAL:       return (key >= '0' && key <= '9');
		case CS_NUMERAL_SPACE: return (key >= '0' && key <= '9') || key == ' ';
		case CS_ALPHA:         return IsPrintable(key) && !(key >= '0' && key <= '9');
		case CS_HEXADECIMAL:   return (key >= '0' && key <= '9') || (key >= 'a' && key <= 'f') || (key >= 'A' && key <= 'F');
	}

	return false;
}

/**
 * Checks whether the given string is valid, i.e. contains only
 * valid (printable) characters and is properly terminated.
 * @param str  The string to validate.
 * @param last The last character of the string, i.e. the string
 *             must be terminated here or earlier.
 */
bool StrValid(const char *str, const char *last)
{
	/* Assume the ABSOLUTE WORST to be in str as it comes from the outside. */

	while (str <= last && *str != '\0') {
		size_t len = Utf8EncodedCharLen(*str);
		/* Encoded length is 0 if the character isn't known.
		 * The length check is needed to prevent Utf8Decode to read
		 * over the terminating '\0' if that happens to be placed
		 * within the encoding of an UTF8 character. */
		if (len == 0 || str + len > last) return false;

		WChar c;
		len = Utf8Decode(&c, str);
		if (!IsPrintable(c) || (c >= SCC_SPRITE_START && c <= SCC_SPRITE_END)) {
			return false;
		}

		str += len;
	}

	return *str == '\0';
}

/**
 * Scans the string for valid characters and if it finds invalid ones,
 * replaces them with a question mark '?' (if not ignored)
 * @param str the string to validate
 * @param last the last valid character of str
 * @param settings the settings for the string validation.
 */
void str_validate(char *str, const char *last, StringValidationSettings settings)
{
	/* Assume the ABSOLUTE WORST to be in str as it comes from the outside. */

	char *dst = str;
	while (str <= last && *str != '\0') {
		size_t len = Utf8EncodedCharLen(*str);
		/* If the character is unknown, i.e. encoded length is 0
		 * we assume worst case for the length check.
		 * The length check is needed to prevent Utf8Decode to read
		 * over the terminating '\0' if that happens to be placed
		 * within the encoding of an UTF8 character. */
		if ((len == 0 && str + 4 > last) || str + len > last) break;

		WChar c;
		len = Utf8Decode(&c, str);
		/* It's possible to encode the string termination character
		 * into a multiple bytes. This prevents those termination
		 * characters to be skipped */
		if (c == '\0') break;

		if ((IsPrintable(c) && (c < SCC_SPRITE_START || c > SCC_SPRITE_END)) || ((settings & SVS_ALLOW_CONTROL_CODE) != 0 && c == SCC_ENCODED)) {
			/* Copy the character back. Even if dst is current the same as str
			 * (i.e. no characters have been changed) this is quicker than
			 * moving the pointers ahead by len */
			do {
				*dst++ = *str++;
			} while (--len != 0);
		} else if ((settings & SVS_ALLOW_NEWLINE) != 0  && c == '\n') {
			*dst++ = *str++;
		} else {
			if ((settings & SVS_ALLOW_NEWLINE) != 0 && c == '\r' && str[1] == '\n') {
				str += len;
				continue;
			}
			/* Replace the undesirable character with a question mark */
			str += len;
			if ((settings & SVS_REPLACE_WITH_QUESTION_MARK) != 0) *dst++ = '?';
		}
	}

	*dst = '\0';
}

/**
 * Scans the string for valid characters and if it finds invalid ones,
 * replaces them with a question mark '?'.
 * @param str the string to validate
 */
void ValidateString(const char *str)
{
	/* We know it is '\0' terminated. */
	str_validate(const_cast<char *>(str), str + strlen(str) + 1);
}

/**
 * Scan the string for old values of SCC_ENCODED and fix it to
 * it's new, static value.
 * @param str the string to scan
 * @param last the last valid character of str
 */
void str_fix_scc_encoded(char *str, const char *last)
{
	while (str <= last && *str != '\0') {
		size_t len = Utf8EncodedCharLen(*str);
		if ((len == 0 && str + 4 > last) || str + len > last) break;

		WChar c;
		len = Utf8Decode(&c, str);
		if (c == '\0') break;

		if (c == 0xE028 || c == 0xE02A) {
			c = SCC_ENCODED;
		}
		str += Utf8Encode(str, c);
	}
	*str = '\0';
}

/** Scans the string for colour codes and strips them */
void str_strip_colours(char *str)
{
	char *dst = str;
	WChar c;
	size_t len;

	for (len = Utf8Decode(&c, str); c != '\0'; len = Utf8Decode(&c, str)) {
		if (c < SCC_BLUE || c > SCC_BLACK) {
			/* Copy the character back. Even if dst is current the same as str
			 * (i.e. no characters have been changed) this is quicker than
			 * moving the pointers ahead by len */
			do {
				*dst++ = *str++;
			} while (--len != 0);
		} else {
			/* Just skip (strip) the colour codes */
			str += len;
		}
	}
	*dst = '\0';
}


/* buffer-aware string functions */

/** Set this string according to a format and args. */
bool stringb::fmt (const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	bool r = vfmt (fmt, args);
	va_end (args);
	return r;
}

/** Append to this string according to a format and args. */
bool stringb::append_fmt (const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	bool r = append_vfmt (fmt, args);
	va_end (args);
	return r;
}

/** Append a unicode character encoded as utf-8 to the string. */
bool stringb::append_utf8 (WChar c)
{
	assert (len < capacity);
	size_t left = capacity - len;

	if (c < 0x80) {
		if (left <= 1) return false;
		buffer[len++] = c;
	} else if (c < 0x800) {
		if (left <= 2) return false;
		buffer[len++] = 0xC0 + GB(c,  6, 5);
		buffer[len++] = 0x80 + GB(c,  0, 6);
	} else if (c < 0x10000) {
		if (left <= 3) return false;
		buffer[len++] = 0xE0 + GB(c, 12, 4);
		buffer[len++] = 0x80 + GB(c,  6, 6);
		buffer[len++] = 0x80 + GB(c,  0, 6);
	} else if (c < 0x110000) {
		if (left <= 4) return false;
		buffer[len++] = 0xF0 + GB(c, 18, 3);
		buffer[len++] = 0x80 + GB(c, 12, 6);
		buffer[len++] = 0x80 + GB(c,  6, 6);
		buffer[len++] = 0x80 + GB(c,  0, 6);
	} else {
		/* DEBUG(misc, 1, "[utf8] can't UTF-8 encode value 0x%X", c); */
		if (left <= 1) return false;
		buffer[len++] = '?';
	}

	buffer[len] = '\0';
	return true;
}

/** Append the hexadecimal representation of an md5sum. */
bool stringb::append_md5sum (const uint8 md5sum [16])
{
	for (uint i = 0; i < 16; i++) {
		if (!append_fmt ("%02X", md5sum[i])) return false;
	}

	return true;
}
