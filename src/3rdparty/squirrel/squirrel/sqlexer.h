/*	see copyright notice in squirrel.h */
#ifndef _SQLEXER_H_
#define _SQLEXER_H_

struct SQLexer
{
	~SQLexer();
	SQLexer(SQSharedState *ss,SQLEXREADFUNC rg,SQUserPointer up,CompilerErrorFunc efunc,void *ed);
	NORETURN void Error(const char *err);
	SQInteger Lex();
	const char *Tok2Str(SQInteger tok);
private:
	SQInteger GetIDType(char *s);
	SQInteger ReadString(WChar ndelim,bool verbatim);
	SQInteger ReadNumber();
	void LexBlockComment();
	SQInteger ReadID();
	void Next();
	SQInteger _curtoken;
	SQTable *_keywords;
	void INIT_TEMP_STRING() { _longstr.resize(0); }
	void APPEND_CHAR(WChar c);
	void TERMINATE_BUFFER() { _longstr.push_back('\0'); }

public:
	SQInteger _prevtoken;
	SQInteger _currentline;
	SQInteger _lasttokenline;
	SQInteger _currentcolumn;
	const char *_svalue;
	SQInteger _nvalue;
	SQFloat _fvalue;
	SQLEXREADFUNC _readf;
	SQUserPointer _up;
	WChar _currdata;
	SQSharedState *_sharedstate;
	sqvector<char> _longstr;
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};

#endif
