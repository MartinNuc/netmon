/*
 * Standard headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

/* added libraries */
#include "../include/macros.h"
#include "../include/error.h"
#include "../include/xmlparse.h"
 
/* Own headers */
#include "../include/xmlobject.h"
#include "../include/network.h"


XMLObject::XMLObject(void)
{
	lastTag = tagList = NULL;
	contextTag = NULL;
	pParent = NULL;
}

XMLObject::~XMLObject(void)
{
	SAFE_DELETE_ARRAY(this->contextTag);

	XMLTag *toDelete, *tag = tagList;
	while(tag)
	{
		toDelete = tag;
		tag = tag->pNext;

		if(toDelete->type == TT_STRING)
		{
			char* p = (*(char**)toDelete->value);
			SAFE_DELETE_ARRAY(p);
		}

		SAFE_DELETE(toDelete);
	}
}

void	XMLObject::SetContext(const char* contextTag)
{
	SAFE_DELETE_ARRAY(this->contextTag);

	this->contextTag = new char[strlen(contextTag) + 1];
	strcpy(this->contextTag, contextTag);
}

void	XMLObject::DefineTag(char* name, TAG_TYPE type, void* variable, int mandatory)
{
	XMLTag *newTag = new XMLTag;
	newTag->type = type;
	newTag->pNext = NULL;
	newTag->name = new char[strlen(name) + 1];
	newTag->value = variable;
	newTag->mandatory = mandatory; // je tento tag povinny?
	newTag->set = 0;	// indikuje zdali byla hodnota tagu prectena
	strcpy(newTag->name, name);

	if(!lastTag)
		tagList = newTag;
	else
		lastTag->pNext = newTag;

	lastTag = newTag;
}

void	XMLObject::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLTag *tag = tagList;
	char	number[20];
	char	buffer[MAX_PATH];
	
	while(tag)
	{
		if(!strcmp(tagName, tag->name))
		{
			switch(tag->type)
			{
				case TT_INT:			
					if(len > 20) 
					{
						printf("Error: Number is too long. Max length is 20 digits.\n");
						conf->np.iErrors++;
						return;
					}

					strncpy(number, s, len);
					number[len] = 0;
					(*(int*)tag->value) = atoi(number);
					tag->set = 1;
					return;

				case TT_STRING:			
					strncpy(buffer, s, len);					
					(*(char**)tag->value) = new char[len + 1];
					NormalizeString((*(char**)tag->value), len, buffer, MAX_PATH);
					(*(char**)tag->value)[len] = 0;					
					tag->set = 1;
					return;
				case TT_BOOL:
					strncpy(number, s, len);
					number[len] = 0;
					if(!strcmp(number, S_TRUE))
						(*(int*)tag->value) = 1;
					else
						(*(int*)tag->value) = 0;	
					tag->set = 1;
					return;
				case TT_USER:
					break;
			}			
		}
		tag = tag->pNext;		
	}
}

void	XMLObject::PostParse(Configuration *conf)
{
	// tuhle metodu si kazdej odvozenej objekt muze pretizit sam

	// zkontroluje povinne tagy
	XMLTag *tag = tagList;
	while(tag)
	{
		if(tag->mandatory && !tag->set)
		{
			char t[MAX_PATH];
			strcpy(t, tag->name);
			printf("  Error: The object does not have defined a mandatory property '%s'.\n\t Use tag <%s></%s> to define this property. Line: %d\n", ConvertStringToUppercase(t), tag->name, tag->name, XML_GetCurrentLineNumber(conf->np.theParser));							
			conf->np.iErrors++;
		}		

		tag = tag->pNext;
	}
}

void*	XMLObject::GetValue(char* tagName)
{
	XMLTag *tag = tagList;
	while(tag)
	{
		if(!strcmp(tagName, tag->name))
		{
			return tag->value;			
		}
		tag = tag->pNext;		
	}
	return NULL;
}

char*	XMLObject::GetStringValue(char* tagName)
{
	return *(char**)GetValue(tagName);
}

int	XMLObject::GetIntValue(char* tagName)
{
	return *(int*)GetValue(tagName);
}
