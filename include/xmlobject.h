

#ifndef XMLOBJECT_H
#define XMLOBJECT_H

#include "../include/macros.h"

struct ParseResult;

enum TAG_TYPE
{
	TT_INT,
	TT_STRING,
	TT_BOOL,
	TT_USER
};

class XMLTag
{
public:
	char*		name;
	TAG_TYPE	type;
	void*		value;
	XMLTag*		pNext;
	int			mandatory;
	int			set;

	XMLTag()
	{
		value = NULL;
		pNext = NULL;
	}

	~XMLTag()
	{
		SAFE_DELETE_ARRAY(name);	
	}
};

class Configuration;
// objekt ktery lze nacitat (mozna i ukladat) z xml
class XMLObject
{
protected:
	XMLTag		*tagList, *lastTag;
	char		*contextTag;
	XMLObject	*pParent;

public:
	void		SetContext(const char* contextTag);
	char*		GetContext() { return contextTag; }
	void		DefineTag(char* name, TAG_TYPE type, void* variable, int mandatory = 0);
	void*		GetValue(char* tagName);
	char*		GetStringValue(char* tagName);
	int			GetIntValue(char* tagName);
	void		SetParent(XMLObject* parent) { pParent = parent; }
	XMLObject*	GetParent() { return pParent; }

	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);	
	virtual void	PostParse(Configuration *conf);	// tato metoda bude volana po uspesnem parsingu

	XMLObject(void);
	virtual ~XMLObject(void);
};

#endif
