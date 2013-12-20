/*
 *
 *	Implementaion of the load parser
 *
 *	Parses uses XMLParser. It parses file defined in config file
 *	to recieve load values to the particulary lines.
 *
 *	Jiri Formanek - 3.9. 2004
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include "../include/macros.h"
#include "../include/error.h"
#include "../include/xmlparse.h"
#include "../include/xmlobject.h"
#include "../include/network.h"
#include "../include/graph.h"
#include "../include/router.h"
#include "../include/line.h"

/* XML parser handlers */
void	startElementHandlerLL(void *userData, const char *name, const char **atts);
void	endElementHandlerLL(void *userData, const char *name);
void	dataHandlerLL(void *userData, const char *s, int len);

int	ParseLineLoads(char	*chLoadFile, Configuration* conf)
{
	char *chBuffer; /* config file in buffer */
	char	szMessage[100];	
	FILE	*pLoadFile;		
	size_t readen;
	unsigned long iCount;
	
	/* Open config file from cmd line determined by net_data_file parameter or default */
	pLoadFile = fopen(chLoadFile, "rb");
	if(pLoadFile == NULL) {
		sprintf(szMessage, "Cannot open file with loads values: '%s'. \n", chLoadFile);
		SetMyLastError(szMessage);
		return FAILURE;
	}
	
	/* get length of file */
	if(fseek(pLoadFile, 0L, SEEK_END) != 0)
	{
		SetMyLastError("Error during: fseek(pLoadFile, 0L, SEEK_END)\n");
		return FAILURE;
	}
	/* Load file to buffer */
	iCount = ftell(pLoadFile);
	if(iCount == 1L)
	{
		SetMyLastError("Error during: ftell(pLoadFile);\n");
		return FAILURE;
	}
	if(fseek(pLoadFile, 0L, SEEK_SET) != 0)	
	{
		SetMyLastError("Error during: fseek(pLoadFile, 0L, SEEK_SET)\n");
		return FAILURE;
	}
	/* Allocate buffer for file */    
	chBuffer = (char*) malloc(iCount * sizeof(char));
	if(!chBuffer)
	{
		SetMyLastError("Error during: (char*) malloc(iCount * sizeof(char));\n");
		return FAILURE;
	}
	/* read whole file to buffer */	
	readen = fread(chBuffer, sizeof(char), iCount, pLoadFile);	
	
	/* Close the file */
	if(fclose(pLoadFile) != 0)
	{
		SAFE_DELETE(chBuffer);	
		SetMyLastError("Cannot close config file.\n");
		return FAILURE;
	}
	if(readen != (size_t)iCount) 
	{
		SAFE_DELETE(chBuffer);	
		SetMyLastError("Cannot read whole config file into memory.\n");
		return FAILURE;
	}

	/* allocate temporary object of the line */
	conf->np.lastParent = new Line;
	memset(conf->np.lastParent, 0, sizeof(Line));
	
	/* Create parser */	
	conf->np.theParser = XML_ParserCreate(XML_CODING);
	/* Set handlers functions */
	XML_SetElementHandler(conf->np.theParser, startElementHandlerLL, endElementHandlerLL);
	XML_SetCharacterDataHandler(conf->np.theParser, dataHandlerLL);
	/* Pass state data to parser */
	XML_SetUserData(conf->np.theParser, conf);
		
	if(XML_Parse(conf->np.theParser, chBuffer, iCount, 1) == 0) {
		sprintf(szMessage, "Parse error in the load file on line: %d\nError: %s, Code: %d\n", XML_GetCurrentLineNumber(conf->np.theParser), XML_ErrorString(XML_GetErrorCode(conf->np.theParser)), XML_GetErrorCode(conf->np.theParser));
		SetMyLastError(szMessage);
		free(chBuffer);	
		/* free temporary object of the line */
		SAFE_DELETE(conf->np.lastParent);		
		return FAILURE;
	}
	/* free temporary object of the line */
	SAFE_DELETE(conf->np.lastParent);	
	/* Release  file buffer */
	SAFE_DELETE(chBuffer);	
	/* Release XML parser */
	XML_ParserFree(conf->np.theParser);	
	return OK;
}

/* 
 *	Parser handlers
 *	- for start tags (and attributes)
 *	- for end tags
 *	- for data between tags
 */
void	startElementHandlerLL(void *userData, const char *name, const char **atts)
{
	Configuration* conf = (Configuration* ) userData;
	size_t iLen = 0;
	char *szID1, *szID2;
	/* 
	 *	LINE_LOAD TAG
	 */
	if(!strcmp(name, XML_LINELOAD))
	{	
		/* get servers ID from the line params */
		if(atts[1])
		{
			/* vytahnuti dvou ID routeru, ke kterym je linka pripojena,
				tahle informace je ulozena jako atribut tagu linky <line connection="ID1 ID2"> */
			iLen = strlen(atts[1]);
			szID1 = new char[(iLen + 1)];
			szID2 = new char[(iLen + 1)];

			/* release previous allocated memory */
			SAFE_DELETE_ARRAY(((Line*)conf->np.lastParent)->szID1);
			SAFE_DELETE_ARRAY(((Line*)conf->np.lastParent)->szID2);

			ExtractString(0, atts[1], szID1, ' ');
			/* nejprve je treba dostat jednotlive ID z atributu a pak teprve alokovat
				misto pro string v objektu linky */
			((Line*)conf->np.lastParent)->szID1 = new char[strlen(szID1) + 1];
			strcpy(((Line*)conf->np.lastParent)->szID1, szID1);
			delete []szID1;

			ExtractString(1, atts[1], szID2, ' ');
			((Line*)conf->np.lastParent)->szID2 = new char[strlen(szID2) + 1];
			strcpy(((Line*)conf->np.lastParent)->szID2, szID2);
			delete []szID2;
		}
		else
		{
			printf("  Warning: Tag <lineload param=\"ID1 ID2\"> is invalid definition. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
			conf->np.iWarnings++;
		}

	}
	/* 
	 *	ROUTER_LOAD TAG
	 */
	if(!strcmp(name, XML_ROUTERLOAD))
	{	
		/* get servers ID from the line params */
		if(atts[1])
		{
			if(((Line*)conf->np.lastParent)->szID1) 
			{
				free(((Line*)conf->np.lastParent)->szID1);
				((Line*)conf->np.lastParent)->szID1 = NULL;
			}
			/* vytahnuti dvou ID routeru, ke kterym je linka pripojena,
				tahle informace je ulozena jako atribut tagu routeru */
			iLen = strlen(atts[1]);			
			/* nejprve je treba dostat jednotlive ID z atributu a pak teprve alokovat
				misto pro string v objektu linky */			
			((Line*)conf->np.lastParent)->szID1 = new char[(iLen + 1)];
			strncpy(((Line*)conf->np.lastParent)->szID1, atts[1], iLen);
			((Line*)conf->np.lastParent)->szID1[iLen] = 0;
		}
		else
		{
			printf("  Warning: Tag <routerload param=\"ROUTER_ID\"> is invalid definition. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
			conf->np.iWarnings++;
		}

	}
	/* set next tag to NULL*/
	strcpy(conf->np.chNextTag, name);
}

void	endElementHandlerLL(void *userData, const char *name)
{
	Configuration* conf = (Configuration* ) userData;
	/* set next tag to NULL*/
	conf->np.chNextTag[0] = 0;	
}

void	dataHandlerLL(void *userData, const char *s, int len)
{
	Configuration* conf = (Configuration* ) userData;
	char szType[100];
	Line *pLine;	
	Router *pRouter;
	/* load line */
	/*
	 *	<lineload param="ID1 ID2">
	 *		<load>...</load>
	 *	 
	 */
	if(!strcmp(conf->np.chNextTag, XML_LOAD))
	{
		strncpy(szType, s, len);
		szType[len] = 0;
		if(((Line*)conf->np.lastParent))
		{
			/* find line with specific IDs and assign the load value */			
			pLine = conf->np.pLines;
			while(pLine)
			{
				if((!strcmp(pLine->szID1, ((Line*)conf->np.lastParent)->szID1)
					&& !strcmp(pLine->szID2, ((Line*)conf->np.lastParent)->szID2)) ||
					(!strcmp(pLine->szID2, ((Line*)conf->np.lastParent)->szID1)
					&& !strcmp(pLine->szID1, ((Line*)conf->np.lastParent)->szID2)))
				{
					pLine->iLoad = atoi(szType);	
					/* max value is 100 percent */
					pLine->iLoad %= 101;
					break;
				}				
				/* move to next line */
				pLine = pLine->pNext;
			}				
		}
		return;
	}	
	/*
	 *	<rotuerload param="ID1">
	 *		<apload>...</apload>
	 *	 
	 */
	if(!strcmp(conf->np.chNextTag, XML_APLOAD))
	{
		strncpy(szType, s, len);
		szType[len] = 0;
		if(((Line*)conf->np.lastParent))
		{
			/* find line with specific IDs and assign the load value */			
			pRouter = conf->np.pRouters;
			while(pRouter)
			{
				if(!strcmp(pRouter->szID, ((Line*)conf->np.lastParent)->szID1))
				{
					pRouter->iAPLoad = atoi(szType);	
					/* max value is 100 percent */
					pRouter->iAPLoad %= 101;
					break;
				}				
				/* move to next line */
				pRouter = pRouter->pNext;
			}				
		}
		return;
	}	
	/*
	 *	<rotuerload param="ID1">
	 *		<ap2load>...</ap2load>
	 *	 
	 */
	if(!strcmp(conf->np.chNextTag, XML_AP2LOAD))
	{
		strncpy(szType, s, len);
		szType[len] = 0;
		if(((Line*)conf->np.lastParent))
		{
			/* find line with specific IDs and assign the load value */			
			pRouter = conf->np.pRouters;
			while(pRouter)
			{
				if(!strcmp(pRouter->szID, ((Line*)conf->np.lastParent)->szID1))
				{
					pRouter->iAP2Load = atoi(szType);	
					/* max value is 100 percent */
					pRouter->iAP2Load %= 101;
					break;
				}				
				/* move to next line */
				pRouter = pRouter->pNext;
			}				
		}
		return;
	}	
	/*
	 *	<rotuerload param="ID1">
	 *		<lanload>...</lanload>
	 *	 
	 */
	if(!strcmp(conf->np.chNextTag, XML_LANLOAD))
	{
		strncpy(szType, s, len);
		szType[len] = 0;
		if(((Line*)conf->np.lastParent))
		{
			/* find line with specific IDs and assign the load value */			
			pRouter = conf->np.pRouters;
			while(pRouter)
			{
				if(!strcmp(pRouter->szID, ((Line*)conf->np.lastParent)->szID1))
				{
					pRouter->iLANLoad = atoi(szType);	
					/* max value is 100 percent */
					pRouter->iLANLoad %= 101;
					break;
				}				
				/* move to next line */
				pRouter = pRouter->pNext;
			}				
		}
		return;
	}	
}

