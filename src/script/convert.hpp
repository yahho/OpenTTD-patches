/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file convert.hpp Conversion of types to and from squirrel. */

#ifndef SQUIRREL_CONVERT_HPP
#define SQUIRREL_CONVERT_HPP

#include "squirrel.hpp"
#include "../core/pointer.h"
#include "../core/smallvec_type.hpp"
#include "../economy_type.h"
#include "../string.h"

/** Definition of a simple array. */
struct Array {
	int32 size;    ///< The size of the array.
	int32 array[]; ///< The data of the array.
};

/**
 * The Squirrel convert routines
 */
namespace SQConvert {

	/** Helper function to get an integer from squirrel. */
	static inline SQInteger GetInteger (HSQUIRRELVM vm, int index)
	{
		SQInteger tmp;
		sq_getinteger (vm, index, &tmp);
		return tmp;
	}

	/** Helper function to get a string from squirrel. */
	char *GetString (HSQUIRRELVM vm, int index);

	/** Helper function to get a user pointer from squirrel. */
	template <typename T>
	static inline T *GetUserPointer (HSQUIRRELVM vm, int index)
	{
		SQUserPointer instance;
		sq_getinstanceup (vm, index, &instance, 0);
		return (T*) instance;
	}

	/** Default param constructor function for struct Param below. */
	template <typename T>
	T GetParam (HSQUIRRELVM vm, int index);

	/** Encapsulate a param from squirrel. */
	template <typename T>
	struct Param {
		T data;

		Param (HSQUIRRELVM vm, int index)
			: data (GetParam<T> (vm, index))
		{
		}

		inline ~Param () { }

		operator T () { return data; }
	};

	template<> inline uint8  GetParam<uint8>  (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline uint16 GetParam<uint16> (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline uint32 GetParam<uint32> (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline int8   GetParam<int8>   (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline int16  GetParam<int16>  (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline int32  GetParam<int32>  (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline int64  GetParam<int64>  (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }
	template<> inline Money  GetParam<Money>  (HSQUIRRELVM vm, int index) { return GetInteger (vm, index); }

	template<> inline bool GetParam<bool> (HSQUIRRELVM vm, int index)
	{
		SQBool tmp;
		sq_getbool (vm, index, &tmp);
		return (tmp != 0);
	}

	template<> inline void *GetParam<void*> (HSQUIRRELVM vm, int index)
	{
		SQUserPointer tmp;
		sq_getuserpointer (vm, index, &tmp);
		return tmp;
	}

	template<> struct Param <const char *> {
		ttd_unique_free_ptr<char> data;

		inline Param (HSQUIRRELVM vm, int index)
			: data (GetString (vm, index))
		{
		}

		operator const char * () { return data.get(); }
	};

	static inline Array *GetArray (HSQUIRRELVM vm, int index)
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
		return arr;
	}

	template<> struct Param <Array*> {
		ttd_unique_free_ptr<Array> data;

		inline Param (HSQUIRRELVM vm, int index)
			: data (GetArray (vm, index))
		{
		}

		operator Array * () { return data.get(); }
	};


	/** Param class for squirrel function calls without parameters. */
	struct Params0 {
		static const uint N = 0;

		CONSTEXPR Params0 (HSQUIRRELVM vm)
		{
		}

		void callv (void (*func) (void))
		{
			(*func) ();
		}

		template <typename R>
		R call (R (*func) (void))
		{
			return (*func) ();
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (void))
		{
			(instance->*func) ();
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (void))
		{
			return (instance->*func) ();
		}

		template <class C>
		C *construct (void)
		{
			return new C ();
		}
	};

	/** Param class for squirrel function calls with 1 parameter. */
	template <typename T1>
	struct Params1 : Params0 {
		static const uint N = 1;

		Param<T1> param1;

		Params1 (HSQUIRRELVM vm) : Params0 (vm), param1 (vm, 2)
		{
		}

		void callv (void (*func) (T1))
		{
			(*func) (this->param1);
		}

		template <typename R>
		R call (R (*func) (T1))
		{
			return (*func) (this->param1);
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (T1))
		{
			(instance->*func) (this->param1);
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (T1))
		{
			return (instance->*func) (this->param1);
		}

		template <class C>
		C *construct (void)
		{
			return new C (this->param1);
		}
	};

	/** Param class for squirrel function calls with 2 parameters. */
	template <typename T1, typename T2>
	struct Params2 : Params1 <T1> {
		static const uint N = 2;

		Param<T2> param2;

		Params2 (HSQUIRRELVM vm) : Params1 <T1> (vm), param2 (vm, 3)
		{
		}

		void callv (void (*func) (T1, T2))
		{
			(*func) (this->param1, this->param2);
		}

		template <typename R>
		R call (R (*func) (T1, T2))
		{
			return (*func) (this->param1, this->param2);
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (T1, T2))
		{
			(instance->*func) (this->param1, this->param2);
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (T1, T2))
		{
			return (instance->*func) (this->param1, this->param2);
		}

		template <class C>
		C *construct (void)
		{
			return new C (this->param1, this->param2);
		}
	};

	/** Param class for squirrel function calls with 3 parameters. */
	template <typename T1, typename T2, typename T3>
	struct Params3 : Params2 <T1, T2> {
		static const uint N = 3;

		Param<T3> param3;

		Params3 (HSQUIRRELVM vm) : Params2 <T1, T2> (vm),
			param3 (vm, 4)
		{
		}

		void callv (void (*func) (T1, T2, T3))
		{
			(*func) (this->param1, this->param2,
					this->param3);
		}

		template <typename R>
		R call (R (*func) (T1, T2, T3))
		{
			return (*func) (this->param1, this->param2,
					this->param3);
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (T1, T2, T3))
		{
			(instance->*func) (this->param1, this->param2,
					this->param3);
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (T1, T2, T3))
		{
			return (instance->*func) (this->param1, this->param2,
					this->param3);
		}

		template <class C>
		C *construct (void)
		{
			return new C (this->param1, this->param2,
					this->param3);
		}
	};

	/** Param class for squirrel function calls with 4 parameters. */
	template <typename T1, typename T2, typename T3, typename T4>
	struct Params4 : Params3 <T1, T2, T3> {
		static const uint N = 4;

		Param<T4> param4;

		Params4 (HSQUIRRELVM vm) : Params3 <T1, T2, T3> (vm),
			param4 (vm, 5)
		{
		}

		void callv (void (*func) (T1, T2, T3, T4))
		{
			(*func) (this->param1, this->param2,
					this->param3, this->param4);
		}

		template <typename R>
		R call (R (*func) (T1, T2, T3, T4))
		{
			return (*func) (this->param1, this->param2,
					this->param3, this->param4);
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (T1, T2, T3, T4))
		{
			(instance->*func) (this->param1, this->param2,
					this->param3, this->param4);
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (T1, T2, T3, T4))
		{
			return (instance->*func) (this->param1, this->param2,
					this->param3, this->param4);
		}

		template <class C>
		C *construct (void)
		{
			return new C (this->param1, this->param2,
					this->param3, this->param4);
		}
	};

	/** Param class for squirrel function calls with 5 parameters. */
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	struct Params5 : Params4 <T1, T2, T3, T4> {
		static const uint N = 5;

		Param<T5> param5;

		Params5 (HSQUIRRELVM vm) : Params4 <T1, T2, T3, T4> (vm),
			param5 (vm, 6)
		{
		}

		void callv (void (*func) (T1, T2, T3, T4, T5))
		{
			(*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5);
		}

		template <typename R>
		R call (R (*func) (T1, T2, T3, T4, T5))
		{
			return (*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5);
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (T1, T2, T3, T4, T5))
		{
			(instance->*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5);
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (T1, T2, T3, T4, T5))
		{
			return (instance->*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5);
		}

		template <class C>
		C *construct (void)
		{
			return new C (this->param1, this->param2,
					this->param3, this->param4,
					this->param5);
		}
	};

	/** Param class for squirrel function calls with 10 parameters. */
	template <typename T1, typename T2, typename T3, typename T4,
			typename T5, typename T6, typename T7, typename T8,
			typename T9, typename T10>
	struct Params10 : Params5 <T1, T2, T3, T4, T5> {
		static const uint N = 10;

		Param<T6> param6;
		Param<T7> param7;
		Param<T8> param8;
		Param<T9> param9;
		Param<T10> param10;

		Params10 (HSQUIRRELVM vm) : Params5 <T1, T2, T3, T4, T5> (vm),
			param6 (vm, 7), param7 (vm, 8), param8 (vm, 9),
			param9 (vm, 10), param10 (vm, 11)
		{
		}

		void callv (void (*func) (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10))
		{
			(*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5, this->param6,
					this->param7, this->param8,
					this->param9, this->param10);
		}

		template <typename R>
		R call (R (*func) (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10))
		{
			return (*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5, this->param6,
					this->param7, this->param8,
					this->param9, this->param10);
		}

		template <class C>
		void callmv (C *instance, void (C::*func) (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10))
		{
			(instance->*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5, this->param6,
					this->param7, this->param8,
					this->param9, this->param10);
		}

		template <class C, typename R>
		R callm (C *instance, R (C::*func) (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10))
		{
			return (instance->*func) (this->param1, this->param2,
					this->param3, this->param4,
					this->param5, this->param6,
					this->param7, this->param8,
					this->param9, this->param10);
		}

		template <class C>
		C *construct (void)
		{
			return new C (this->param1, this->param2,
					this->param3, this->param4,
					this->param5, this->param6,
					this->param7, this->param8,
					this->param9, this->param10);
		}
	};


	/** Helper class to identify the signature of a function. */
	template <typename F> struct FSig;

	template <typename R>
	struct FRet {
		typedef R Ret;
	};

	template <typename R>
	struct FSig <R (void)> : FRet <R> {
		typedef Params0 Params;
	};

	template <typename R, typename T1>
	struct FSig <R (T1)> : FRet <R> {
		typedef Params1 <T1> Params;
	};

	template <typename R, typename T1, typename T2>
	struct FSig <R (T1, T2)> : FRet <R> {
		typedef Params2 <T1, T2> Params;
	};

	template <typename R, typename T1, typename T2, typename T3>
	struct FSig <R (T1, T2, T3)> : FRet <R> {
		typedef Params3 <T1, T2, T3> Params;
	};

	template <typename R, typename T1, typename T2, typename T3,
			typename T4>
	struct FSig <R (T1, T2, T3, T4)> : FRet <R> {
		typedef Params4 <T1, T2, T3, T4> Params;
	};

	template <typename R, typename T1, typename T2, typename T3,
			typename T4, typename T5>
	struct FSig <R (T1, T2, T3, T4, T5)> : FRet <R> {
		typedef Params5 <T1, T2, T3, T4, T5> Params;
	};

	template <typename R, typename T1, typename T2, typename T3,
			typename T4, typename T5, typename T6, typename T7,
			typename T8, typename T9, typename T10>
	struct FSig <R (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)> : FRet <R> {
		typedef Params10 <T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> Params;
	};

	template <typename F>
	struct FSig <F*> : FSig <F> {
	};

	/*
	 * Now this should work for class methods:
	 *      template <class C, typename F>
	 *      struct FSig <F C::*> : FSig <F> {
	 *              typedef C Cls;
	 *      };
	 * ...but MSVC chokes on it, so we must repeat everything.
	 */

	template <typename C>
	struct FCls {
		typedef C Cls;
	};

	template <class C, typename R>
	struct FSig <R (C::*) (void)> : FRet <R>, FCls <C> {
		typedef Params0 Params;
	};

	template <class C, typename R, typename T1>
	struct FSig <R (C::*) (T1)> : FRet <R>, FCls <C> {
		typedef Params1 <T1> Params;
	};

	template <class C, typename R, typename T1, typename T2>
	struct FSig <R (C::*) (T1, T2)> : FRet <R>, FCls <C> {
		typedef Params2 <T1, T2> Params;
	};

	template <class C, typename R, typename T1, typename T2, typename T3>
	struct FSig <R (C::*) (T1, T2, T3)> : FRet <R>, FCls <C> {
		typedef Params3 <T1, T2, T3> Params;
	};

	template <class C, typename R, typename T1, typename T2, typename T3,
			typename T4>
	struct FSig <R (C::*) (T1, T2, T3, T4)> : FRet <R>, FCls <C> {
		typedef Params4 <T1, T2, T3, T4> Params;
	};

	template <class C, typename R, typename T1, typename T2, typename T3,
			typename T4, typename T5>
	struct FSig <R (C::*) (T1, T2, T3, T4, T5)> : FRet <R>, FCls <C> {
		typedef Params5 <T1, T2, T3, T4, T5> Params;
	};

	template <class C, typename R, typename T1, typename T2, typename T3,
			typename T4, typename T5, typename T6, typename T7,
			typename T8, typename T9, typename T10>
	struct FSig <R (C::*) (T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)> : FRet <R>, FCls <C> {
		typedef Params10 <T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> Params;
	};


	/** Push a value onto the squirrel stack. */
	template <typename T>
	inline void Push (HSQUIRRELVM vm, T res)
	{
		sq_pushinteger (vm, static_cast<int32>(res));
	}

	template <> inline void Push<uint8>       (HSQUIRRELVM vm, uint8 res)       { sq_pushinteger (vm, (int32)res); }
	template <> inline void Push<uint16>      (HSQUIRRELVM vm, uint16 res)      { sq_pushinteger (vm, (int32)res); }
	template <> inline void Push<uint32>      (HSQUIRRELVM vm, uint32 res)      { sq_pushinteger (vm, (int32)res); }
	template <> inline void Push<int8>        (HSQUIRRELVM vm, int8 res)        { sq_pushinteger (vm, res); }
	template <> inline void Push<int16>       (HSQUIRRELVM vm, int16 res)       { sq_pushinteger (vm, res); }
	template <> inline void Push<int32>       (HSQUIRRELVM vm, int32 res)       { sq_pushinteger (vm, res); }
	template <> inline void Push<int64>       (HSQUIRRELVM vm, int64 res)       { sq_pushinteger (vm, res); }
	template <> inline void Push<Money>       (HSQUIRRELVM vm, Money res)       { sq_pushinteger (vm, res); }
	template <> inline void Push<bool>        (HSQUIRRELVM vm, bool res)        { sq_pushbool    (vm, res); }

	template <>
	inline void Push<char *> (HSQUIRRELVM vm, char *res)
	{
		if (res == NULL) {
			sq_pushnull (vm);
		} else {
			sq_pushstring (vm, res, -1);
			free(res);
		}
	}

	template <>
	inline void Push<const char *> (HSQUIRRELVM vm, const char *res)
	{
		if (res == NULL) {
			sq_pushnull (vm);
		} else {
			sq_pushstring (vm, res, -1);
		}
	}

	template <>
	inline void Push<void *> (HSQUIRRELVM vm, void *res)
	{
		sq_pushuserpointer (vm, res);
	}

	template <>
	inline void Push<HSQOBJECT> (HSQUIRRELVM vm, HSQOBJECT res)
	{
		sq_pushobject (vm, res);
	}


	/** Helper class to return a value to squirrel. */
	template <typename R>
	struct SQRetVal {
		R value;

		template <typename P, typename F>
		SQRetVal (P *params, F func) : value (params->call (func))
		{
		}

		template <typename P, class C, typename F>
		SQRetVal (P *params, C *instance, F func)
			: value (params->callm (instance, func))
		{
		}

		int Return (HSQUIRRELVM vm) const
		{
			Push (vm, this->value);
			return 1;
		}
	};

	template <>
	struct SQRetVal<void> {
		template <typename P, typename F>
		SQRetVal (P *params, F func)
		{
			params->callv (func);
		}

		template <typename P, class C, typename F>
		SQRetVal (P *params, C *instance, F func)
		{
			params->callmv (instance, func);
		}

		static int Return (HSQUIRRELVM vm)
		{
			return 0;
		}
	};


	/** Method callback data. */
	template <typename Tmethod>
	struct MethodCallbackData {
		const char *cname;
		Tmethod method;
	};

	/** Get the object and method pointers for a non-static call. */
	const char *GetMethodPointers (HSQUIRRELVM vm,
		SQUserPointer *obj, SQUserPointer *data);

	/**
	 * A general template for all non-static method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tmethod>
	inline SQInteger DefSQNonStaticCallback(HSQUIRRELVM vm)
	{
		typedef FSig<Tmethod> Sig;

		SQUserPointer obj = NULL;
		SQUserPointer ptr = NULL;
		const char *err = GetMethodPointers (vm, &obj, &ptr);
		if (err != NULL) return sq_throwerror (vm, err);

		typename Sig::Cls *instance = (typename Sig::Cls *) obj;
		Tmethod method = ((MethodCallbackData <Tmethod> *) ptr)->method;

		try {
			/* Delegate it to a template that can handle this specific function */
			typename Sig::Params params (vm);
			SQRetVal <typename Sig::Ret> ret (&params,
						instance, method);
			return ret.Return (vm);
		} catch (SQInteger e) {
			return e;
		}
	}

	/**
	 * This defines a method inside a class for Squirrel with defined params.
	 * @note If you define nparam, make sure that he first param is always 'x',
	 *  which is the 'this' inside the function. This is hidden from the rest
	 *  of the code, but without it calling your function will fail!
	 */
	template <typename Tmethod>
	inline void DefSQMethod (Squirrel *engine, const char *class_name,
		Tmethod function_proc, const char *function_name,
		int nparam, const char *params)
	{
		MethodCallbackData <Tmethod> data = { class_name, function_proc };
		assert_tcompile ((const char**)(SQUserPointer)&data == &data.cname);
		engine->AddMethod (function_name, DefSQNonStaticCallback<Tmethod>, nparam, params, &data, sizeof(data));
	}

	/**
	 * A general template for all non-static advanced method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tcls>
	inline SQInteger DefSQAdvancedNonStaticCallback(HSQUIRRELVM vm)
	{
		SQUserPointer obj = NULL;
		SQUserPointer ptr = NULL;
		const char *err = GetMethodPointers (vm, &obj, &ptr);
		if (err != NULL) return sq_throwerror (vm, err);

		/* Call the function, which its only param is always the VM */
		Tcls *instance = (Tcls *) obj;
		typedef SQInteger (Tcls::*F) (HSQUIRRELVM);
		F method = ((MethodCallbackData <F> *) ptr)->method;
		return (instance->*method) (vm);
	}

	/**
	 * This defines a method inside a class for Squirrel, which has access to the 'engine' (experts only!).
	 */
	template <typename Tcls>
	void DefSQAdvancedMethod (Squirrel *engine, const char *class_name,
		SQInteger (Tcls::*function_proc) (HSQUIRRELVM),
		const char *function_name)
	{
		typedef SQInteger (Tcls::*F) (HSQUIRRELVM);
		MethodCallbackData <F> data = { class_name, function_proc };
		assert_tcompile ((const char**)(SQUserPointer)&data == &data.cname);
		engine->AddMethod (function_name, DefSQAdvancedNonStaticCallback <Tcls>, 0, NULL, &data, sizeof(data));
	}

	/**
	 * A general template for all function/static method callbacks from Squirrel.
	 *  In here the function_proc is recovered, and the SQCall is called that
	 *  can handle this exact amount of params.
	 */
	template <typename Tmethod>
	inline SQInteger DefSQStaticCallback(HSQUIRRELVM vm)
	{
		/* Find the amount of params we got */
		int nparam = sq_gettop(vm);
		SQUserPointer ptr = NULL;

		/* Get the real function pointer */
		sq_getuserdata(vm, nparam, &ptr, 0);

		try {
			/* Delegate it to a template that can handle this specific function */
			typedef FSig<Tmethod> Sig;
			typename Sig::Params params (vm);
			SQRetVal <typename Sig::Ret> ret (&params,
						*(Tmethod *)ptr);
			return ret.Return (vm);
		} catch (SQInteger e) {
			return e;
		}
	}

	/** This defines a static method inside a class for Squirrel. */
	template <typename Func>
	inline void DefSQStaticMethod (Squirrel *engine, Func function_proc, const char *function_name)
	{
		engine->AddMethod (function_name, DefSQStaticCallback<Func>, 0, NULL, &function_proc, sizeof(function_proc));
	}

	/**
	 * This defines a static method inside a class for Squirrel with defined params.
	 * @note If you define nparam, make sure that he first param is always 'x',
	 *  which is the 'this' inside the function. This is hidden from the rest
	 *  of the code, but without it calling your function will fail!
	 */
	template <typename Func>
	inline void DefSQStaticMethod (Squirrel *engine, Func function_proc, const char *function_name, int nparam, const char *params)
	{
		engine->AddMethod (function_name, DefSQStaticCallback<Func>, nparam, params, &function_proc, sizeof(function_proc));
	}

	/**
	 * This defines a static method inside a class for Squirrel, which has access to the 'engine' (experts only!).
	 */
	inline void DefSQAdvancedStaticMethod (Squirrel *engine, SQInteger (*function_proc) (HSQUIRRELVM), const char *function_name)
	{
		engine->AddMethod (function_name, function_proc);
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
	template <typename Tmethod>
	inline SQInteger DefSQConstructorCallback(HSQUIRRELVM vm)
	{
		typedef FSig<Tmethod> Sig;

		try {
			/* Create the real instance */
			typename Sig::Params params (vm);
			typename Sig::Cls *instance = params.template construct <typename Sig::Cls> ();
			int nparam = Sig::Params::N + 1;
			sq_setinstanceup (vm, -nparam, instance);
			sq_setreleasehook (vm, -nparam, DefSQDestructorCallback<typename Sig::Cls>);
			instance->AddRef();
			return 0;
		} catch (SQInteger e) {
			return e;
		}
	}

	template <typename Tmethod, int Tnparam>
	inline void AddConstructor (Squirrel *engine, const char *params)
	{
		assert_tcompile (Tnparam == FSig<Tmethod>::Params::N + 1);
		engine->AddMethod ("constructor", DefSQConstructorCallback<Tmethod>, Tnparam, params);
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

	template <typename Tcls>
	inline void AddSQAdvancedConstructor (Squirrel *engine)
	{
		engine->AddMethod ("constructor", DefSQAdvancedConstructorCallback<Tcls>, 0, NULL);
	}


	/** Helper struct for the generic implementation of GetParam.
	 * (partial specialisations of function templates are not allowed) */
	template <typename T>
	struct GetParamHelper {
		static T Get (HSQUIRRELVM vm, int index)
		{
			return static_cast<T> (GetInteger (vm, index));
		}
	};

	template <typename T>
	struct GetParamHelper <T *> {
		static T *Get (HSQUIRRELVM vm, int index)
		{
			return GetUserPointer<T> (vm, index);
		}
	};

	template <typename T>
	struct GetParamHelper <T &> {
		static T &Get (HSQUIRRELVM vm, int index)
		{
			return *GetUserPointer<T> (vm, index);
		}
	};

	/* Generic implementation of GetParam. */
	template <typename T>
	inline T GetParam (HSQUIRRELVM vm, int index)
	{
		return GetParamHelper<T>::Get (vm, index);
	}


	/** Helper function to implement Push<T*> for objects. */
	template <typename T>
	void PushObj (HSQUIRRELVM vm, T *res, const char *realname, bool addref = true)
	{
		if (res == NULL) {
			sq_pushnull (vm);
		} else {
			if (addref) res->AddRef();
			Squirrel::CreatePrefixedClassInstance (vm, realname,
					res, DefSQDestructorCallback<T>);
		}
	}

} // namespace SQConvert

#endif /* SQUIRREL_CONVERT_HPP */
