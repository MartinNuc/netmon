
/*
 * Standard headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include "../include/macros.h"
#include "../include/error.h"
#include "../include/xmlobject.h"
#include "../include/network.h"
#include "../include/graph.h"
#include "../include/line.h"
#include "../include/router.h"

LineType::LineType()
{
	id = -1;
	width = 3;
	desc = NULL;
	pNext = NULL;	
	
	DefineTag(XML_LT_ID, TT_INT, &id, 1);
	DefineTag(XML_LT_WIDTH, TT_INT, &width);
	DefineTag(XML_LT_DESC, TT_STRING, &desc);
}

LineType::~LineType()
{
	SAFE_DELETE_ARRAY(desc);
}

Line::Line()
{
	lid = 0;
	pNode1 = pNode2 = NULL;
	szLineURL = szID1 = szID2 = szInt1 = szInt2 = szLineTitle = NULL;	
	iLoad = -1;
	pNext = NULL;
	pLastPath = pPath = NULL;
	bUnderConst = false;
	pParallelLine = NULL;
	bParallel = false;
	
	DefineTag(XML_LTYPE, TT_INT, &lid, 1);
	DefineTag(XML_LINEURL, TT_STRING, &szLineURL);
	DefineTag(XML_LTITLE, TT_STRING, &szLineTitle);
	DefineTag(XML_LUNDERCONST, TT_BOOL, &bUnderConst);
}

Line::~Line()
{		
	SAFE_DELETE_ARRAY(szID1);
	SAFE_DELETE_ARRAY(szID2);
	SAFE_DELETE_ARRAY(szInt1);
	SAFE_DELETE_ARRAY(szInt2);
	SAFE_DELETE_ARRAY(szLineURL);
	SAFE_DELETE_ARRAY(szLineTitle);
}

void	Line::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLTag *tag = tagList;
	int		iInvalidType;
	while(tag)
	{
		if(!strcmp(tagName, tag->name))
		{
			switch(tag->type)
			{
				case TT_INT:			
				case TT_STRING:
				case TT_BOOL:
					XMLObject::Parse(tagName, s, len, conf);
					if(!strcmp(tagName, XML_LTYPE))
					{
						/* find line type in the list */		
						LineType *pLType = conf->np.pLineTypes;
						iInvalidType = 1;
						while(pLType)
						{
							/* jump over the invalid types */
							/* try to find valid type */
							if(pLType->id != -1 && pLType->id == lid)
							{
								/* set AP atribute */				
								iInvalidType = 0;
								pLT = pLType;
								break;
							}
							pLType = pLType->pNext;
						}
						/* set default router type */
						if(iInvalidType)
						{
							if(!conf->np.pLineTypes)
							{
								printf("  Error: Type of the line '%s<->%s' does not exist and default type is invalid. Line: %d\n", szID1, szID2, XML_GetCurrentLineNumber(conf->np.theParser));							
								conf->np.iErrors++;
							}
							else
							{
								printf("  Warning: Type of the line '%s<->%s' does not exist, default is set.\n           Default is type with ID: %d. Line: %d\n", szID1, szID2, conf->np.pRouterTypes->id, XML_GetCurrentLineNumber(conf->np.theParser));				
								lid = conf->np.pLineTypes->id;
								pLT = conf->np.pLineTypes;			
								conf->np.iWarnings++;
							}			
						}						
					}					
					return;		
				case TT_USER:
					break;
			}
		}
		tag = tag->pNext;
	}
}

void	Line::PostParse(Configuration *conf)
{
	Line* pLine = conf->np.pLines;
	while(pLine)
	{
		// Preskocime samotnou linku 
		if(pLine != this)
		{
			// Predek ma stejny IDs jako potomek a nesmi mit nastaven priznak ze je potomek
			if(!strcmp(pLine->szID1, szID1) && !strcmp(pLine->szID2, szID2) && !pLine->bParallel)
			{
				pLine->pParallelLine = this;	// jeji predek
				bParallel = true;				// aktualni linka je potomek
				pParallelLine = pLine;
				break;
			}
		}			
		pLine = pLine->pNext;
	}
	
}


int	Line::SegmentsCount()
{
	int count = 0;
	LineSegment* segment = pPath;
	while(segment)
	{
		// Zjednodusuje linku pokud je prima, nedela ten prdik mezi
		while(segment->pNext)
		{			
			// pokud maji segmenty stejnou normalu, spojuji se
			if(segment->fX == segment->pNext->fX && segment->fY == segment->pNext->fY)
			{					
				segment = segment->pNext;										
			}
			else
			{
				// pokud se cara zlomi, ukonci se zjednodusovani
				break;
			}			
		}
		count++;
		segment = segment->pNext;						
	}		
	return count;
}

LineSegment*	Line::GetSegment(int index, LineSegment *output)
{	
	LineSegment* segment = pPath;
	int i = 0;
	while(segment)
	{
		*output = *segment;
		// Zjednodusuje linku pokud je prima, nedela ten prdik mezi
		while(segment->pNext)
		{			
			// pokud maji segmenty stejnou normalu, spojuji se
			if(segment->fX == segment->pNext->fX && segment->fY == segment->pNext->fY)
			{	
				// porad se nastavuje koncovy bod toho posledniho
				output->pNodes[1] = segment->pNext->pNodes[1];
				// a taky ukazatel na dalsi segment
				output->pNext = segment->pNext->pNext;
				segment = segment->pNext;										
			}
			else
			{
				// pokud se cara zlomi, ukonci se zjednodusovani
				break;
			}			
		}

		// pokud jsme se v poli dostali na index ktery chceme, ukoncime hledani
		if(i == index)
			break;

		i++;		
		segment = segment->pNext;						
	}		
	return output;
}

int	Line::LineState()
{
	int iLineState = -1;
	// get router's together 
	int iState = pNode1->rState | pNode2->rState;
	// both routers ok, line is OK
	if(iState & RS_OK && !(iState & ~RS_OK))
	{
		iLineState = LS_OK;
	}
	// both routers ko, line unknown 
	if(iState & RS_KO && !(iState & ~RS_KO))
	{
		iLineState = LS_UNKNOWN;
	}
	// both routers unknown, line unknown 
	if(iState & RS_UNKNOWN && !(iState & ~RS_UNKNOWN))
	{
		iLineState = LS_UNKNOWN;
	}
	// obe linky jsou invalid, linka je invalid
	if(iState & RS_INVALID && !(iState & ~RS_INVALID))
	{
		iLineState = LS_INVALID;
	}
	// one is ok, second ko, line ko 
	if(iState & RS_OK && iState & RS_KO)
	{
		iLineState = LS_KO;
	}
	// one is invalid, second unknown, line unknown/invalid - tezko rict
	if(iState & RS_INVALID && iState & RS_UNKNOWN)
	{
		iLineState = LS_UNKNOWN;
	}
	// one is ko and second is unknown, line is unknown 
	if(iState & RS_KO && iState & RS_UNKNOWN)
	{
		iLineState = LS_UNKNOWN;
	}	
	// one is ok and second is unknown, line is unknown 
	if(iState & RS_OK && iState & RS_UNKNOWN)
	{
		iLineState = LS_UNKNOWN;
	}
	// Pokud je alespon jeden invalid, cara bude taky invalid		
	if(iState & RS_INVALID && iState & RS_OK)
	{
		iLineState = LS_INVALID;
	}
	if(iState & RS_INVALID && iState & RS_KO)
	{
		iLineState = LS_INVALID;
	}
	// Pokud je cokoliv ve vystavbe, ma to nejvyssi prioritu a stav je MODRA
	if(pNode1->iUnderConst || pNode2->iUnderConst || bUnderConst)
	{
		iLineState = LS_INVALID;
	}

	if(iLineState == -1)
	{
		printf("Error: Line cannot be drawn, netmon is not able to determine right state.\n");		
	}	
	return iLineState;
}

void	Line::BaseColors(int &r, int &g, int &b)
{
	switch(LineState())
	{
	case LS_OK:
		if(iLoad != -1)
		{
			float loadColor = (float)iLoad*1.3f;			
			r = 100;
			g = 255-(int)loadColor;
			b = 50;
		}
		else
		{
			r = 100;
			g = 190;
			b = 50;
		}
		break;
	case LS_KO:		
		r = 220;
		g = 80;
		b = 80;
		break;
	case LS_UNKNOWN:		
		r = 140;
		g = 140;
		b = 140;
		break;
	case LS_INVALID:		
		r = 80;
		g = 170;
		b = 255;
		break;
	default:		
		r = 240;
		g = 240;
		b = 240;			
		break;
	}				
}
