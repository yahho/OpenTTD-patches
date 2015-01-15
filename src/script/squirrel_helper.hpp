/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file squirrel_helper.hpp declarations and parts of the implementation of the class for convert code */

#ifndef SQUIRREL_HELPER_HPP
#define SQUIRREL_HELPER_HPP

#include "squirrel.hpp"
#include "../core/smallvec_type.hpp"
#include "../economy_type.h"
#include "../string.h"
#include "squirrel_helper_type.hpp"

template <class CL, ScriptType ST> const char *GetClassName();

/**
 * The Squirrel convert routines
 */
namespace SQConvert {
	template <bool Y> struct YesT {
		static const bool Yes = Y;
		static const bool No = !Y;
	};

	/**
	 * Helper class to recognize if the given type is void. Usage: 'IsVoidT<T>::Yes'
	 */
	template <typename T> struct IsVoidT : YesT<false> {};
	template <> struct IsVoidT<void> : YesT<true> {};

	/**
	 * Helper class to recognize if the function/method return type is void.
	 */
	template <typename Tfunc> struct HasVoidReturnT;
	/* functions */
	template <typename Tretval> struct HasVoidReturnT<Tretval (*)()> : IsVoidT<Tretval> {};
	template <typename Tretval, typename Targ1> struct HasVoidReturnT<Tretval (*)(Targ1)> : IsVoidT<Tretval> {};
	template <typename Tretval, typename Targ1, typename Targ2> struct HasVoidReturnT<Tretval (*)(Targ1, Targ2)> : IsVoidT<Tretval> {};
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3> struct HasVoidReturnT<Tretval (*)(Targ1, Targ2, Targ3)> : IsVoidT<Tretval> {};
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4> struct HasVoidReturnT<Tretval (*)(Targ1, Targ2, Targ3, Targ4)> : IsVoidT<Tretval> {};
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5> struct HasVoidReturnT<Tretval (*)(Targ1, Targ2, Targ3, Targ4, Targ5)> : IsVoidT<Tretval> {};
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5, typename Targ6, typename Targ7, typename Targ8, typename Targ9, typename Targ10> struct HasVoidReturnT<Tretval (*)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10)> : IsVoidT<Tretval> {};
	/* methods */
	template <class Tcls, typename Tretval> struct HasVoidReturnT<Tretval (Tcls::*)()> : IsVoidT<Tretval> {};
	template <class Tcls, typename Tretval, typename Targ1> struct HasVoidReturnT<Tretval (Tcls::*)(Targ1)> : IsVoidT<Tretval> {};
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2> struct HasVoidReturnT<Tretval (Tcls::*)(Targ1, Targ2)> : IsVoidT<Tretval> {};
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3> struct HasVoidReturnT<Tretval (Tcls::*)(Targ1, Targ2, Targ3)> : IsVoidT<Tretval> {};
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4> struct HasVoidReturnT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4)> : IsVoidT<Tretval> {};
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5> struct HasVoidReturnT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4, Targ5)> : IsVoidT<Tretval> {};
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5, typename Targ6, typename Targ7, typename Targ8, typename Targ9, typename Targ10> struct HasVoidReturnT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10)> : IsVoidT<Tretval> {};


	/**
	 * To return a value to squirrel, we call this function. It converts to the right format.
	 */
	template <typename T> static int Return(HSQUIRRELVM vm, T t);

	template <> inline int Return<uint8>       (HSQUIRRELVM vm, uint8 res)       { sq_pushinteger(vm, (int32)res); return 1; }
	template <> inline int Return<uint16>      (HSQUIRRELVM vm, uint16 res)      { sq_pushinteger(vm, (int32)res); return 1; }
	template <> inline int Return<uint32>      (HSQUIRRELVM vm, uint32 res)      { sq_pushinteger(vm, (int32)res); return 1; }
	template <> inline int Return<int8>        (HSQUIRRELVM vm, int8 res)        { sq_pushinteger(vm, res); return 1; }
	template <> inline int Return<int16>       (HSQUIRRELVM vm, int16 res)       { sq_pushinteger(vm, res); return 1; }
	template <> inline int Return<int32>       (HSQUIRRELVM vm, int32 res)       { sq_pushinteger(vm, res); return 1; }
	template <> inline int Return<int64>       (HSQUIRRELVM vm, int64 res)       { sq_pushinteger(vm, res); return 1; }
	template <> inline int Return<Money>       (HSQUIRRELVM vm, Money res)       { sq_pushinteger(vm, res); return 1; }
	template <> inline int Return<bool>        (HSQUIRRELVM vm, bool res)        { sq_pushbool   (vm, res); return 1; }
	template <> inline int Return<char *>      (HSQUIRRELVM vm, char *res)       { if (res == NULL) sq_pushnull(vm); else { sq_pushstring(vm, res, -1); free(res); } return 1; }
	template <> inline int Return<const char *>(HSQUIRRELVM vm, const char *res) { if (res == NULL) sq_pushnull(vm); else { sq_pushstring(vm, res, -1); } return 1; }
	template <> inline int Return<void *>      (HSQUIRRELVM vm, void *res)       { sq_pushuserpointer(vm, res); return 1; }
	template <> inline int Return<HSQOBJECT>   (HSQUIRRELVM vm, HSQOBJECT res)   { sq_pushobject(vm, res); return 1; }


	/** Helper function to get an integer from squirrel. */
	static inline SQInteger GetInteger (HSQUIRRELVM vm, int index)
	{
		SQInteger tmp;
		sq_getinteger (vm, index, &tmp);
		return tmp;
	}

	/** Helper function to get a user pointer from squirrel. */
	template <typename T>
	static inline T *GetUserPointer (HSQUIRRELVM vm, int index)
	{
		SQUserPointer instance;
		sq_getinstanceup (vm, index, &instance, 0);
		return (T*) instance;
	}

	/** Encapsulate a param from squirrel. */
	template <typename T>
	struct Param {
		T data;

		Param (HSQUIRRELVM vm, int index);

		inline ~Param () { }

		operator T () { return data; }
	};

	template<> inline Param<uint8> ::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<uint16>::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<uint32>::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<int8>  ::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<int16> ::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<int32> ::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<int64> ::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }
	template<> inline Param<Money> ::Param (HSQUIRRELVM vm, int index) : data (GetInteger (vm, index)) { }

	template<> inline Param<bool>  ::Param (HSQUIRRELVM vm, int index)
	{
		SQBool tmp;
		sq_getbool (vm, index, &tmp);
		data = (tmp != 0);
	}

	template<> inline Param<void*> ::Param (HSQUIRRELVM vm, int index)
	{
		SQUserPointer tmp;
		sq_getuserpointer (vm, index, &tmp);
		data = tmp;
	}

	template<> struct Param <const char *> {
		char *data;

		inline Param (HSQUIRRELVM vm, int index)
		{
			/* Convert what-ever there is as parameter to a string */
			sq_tostring(vm, index);

			const char *tmp;
			sq_getstring(vm, -1, &tmp);
			char *tmp_str = xstrdup(tmp);
			sq_poptop(vm);
			str_validate(tmp_str, tmp_str + strlen(tmp_str));
			data = tmp_str;
		}

		inline ~Param ()
		{
			free (data);
		}

		operator const char * () { return data; }

	private:
		Param (const Param &) DELETED;

		Param & operator = (const Param &) DELETED;
	};

	template<> struct Param <Array*> {
		Array *data;

		inline Param (HSQUIRRELVM vm, int index)
		{
			/* Sanity check of the size. */
			if (sq_getsize(vm, index) > UINT16_MAX) throw sq_throwerror(vm, "an array used as parameter to a function is too large");

			SQObject obj;
			sq_getstackobj(vm, index, &obj);
			sq_pushobject(vm, obj);
			sq_pushnull(vm);

			SmallVector<int32, 2> data;

			while (SQ_SUCCEEDED(sq_next(vm, -2))) {
				SQInteger tmp;
				if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &tmp))) {
					*data.Append() = (int32)tmp;
				} else {
					sq_pop(vm, 4);
					throw sq_throwerror(vm, "a member of an array used as parameter to a function is not numeric");
				}

				sq_pop(vm, 2);
			}
			sq_pop(vm, 2);

			Array *arr = (Array*) xmalloc (sizeof(Array) + sizeof(int32) * data.Length());
			arr->size = data.Length();
			memcpy(arr->array, data.Begin(), sizeof(int32) * data.Length());

			this->data = arr;
		}

		inline ~Param (void)
		{
			free (data);
		}

		operator Array * () { return data; }

	private:
		Param (const Param &) DELETED;

		Param & operator = (const Param &) DELETED;
	};


	/**
	 * Helper class to recognize the function type (retval type, args) and use the proper specialization
	 * for SQ callback. The partial specializations for the second arg (Tis_void_retval) are not possible
	 * on the function. Therefore the class is used instead.
	 */
	template <typename Tfunc, bool Tis_void_retval = HasVoidReturnT<Tfunc>::Yes> struct HelperT;

	/**
	 * The real C++ caller for function with return value and 0 params.
	 */
	template <typename Tretval>
	struct HelperT<Tretval (*)(), false> {
		static int SQCall(void *instance, Tretval (*func)(), HSQUIRRELVM vm)
		{
			return Return(vm, (*func)());
		}
	};

	/**
	 * The real C++ caller for function with no return value and 0 params.
	 */
	template <typename Tretval>
	struct HelperT<Tretval (*)(), true> {
		static int SQCall(void *instance, Tretval (*func)(), HSQUIRRELVM vm)
		{
			(*func)();
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 0 params.
	 */
	template <class Tcls, typename Tretval>
	struct HelperT<Tretval (Tcls::*)(), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(), HSQUIRRELVM vm)
		{
			return Return(vm, (instance->*func)());
		}
	};

	/**
	 * The real C++ caller for method with no return value and 0 params.
	 */
	template <class Tcls, typename Tretval>
	struct HelperT<Tretval (Tcls::*)(), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(), HSQUIRRELVM vm)
		{
			(instance->*func)();
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(), HSQUIRRELVM vm)
		{
			return new Tcls();
		}
	};

	/**
	 * The real C++ caller for function with return value and 1 param.
	 */
	template <typename Tretval, typename Targ1>
	struct HelperT<Tretval (*)(Targ1), false> {
		static int SQCall(void *instance, Tretval (*func)(Targ1), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Tretval ret = (*func) (param1);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for function with no return value and 1 param.
	 */
	template <typename Tretval, typename Targ1>
	struct HelperT<Tretval (*)(Targ1), true> {
		static int SQCall(void *instance, Tretval (*func)(Targ1), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			(*func) (param1);
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 1 param.
	 */
	template <class Tcls, typename Tretval, typename Targ1>
	struct HelperT<Tretval (Tcls::*)(Targ1), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Tretval ret = (instance->*func) (param1);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for method with no return value and 1 param.
	 */
	template <class Tcls, typename Tretval, typename Targ1>
	struct HelperT<Tretval (Tcls::*)(Targ1), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			(instance->*func) (param1);
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(Targ1), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			return new Tcls (param1);
		}
	};

	/**
	 * The real C++ caller for function with return value and 2 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2>
	struct HelperT<Tretval (*)(Targ1, Targ2), false> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Tretval ret = (*func) (param1, param2);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for function with no return value and 2 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2>
	struct HelperT<Tretval (*)(Targ1, Targ2), true> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			(*func) (param1, param2);
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 2 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Tretval ret = (instance->*func) (param1, param2);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for method with no return value and 2 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			(instance->*func) (param1, param2);
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			return new Tcls (param1, param2);
		}
	};

	/**
	 * The real C++ caller for function with return value and 3 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3), false> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Tretval ret = (*func) (param1, param2, param3);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for function with no return value and 3 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3), true> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			(*func) (param1, param2, param3);
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 3 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Tretval ret = (instance->*func) (param1, param2, param3);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for method with no return value and 3 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			(instance->*func) (param1, param2, param3);
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			return new Tcls (param1, param2, param3);
		}
	};

	/**
	 * The real C++ caller for function with return value and 4 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3, Targ4), false> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3, Targ4), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Tretval ret = (*func) (param1, param2, param3, param4);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for function with no return value and 4 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3, Targ4), true> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3, Targ4), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			(*func) (param1, param2, param3, param4);
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 4 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Tretval ret = (instance->*func) (param1, param2, param3, param4);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for method with no return value and 4 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			(instance->*func) (param1, param2, param3, param4);
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			return new Tcls (param1, param2, param3, param4);
		}
	};

	/**
	 * The real C++ caller for function with return value and 5 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3, Targ4, Targ5), false> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3, Targ4, Targ5), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Tretval ret = (*func) (param1, param2, param3, param4, param5);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for function with no return value and 5 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3, Targ4, Targ5), true> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3, Targ4, Targ5), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			(*func) (param1, param2, param3, param4, param5);
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 5 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4, Targ5), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4, Targ5), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Tretval ret = (instance->*func) (param1, param2, param3, param4, param5);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for method with no return value and 5 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4, Targ5), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4, Targ5), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			(instance->*func) (param1, param2, param3, param4, param5);
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4, Targ5), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			return new Tcls (param1, param2, param3, param4, param5);
		}
	};

	/**
	 * The real C++ caller for function with return value and 10 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5, typename Targ6, typename Targ7, typename Targ8, typename Targ9, typename Targ10>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), false> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Param<Targ6> param6 (vm, 7);
			Param<Targ7> param7 (vm, 8);
			Param<Targ8> param8 (vm, 9);
			Param<Targ9> param9 (vm, 10);
			Param<Targ10> param10 (vm, 11);
			Tretval ret = (*func) (param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for function with no return value and 10 params.
	 */
	template <typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5, typename Targ6, typename Targ7, typename Targ8, typename Targ9, typename Targ10>
	struct HelperT<Tretval (*)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), true> {
		static int SQCall(void *instance, Tretval (*func)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Param<Targ6> param6 (vm, 7);
			Param<Targ7> param7 (vm, 8);
			Param<Targ8> param8 (vm, 9);
			Param<Targ9> param9 (vm, 10);
			Param<Targ10> param10 (vm, 11);
			(*func) (param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
			return 0;
		}
	};

	/**
	 * The real C++ caller for method with return value and 10 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5, typename Targ6, typename Targ7, typename Targ8, typename Targ9, typename Targ10>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), false> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Param<Targ6> param6 (vm, 7);
			Param<Targ7> param7 (vm, 8);
			Param<Targ8> param8 (vm, 9);
			Param<Targ9> param9 (vm, 10);
			Param<Targ10> param10 (vm, 11);
			Tretval ret = (instance->*func) (param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
			return Return(vm, ret);
		}
	};

	/**
	 * The real C++ caller for method with no return value and 10 params.
	 */
	template <class Tcls, typename Tretval, typename Targ1, typename Targ2, typename Targ3, typename Targ4, typename Targ5, typename Targ6, typename Targ7, typename Targ8, typename Targ9, typename Targ10>
	struct HelperT<Tretval (Tcls::*)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), true> {
		static int SQCall(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Param<Targ6> param6 (vm, 7);
			Param<Targ7> param7 (vm, 8);
			Param<Targ8> param8 (vm, 9);
			Param<Targ9> param9 (vm, 10);
			Param<Targ10> param10 (vm, 11);
			(instance->*func) (param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
			return 0;
		}

		static Tcls *SQConstruct(Tcls *instance, Tretval (Tcls::*func)(Targ1, Targ2, Targ3, Targ4, Targ5, Targ6, Targ7, Targ8, Targ9, Targ10), HSQUIRRELVM vm)
		{
			Param<Targ1> param1 (vm, 2);
			Param<Targ2> param2 (vm, 3);
			Param<Targ3> param3 (vm, 4);
			Param<Targ4> param4 (vm, 5);
			Param<Targ5> param5 (vm, 6);
			Param<Targ6> param6 (vm, 7);
			Param<Targ7> param7 (vm, 8);
			Param<Targ8> param8 (vm, 9);
			Param<Targ9> param9 (vm, 10);
			Param<Targ10> param10 (vm, 11);
			return new Tcls (param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
		}
	};


	/**
	 * A general template for all non-static method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tcls, typename Tmethod, ScriptType Ttype>
	inline SQInteger DefSQNonStaticCallback(HSQUIRRELVM vm)
	{
		/* Find the amount of params we got */
		int nparam = sq_gettop(vm);
		SQUserPointer ptr = NULL;
		SQUserPointer real_instance = NULL;
		HSQOBJECT instance;

		/* Get the 'SQ' instance of this class */
		Squirrel::GetInstance(vm, &instance);

		/* Protect against calls to a non-static method in a static way */
		sq_pushroottable(vm);
		const char *className = GetClassName<Tcls, Ttype>();
		sq_pushstring(vm, className, -1);
		sq_get(vm, -2);
		sq_pushobject(vm, instance);
		if (sq_instanceof(vm) != SQTrue) return sq_throwerror(vm, "class method is non-static");
		sq_pop(vm, 3);

		/* Get the 'real' instance of this class */
		sq_getinstanceup(vm, 1, &real_instance, 0);
		/* Get the real function pointer */
		sq_getuserdata(vm, nparam, &ptr, 0);
		if (real_instance == NULL) return sq_throwerror(vm, "couldn't detect real instance of class for non-static call");
		/* Remove the userdata from the stack */
		sq_pop(vm, 1);

		try {
			/* Delegate it to a template that can handle this specific function */
			return HelperT<Tmethod>::SQCall((Tcls *)real_instance, *(Tmethod *)ptr, vm);
		} catch (SQInteger e) {
			return e;
		}
	}

	/**
	 * A general template for all non-static advanced method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tcls, typename Tmethod, ScriptType Ttype>
	inline SQInteger DefSQAdvancedNonStaticCallback(HSQUIRRELVM vm)
	{
		/* Find the amount of params we got */
		int nparam = sq_gettop(vm);
		SQUserPointer ptr = NULL;
		SQUserPointer real_instance = NULL;
		HSQOBJECT instance;

		/* Get the 'SQ' instance of this class */
		Squirrel::GetInstance(vm, &instance);

		/* Protect against calls to a non-static method in a static way */
		sq_pushroottable(vm);
		const char *className = GetClassName<Tcls, Ttype>();
		sq_pushstring(vm, className, -1);
		sq_get(vm, -2);
		sq_pushobject(vm, instance);
		if (sq_instanceof(vm) != SQTrue) return sq_throwerror(vm, "class method is non-static");
		sq_pop(vm, 3);

		/* Get the 'real' instance of this class */
		sq_getinstanceup(vm, 1, &real_instance, 0);
		/* Get the real function pointer */
		sq_getuserdata(vm, nparam, &ptr, 0);
		if (real_instance == NULL) return sq_throwerror(vm, "couldn't detect real instance of class for non-static call");
		/* Remove the userdata from the stack */
		sq_pop(vm, 1);

		/* Call the function, which its only param is always the VM */
		return (SQInteger)(((Tcls *)real_instance)->*(*(Tmethod *)ptr))(vm);
	}

	/**
	 * A general template for all function/static method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tcls, typename Tmethod>
	inline SQInteger DefSQStaticCallback(HSQUIRRELVM vm)
	{
		/* Find the amount of params we got */
		int nparam = sq_gettop(vm);
		SQUserPointer ptr = NULL;

		/* Get the real function pointer */
		sq_getuserdata(vm, nparam, &ptr, 0);

		try {
			/* Delegate it to a template that can handle this specific function */
			return HelperT<Tmethod>::SQCall((Tcls *)NULL, *(Tmethod *)ptr, vm);
		} catch (SQInteger e) {
			return e;
		}
	}


	/**
	 * A general template for all static advanced method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tcls, typename Tmethod>
	inline SQInteger DefSQAdvancedStaticCallback(HSQUIRRELVM vm)
	{
		/* Find the amount of params we got */
		int nparam = sq_gettop(vm);
		SQUserPointer ptr = NULL;

		/* Get the real function pointer */
		sq_getuserdata(vm, nparam, &ptr, 0);
		/* Remove the userdata from the stack */
		sq_pop(vm, 1);

		/* Call the function, which its only param is always the VM */
		return (SQInteger)(*(*(Tmethod *)ptr))(vm);
	}

	/**
	 * A general template for the destructor of SQ instances. This is needed
	 *  here as it has to be in the same scope as DefSQConstructorCallback.
	 */
	template <typename Tcls>
	static SQInteger DefSQDestructorCallback(SQUserPointer p, SQInteger size)
	{
		/* Remove the real instance too */
		if (p != NULL) ((Tcls *)p)->Release();
		return 0;
	}

	/**
	 * A general template to handle creating of instance with any amount of
	 *  params. It creates the instance in C++, and it sets all the needed
	 *  settings in SQ to register the instance.
	 */
	template <typename Tcls, typename Tmethod, int Tnparam>
	inline SQInteger DefSQConstructorCallback(HSQUIRRELVM vm)
	{
		try {
			/* Create the real instance */
			Tcls *instance = HelperT<Tmethod>::SQConstruct((Tcls *)NULL, (Tmethod)NULL, vm);
			sq_setinstanceup(vm, -Tnparam, instance);
			sq_setreleasehook(vm, -Tnparam, DefSQDestructorCallback<Tcls>);
			instance->AddRef();
			return 0;
		} catch (SQInteger e) {
			return e;
		}
	}

	/**
	 * A general template to handle creating of an instance with a complex
	 *  constructor.
	 */
	template <typename Tcls>
	inline SQInteger DefSQAdvancedConstructorCallback(HSQUIRRELVM vm)
	{
		try {
			/* Find the amount of params we got */
			int nparam = sq_gettop(vm);

			/* Create the real instance */
			Tcls *instance = new Tcls(vm);
			sq_setinstanceup(vm, -nparam, instance);
			sq_setreleasehook(vm, -nparam, DefSQDestructorCallback<Tcls>);
			instance->AddRef();
			return 0;
		} catch (SQInteger e) {
			return e;
		}
	}

} // namespace SQConvert

#endif /* SQUIRREL_HELPER_HPP */
