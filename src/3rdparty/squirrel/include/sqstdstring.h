/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_STRING_H_
#define _SQSTD_STRING_H_

typedef unsigned int SQRexBool;
typedef struct SQRex SQRex;

typedef struct {
	const char *begin;
	SQInteger len;
} SQRexMatch;

SQRex *sqstd_rex_compile(const char *pattern,const char **error);
void sqstd_rex_free(SQRex *exp);
SQBool sqstd_rex_match(SQRex* exp,const char* text);
SQBool sqstd_rex_search(SQRex* exp,const char* text, const char** out_begin, const char** out_end);
SQBool sqstd_rex_searchrange(SQRex* exp,const char* text_begin,const char* text_end,const char** out_begin, const char** out_end);
SQInteger sqstd_rex_getsubexpcount(SQRex* exp);
SQBool sqstd_rex_getsubexp(SQRex* exp, SQInteger n, SQRexMatch *subexp);

SQRESULT sqstd_format(HSQUIRRELVM v,SQInteger nformatstringidx,SQInteger *outlen,char **output);

SQRESULT sqstd_register_stringlib(HSQUIRRELVM v);

#endif /*_SQSTD_STRING_H_*/
