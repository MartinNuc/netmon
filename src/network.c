/* 
 * Implementation file for network parsing  
 */

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
#include "../include/xmlparse.h"
#include "../include/xmlobject.h"
#include "../include/network.h"
#include "../include/graph.h"
#include "../include/line.h"
#include "../include/router.h"
#include "../include/options.h"
#include "../include/cloud.h"


int	ParseNetwork(char	*chConfigFile, Configuration* conf)
{	
	char *chBuffer; /* config file in buffer */
	char	szMessage[100];	
	FILE	*pCfgFile;
	Line *pLine;
	int i;
	size_t readen;
	unsigned long iCount;
	
	/* Open config file from cmd line determined by net_data_file parameter or default */
	pCfgFile = fopen(chConfigFile, "rb");
	if(pCfgFile == NULL) {
		sprintf(szMessage, "Cannot open configuration file '%s'. \n", chConfigFile);
		SetMyLastError(szMessage);
		return FAILURE;
	}
	
	/* get length of file */
	if(fseek(pCfgFile, 0L, SEEK_END) != 0)
	{
		SetMyLastError("Error during: fseek(pCfgFile, 0L, SEEK_END)\n");
		return FAILURE;
	}
	/* Load file to buffer */
	iCount = ftell(pCfgFile);
	if(iCount == 1L)
	{
		SetMyLastError("Error during: ftell(pCfgFile);\n");
		return FAILURE;
	}
	if(fseek(pCfgFile, 0L, SEEK_SET) != 0)	
	{
		SetMyLastError("Error during: fseek(pCfgFile, 0L, SEEK_SET)\n");
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
	readen = fread(chBuffer, sizeof(char), iCount, pCfgFile);	
	
	/* Close the file */
	if(fclose(pCfgFile) != 0)
	{
		free(chBuffer);	
		SetMyLastError("Cannot close config file.\n");
		return FAILURE;
	}
	if(readen != (size_t)iCount) 
	{
		free(chBuffer);	
		SetMyLastError("Cannot read whole config file into memory.\n");
		return FAILURE;
	}

	
	/* Create parser */	
	conf->np.theParser = XML_ParserCreate(XML_CODING);
	/* Set handlers functions */
	XML_SetElementHandler(conf->np.theParser, startElementHandler, endElementHandler);
	XML_SetCharacterDataHandler(conf->np.theParser, dataHandler);
	/* Pass state data to parser */
	XML_SetUserData(conf->np.theParser, conf);
		
	if(XML_Parse(conf->np.theParser, chBuffer, iCount, 1) == 0) {
		sprintf(szMessage, "Parse error in the config file on line: %d\nError: %s, Code: %d\n", XML_GetCurrentLineNumber(conf->np.theParser), XML_ErrorString(XML_GetErrorCode(conf->np.theParser)), XML_GetErrorCode(conf->np.theParser));
		SetMyLastError(szMessage);
		free(chBuffer);	
		return FAILURE;
	}
	/* Release  file buffer */
	free(chBuffer);	
	/* Release XML parser */
	XML_ParserFree(conf->np.theParser);
	
	/* pokud nastaly fatalni chyby, ukonci okamzite parsing */
	if(conf->np.iErrors > 0)
	{
		return FAILURE;
	}
	/* allocate space for the router's line pointers and init them */
	/* go through all routers */
	pLine = conf->np.pLines;
	/* find space for the new router */
	while(pLine)
	{
		/* init lines pointers for each router only once */
		if(!pLine->pNode1->parLines && pLine->pNode1->nLines > 0)
		{
			pLine->pNode1->parLines = new Line*[pLine->pNode1->nLines];
			/* prepares pointers */
			for(i = 0; i < pLine->pNode1->nLines; i++)
			{
				pLine->pNode1->parLines[i] = NULL;
			}
		}
		if(!pLine->pNode2->parLines && pLine->pNode2->nLines > 0)
		{
			pLine->pNode2->parLines = new Line*[pLine->pNode2->nLines];
			/* prepares pointers */
			for(i = 0; i < pLine->pNode2->nLines; i++)
			{
				pLine->pNode2->parLines[i] = NULL;
			}
		}
		/* attach line to both routers */
		for(i = 0; i < pLine->pNode1->nLines; i++)
		{
			if(!pLine->pNode1->parLines[i])
			{
				pLine->pNode1->parLines[i] = pLine;
				break;
			}
		}	
		for(i = 0; i < pLine->pNode2->nLines; i++)
		{
			if(!pLine->pNode2->parLines[i])
			{
				pLine->pNode2->parLines[i] = pLine;
				break;
			}
		}	
		pLine = pLine->pNext;
	}
	return OK;
}

/* 
 *	Parser handlers
 *	- for start tags (and attributes)
 *	- for end tags
 *	- for data between tags
 */
void	startElementHandler(void *userData, const char *name, const char **atts)
{
	/* init */
	Configuration* conf = (Configuration* ) userData;
	XMLObject *pObject;


	RouterType *pRTNew = NULL;	
	LineType *pLTNew = NULL;	
	Interface *pNewInt = NULL;
	Interface *pInt;
	Router *pRouter;
	char	* szID1, *szID2;
	char	szTemp[30];
	size_t iLen;
	int c;

	/* 
	 *	ROUTER TAG
	 */
	if(!strcmp(name, XML_ROUTER))
	{
		conf->np.iRoutersCount++;		
		/**************************************************/
		/* create router */
		pObject = new Router;
		/* init atributes */
		if(atts[1] && !(conf->np.lastParent && !strcmp( ((Router*)conf->np.lastParent)->szID, atts[1] )))
		{
			iLen = strlen(atts[1]);
			((Router*)pObject)->szID = new char[iLen+1];
			if(((Router*)pObject)->szID)
			{
				strncpy(((Router*)pObject)->szID, atts[1], iLen);
				((Router*)pObject)->szID[iLen] = 0;
			}
		}	
		else
		{
			printf("  Fatal error: The router ID is not defined with attribute 'id'. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
			conf->np.iErrors++;
			return;
		}
		/**************************************************/
		/* najde hlavni router a nastavi ho podle jeho ID ktery je bud z command line nebo pres konf. soubor */
		if(conf->np.szMainRouterID && !strcmp(conf->np.szMainRouterID, ((Router*)pObject)->szID))
		{
			conf->np.pMainRouter = ((Router*)pObject);
		}
			
		/**************************************************/
		/* find end of list of routers */
		if(!conf->np.pRouters)
		{
			conf->np.pRouters = (Router*)pObject;					
		}
		else
		{
			((Router*)conf->np.lastSibling)->pNext = (Router*)pObject;			
		}		
		conf->np.lastParent = conf->np.lastSibling = pObject;	
	}

	// CLOUD
	if(!strcmp(name, XML_CLOUD))
	{				
		pObject = new Cloud;
		// init atributes 
		if(atts[1])
		{
			int i = 0;
			ExtractString(i++, atts[1], szTemp, ' ');
			do
			{
				pRouter = conf->np.pRouters;
				while(pRouter)
				{
					if(!strcmp(pRouter->szID, szTemp))
					{
						((Cloud*)pObject)->AddRouter(pRouter);
						break;
					}
					pRouter = pRouter->pNext;
				}

				ExtractString(i++, atts[1], szTemp, ' ');	

			} while(strlen(szTemp) != 0);			
		}			
	
		// Pripoji novy cloud na konec seznamu
		if(!conf->np.pClouds)
		{
			conf->np.pClouds = (Cloud*)pObject;					
		}
		else
		{
			((Cloud*)conf->np.lastSibling)->pNext = (Cloud*)pObject;			
		}		
		// Nastavi novy cloud jako posledni v seznamu
		conf->np.lastParent = conf->np.lastSibling = pObject;	
		conf->np.iCloudsCount++;
	}

	/* 
	 * LINE TAG
	 */
	if(!strcmp(name, XML_LINE))
	{		
		/**************************************************/
		/* create new router */
		pObject = new Line;
		/* init atributes */
		if(atts[1])
		{
			/* vytahnuti dvou ID routeru, ke kterym je linka pripojena,
				tahle informace je ulozena jako atribut tagu linky <line connection="ID1 ID2"> */
			iLen = strlen(atts[1]);
			szID1 = new char[iLen + 1];
			szID2 = new char[iLen + 1];
			ExtractString(0, atts[1], szID1, ' ');
			/* nejprve je treba dostat jednotlive ID z atributu a pak teprve alokovat
				misto pro string v objektu linky */
			ExtractString(0, szID1, szTemp, ':');
			((Line*)pObject)->szID1 = new char[strlen(szTemp) + 1];
			strcpy(((Line*)pObject)->szID1, szTemp);

			/* pokud je zapsan interface vytahne se i interface */
			ExtractString(1, szID1, szTemp, ':');
			if(strlen(szTemp) > 0)
			{
				((Line*)pObject)->szInt1 = new char[strlen(szTemp) + 1];
				strcpy(((Line*)pObject)->szInt1, szTemp);
			}

			free(szID1);

			ExtractString(1, atts[1], szID2, ' ');

			ExtractString(0, szID2, szTemp, ':');
			((Line*)pObject)->szID2 = new char[strlen(szTemp) + 1];
			strcpy(((Line*)pObject)->szID2, szTemp);

			/* pokud je zapsan interface vytahne se i interface */
			ExtractString(1, szID2, szTemp, ':');
			if(strlen(szTemp) > 0)
			{
				((Line*)pObject)->szInt2 = new char[strlen(szTemp) + 1];
				strcpy(((Line*)pObject)->szInt2, szTemp);
			}

			free(szID2);
		}
		else
		{
			printf("  Fatal error: The line connection is not defined with 'connection' attribute. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
			conf->np.iErrors++;
			return;
		}
		/* find the line's routers and attach them to the line */
		/* kazda linka ma dva ukazetele na routery do kterych vede, 
		   zde se tyto ukazatele zinicializuji - to jde protoze jsou 
		   vzdy jen dva. U routeru se inkrementuje pocet linek
		   pripojenych na dany router - zde nelze priradit ukazatel
		   protoze jeste nevime kolik linek dany router ma - 
		   pole ukazatelu nelze vytvorit */
		Router* pRouter = conf->np.pRouters;
		while(pRouter)
		{		
			c = 0;
			if(!strcmp(pRouter->szID, ((Line*)pObject)->szID1))
			{
				c = 1; // router je pripojen na ID1 teto linky
			}
			else
			{
				if(!strcmp(pRouter->szID, ((Line*)pObject)->szID2))
				{
					c = 2; // router je pripojen na ID2 teto linky
				}
				else
				{
					pRouter = pRouter->pNext; // router nema s touto linkou nic co delat, jde se na dalsi
					continue;
				}
			}
			
			/* attach the routers */
			// tady se zinicializuje jeden z ukazatelu na linky na router - nastavi se ten, ktery je NULL
			if(!((Line*)pObject)->pNode1)
			{
				((Line*)pObject)->pNode1 = pRouter;
				/* add new line to the router */
				pRouter->nLines++;
			}
			else
			{
				if(!((Line*)pObject)->pNode2)
				{
					((Line*)pObject)->pNode2 = pRouter;
					/* add new line to the router */
					pRouter->nLines++;					
				}					
			}	

			// projdou se vsechny interfacy danyho routeru a hleda se ten, ktery nalezi teto lince
			pInt = pRouter->pInterfaces;
			while(pInt)
			{
				// router je pripojeny na ID1
				if(c == 1)
				{
					// Linka je pripojena na nulovy interface, ale router takovy nema definovany
					if(!((Line*)pObject)->szInt1 && !pRouter->iHasDefault)
					{
						printf("  Warning: Line %s<->%s is connected to default interface which is \n\t   not defined in router %s. First defined will be used.\n", ((Line*)pObject)->szID1, ((Line*)pObject)->szID2, pRouter->szName);			
						conf->np.iWarnings++;
						// je to jenom warning, pak se pouzije prvni definovany interface
						// ten urcite existuje, protoze kdyby ne, vytvoril by se default a ten neni
						c *= -1;
						break;
					}
					// default
					if(!pInt->szName && !((Line*)pObject)->szInt1)
					{
						c *= -1;
						break;
					}
					// standard - interface defined
					if(pInt->szName && ((Line*)pObject)->szInt1)
					{
						if(!strcmp(pInt->szName, ((Line*)pObject)->szInt1))
						{
							c *= -1;
							break;
						}
					}
				}
				// router je pripojeny na ID2
				if(c == 2)
				{
					// Linka je pripojena na nulovy interface, ale router takovy nema definovany
					if(!((Line*)pObject)->szInt2 && !pRouter->iHasDefault)
					{
						printf("  Warning: Line %s<->%s is connected to default interface which is \n\t   not defined in router %s. First defined will be used.\n", ((Line*)pObject)->szID1, ((Line*)pObject)->szID2, pRouter->szName);							
						conf->np.iWarnings++;
						// je to jenom warning, pak se pouzije prvni definovany interface
						// ten urcite existuje, protoze kdyby ne, vytvoril by se default a ten neni
						c *= -1;
						break;
					}
					// default
					if(!pInt->szName && !((Line*)pObject)->szInt2)
					{
						c *= -1;
						break;
					}
					// standard - interface defined
					if(pInt->szName && ((Line*)pObject)->szInt2)
					{
						if(!strcmp(pInt->szName, ((Line*)pObject)->szInt2))
						{
							c *= -1;
							break;
						}
					}
				}
				pInt = pInt->pNext;
			}
			// pokud c zustalo kladny, znamena to, ze linka pouziva nedefinovany interface
			if(c == 1)
			{
				printf("  Error: Router %s hasn't used interface '%s'.\n", pRouter->szID, ((Line*)pObject)->szInt1);
				conf->np.iErrors++;   
			}
			if(c == 2)
			{
				printf("  Error: Router %s hasn't used interface '%s'.\n", pRouter->szID, ((Line*)pObject)->szInt2);
				conf->np.iErrors++;   
			}

			// Pokud jsou oba ukazatele nastaveny, linka je ok a muzem pokracovat
			if(((Line*)pObject)->pNode1 && ((Line*)pObject)->pNode2)
				break;			
			
			pRouter = pRouter->pNext;
		}
		/* pokud se dany router nenajde, je to chyba */
		/* pokud je ukazatel na pNode1 NULL znamena to, ze ani jeden
		   router ktery je definovan na lince neexistuje */
		if(!((Line*)pObject)->pNode1)
		{
			printf("  Error: Devices %s and %s is not defined in the config file. Line: %d\n", ((Line*)pObject)->szID1, ((Line*)pObject)->szID2, XML_GetCurrentLineNumber(conf->np.theParser));
			conf->np.iErrors++;            			
		}
		else
		{
			if(!((Line*)pObject)->pNode2)
			{
				if(!strcmp(((Line*)pObject)->pNode1->szID, ((Line*)pObject)->szID1))
				{
					printf("  Error: Device %s is not defined in the config file. Line: %d\n", ((Line*)pObject)->szID2, XML_GetCurrentLineNumber(conf->np.theParser));
				}
				else
				{
					printf("  Error: Device %s is not defined in the config file. Line: %d\n", ((Line*)pObject)->szID1, XML_GetCurrentLineNumber(conf->np.theParser));
				}				
				conf->np.iErrors++;            			
			}
		}
		/**************************************************/		
		/* find new pos */			
		if(!conf->np.pLines)
		{
			conf->np.pLines = ((Line*)pObject);			
		}
		else
		{
			((Line*)conf->np.lastSibling)->pNext = (Line*)pObject;			
		}
		conf->np.lastParent = conf->np.lastSibling = pObject;
		
		conf->np.iLinesCount++;		
	}

	/* INTERFACE */
	if(!strcmp(name, XML_INTERFACE))
	{
		// Pokud prijde tag Interface, musi byt predchazejici objekt router
		Router *lastRouter = (Router*)conf->np.lastParent;		
		if(lastRouter)
		{
			pNewInt = new Interface;
		
			/* prvni rozhrani jeste neni vytvoreno */
			if(!lastRouter->pInterfaces)
			{
				lastRouter->pInterfaces = pNewInt;
			}
			else
			{
				/* jinak novy pripojim na konec seznamu */
				lastRouter->pLastInt->pNext = pNewInt;
			}
			/* zapamatuju si posledni pro tento router,
			   rozhrani jsou za sebou takze se nemuzou poplest */			
			conf->np.lastSibling = (XMLObject*)pNewInt;
			lastRouter->pLastInt = pNewInt;
		}
	}

	

	/*
	 * ROUTER TYPE TAG 
	 */
	if(!strcmp(name, XML_RT))
	{
		/* create new type */
		pRTNew = new RouterType;
		

		/* find new pos */
		if(!conf->np.pRouterTypes)
		{
			conf->np.pRouterTypes = pRTNew;			
		}
		else
		{
			((RouterType*)conf->np.lastSibling)->pNext = pRTNew;			
		}
		conf->np.lastSibling = pRTNew;
		conf->np.iRTCount++;		
	}
	/*
	 * LINE TYPE TAG 
	 */
	if(!strcmp(name, XML_LT))
	{
		/* create new type */
		pLTNew =  new LineType;
		pLTNew->pNext = NULL;
		pLTNew->id = -1;
		pLTNew->width  = -1;
		pLTNew->desc  = NULL;
		
		/* find new pos */
		if(!conf->np.pLineTypes)
		{
			conf->np.pLineTypes = pLTNew;			
		}
		else
		{
			((LineType*)conf->np.lastSibling)->pNext = pLTNew;			
		}
		conf->np.lastSibling = pLTNew;
		conf->np.iLTCount++;		
	}
	
	/* set next tag - ulozeni kontextu */
	strcpy(conf->np.chNextTag, name);

	if(!strcmp(name, XML_ROUTER))
	{
		conf->np.iParserContext |= PC_ROUTER;
		conf->np.lastSibling->SetContext(name);
	}
	if(!strcmp(name, XML_ROUTER_TYPE))
	{
		conf->np.iParserContext |= PC_ROUTER_TYPE;
		conf->np.lastSibling->SetContext(name);
	}
	if(!strcmp(name, XML_INTERFACE))
	{
		conf->np.iParserContext |= PC_INTERFACE;
		conf->np.lastSibling->SetContext(name);
	}
	if(!strcmp(name, XML_OPTIONS))
	{
		conf->np.iParserContext |= PC_OPTIONS;
		conf->np.lastSibling = conf;
		conf->np.lastSibling->SetContext(name);	
	}
	if(!strcmp(name, XML_LINE_TYPE))
	{
		conf->np.iParserContext |= PC_LINE_TYPE;
		conf->np.lastSibling->SetContext(name);
	}
	if(!strcmp(name, XML_LINE))
	{
		conf->np.iParserContext |= PC_LINE;
		conf->np.lastSibling->SetContext(name);
	}
	if(!strcmp(name, XML_CLOUD))
	{
		conf->np.iParserContext |= PC_CLOUD;
		conf->np.lastSibling->SetContext(name);
	}
	if(!strcmp(name, XML_PATHS))
	{
		conf->np.iParserContext |= PC_PATHS;
		conf->np.lastSibling = conf;
		conf->np.lastSibling->SetContext(name);		
	}
}

void	endElementHandler(void *userData, const char *name)
{
	Configuration* conf = (Configuration* ) userData;
	/* set next tag to NULL*/
	conf->np.chNextTag[0] = 0;	

	// Pokud je zpracovan cely objekt, vola se metoda PostParse()
	if(conf->np.lastSibling && !strcmp(name, conf->np.lastSibling->GetContext()))
	{
		conf->np.lastSibling->PostParse(conf);
	}

	// Vynoreni z vnorenyho objektu Interfaces
	// Musi se vratit ukazatel na posledniho sourozence
	if(!strcmp(name, XML_INTERFACES))
	{
		conf->np.lastSibling = conf->np.lastParent;
	}

	// Konec vsech routeru, linek a typu
	if(!strcmp(name, XML_CLOUDS) || !strcmp(name, XML_ROUTERS) || !strcmp(name, XML_LINES) || !strcmp(name, XML_ROUTER_TYPES) || !strcmp(name, XML_LINE_TYPES)|| !strcmp(name, XML_PATHS)|| !strcmp(name, XML_OPTIONS))
	{
		conf->np.lastSibling = NULL;
	}
}

void	dataHandler(void *userData, const char *s, int len)
{
	Configuration* conf = (Configuration*) userData;
	if(conf->np.lastSibling && strlen(conf->np.chNextTag) > 0)
	{
		conf->np.lastSibling->Parse(conf->np.chNextTag, s, len, conf);
	}
}

int		BreadthFirstSearch(Configuration* conf)
{
	Router ** pQueue, *pAdj;
	Router *pRouter = conf->np.pRouters;
	char inter[20];
	Interface *pInt;
	int bq = 0; int be = 0;
	int i, iRet;
	if(!conf->np.pMainRouter) return FAILURE;	
	
	/* init queue */
	pQueue = new Router*[conf->np.iRoutersCount];
	/* all routers are already fresh */
	while(pRouter)
	{
		pRouter->iDistance = -1;
		pRouter->nState = NS_FRESH;
		pRouter = pRouter->pNext;
	}
	/* open first router - main router */
	conf->np.pMainRouter->nState = NS_OPEN;
	conf->np.pMainRouter->iDistance = 0;
	conf->np.pMainRouter->rState = RS_OK;
	/* enqueue main router */
	pQueue[be++] = conf->np.pMainRouter;
	while(bq < be)
	{
		/* queue first router */
		pRouter = pQueue[bq];		
		/* for each adj */
		for(i = 0; i < pRouter->nLines; i++)
		{
			pAdj = (pRouter == pRouter->parLines[i]->pNode1) ? pRouter->parLines[i]->pNode2 
															 : pRouter->parLines[i]->pNode1;

			inter[0] = 0;
			// rozhrani na ktere se bude pingat se rozlisi podle toho
			// na ktere ze dvou rozhrani je pripojen router ze ktereho se pinguje
			// pokud napriklad je testovaci router pripojen na 1 (tedy int1)
			// bude se pingat z 2 (tedy int2) a naopak
			if(!strcmp(pRouter->parLines[i]->szID1, pRouter->szID))
			{
				if(pRouter->parLines[i]->szInt2)
					strcpy(inter, pRouter->parLines[i]->szInt2);
			}
			else
			{
				if(pRouter->parLines[i]->szInt1)
					strcpy(inter, pRouter->parLines[i]->szInt1);
			}
			
			
			if(pAdj->nState == NS_FRESH && !pAdj->iUnderConst)
			{
				/* open the adj */
				pAdj->nState = NS_OPEN;
				/* add distance */
				pAdj->iDistance = pRouter->iDistance + 1;
				
				if(conf->iVerbose)
				{
					printf("    %s->%s", pRouter->szName, pAdj->szName);
				}
				/* ping adj routers */
				if(!pAdj->prType->bSwitch)
				{	
					iRet = 0;
					pInt = pAdj->pInterfaces;
					while(pInt)
					{
						// neni definovany default interface a linka je na nej napojena
						// cili se pouzije prvni definovany interface
						if(strlen(inter) == 0 && !pAdj->iHasDefault)
						{
							pInt = pAdj->pInterfaces;
							if(conf->iVerbose)
								printf(":%s ", pInt->szName);
							break;
						}

						/* default interface */
						if(pInt->szName == NULL && strlen(inter) == 0)
							break;

						if(pInt->szName && !strcmp(inter, pInt->szName))
						{
							if(conf->iVerbose)
								printf(":%s ", pInt->szName);
							break;	
						}

						pInt = pInt->pNext;
					}
					if(pInt)
						iRet = ping(pInt->szIP, conf->ping.iMaxAttempt, conf->ping.iTimeout);							
				}
				else
				{
					/* switch is always reachable - 1 means 1 ms*/
					iRet = 1;
				}
				if(iRet > 0)
				{
					/* enqueque only OK adj */
					pQueue[be++] = pAdj;
					pAdj->rState = RS_OK;
					if(conf->iVerbose)
					{
						printf("(time=%dms)\n", iRet);
					}	
				}
				else
				{					
					if(iRet == (int)FAILURE)
					{
						/* routers behind the KO routers are in the UNKNOWN state */
						pAdj->rState = RS_KO;						
						if(conf->iVerbose)
						{
							printf("(Router is unreachable)\n");
						}	
					}
					else
					{					
						pAdj->nState = NS_CLOSED;
						pAdj->rState = RS_INVALID;	
						if(conf->iVerbose)
						{
							printf("(Socket error)\n");
						}					
					}
				}				
			}			
		}		
		/* move begin of the queque to next open router */
		bq++;
		/* close current router */
		pRouter->nState = NS_CLOSED;
	}
	if(!conf->iVerbose)
	{
		printf("\n");
	}
	printf("Network checking done.\n\n");
	free(pQueue);
	return OK;
}

Configuration::Configuration()
{
	np.szResHTML = np.szResPNG = np.szSourcePath = np.szLoadURL = NULL;
	gd.szFontPath = gd.szAPPath = gd.szAP2Path = gd.szLANPath = np.szMainRouterID = NULL;
	np.iCloudsCount = np.iRoutersCount = np.iAPsCount = np.iLANsCount = np.iLinesCount = np.iRTCount = np.iLTCount = np.iWarnings = np.iErrors = np.iLineSegments = 0;
	gd.pppRouterMatrix = NULL;
	np.lastSibling = np.lastParent = NULL;
	np.iParserContext = 0;
	np.pLines = NULL;
	np.pRouters = NULL;
	np.pLineTypes = NULL;
	np.pRouterTypes = NULL;
	np.pMainRouter = NULL;
	np.pClouds = NULL;
	np.networkGraph = new Network;

	ping.iMaxAttempt = 3;
	ping.iTimeout = 1000;
	gd.iCellWidth = 110;
	gd.iCellHeight = 70;

	DefineTag(XML_MAXATTEMPTS, TT_INT, &ping.iMaxAttempt);
	DefineTag(XML_TIMEOUT, TT_INT, &ping.iTimeout);
	DefineTag(XML_MAINROUTER, TT_USER, NULL); 
	DefineTag(XML_VERTICAL_CELLS, TT_INT, &gd.iVerticalCells, 1);
	DefineTag(XML_HORIZONTAL_CELLS, TT_INT, &gd.iHorizontalCells, 1);
	DefineTag(XML_CELL_WIDTH, TT_INT, &gd.iCellWidth);
	DefineTag(XML_CELL_HEIGHT, TT_INT, &gd.iCellHeight);

	DefineTag(XML_RESHTML_PATH, TT_STRING, &np.szResHTML);
	DefineTag(XML_RESPNG_PATH, TT_STRING, &np.szResPNG);
	DefineTag(XML_SOURCE_PATH, TT_STRING, &np.szSourcePath);
	DefineTag(XML_LOADURL, TT_STRING, &np.szLoadURL);
	
	DefineTag(XML_APPNG_PATH, TT_STRING, &gd.szAPPath, 1);
	DefineTag(XML_AP2PNG_PATH, TT_STRING, &gd.szAP2Path, 1);
	DefineTag(XML_LANPNG_PATH, TT_STRING, &gd.szLANPath, 1);
	DefineTag(XML_DEFFONT, TT_STRING, &gd.szFontPath, 1);
}

Configuration::~Configuration()
{
	Router * pRouter;
	Router * pNextRouter;
	RouterType *pRT;
	RouterType *pRTNext;
	Line *	pLine;
	Line *	pNextLine;
	Cloud* pCloud, *pNextCloud;
	
	/* delete routers */
	pRouter = np.pRouters;
	while(pRouter)

	{
		/* save pointer to next router */
		pNextRouter = pRouter->pNext;
		/* delete current router */			
		SAFE_DELETE(pRouter);
		/* move to next router */
		pRouter = pNextRouter;
	}		
	np.pRouters = NULL;
	/* delete lines */
	pLine = np.pLines;
	while(pLine)
	{
		/* save pointer to next line */
		pNextLine = pLine->pNext;
		/* delete current line */							
		SAFE_DELETE(pLine);
		/* move to next line */
		pLine = pNextLine;
	}		
	np.pLines = NULL;
	/* delete router types */
	pRT = np.pRouterTypes;
	while(pRT)
	{
		/* save pointer to next router type */
		pRTNext = pRT->pNext;
		/* delete current router type */			
		SAFE_DELETE(pRT);
		/* move to next router type */
		pRT = pRTNext;
	}		
	np.pRouterTypes = NULL;
	// delete clouds
	pCloud = np.pClouds;	
	while(pCloud)
	{
		// save pointer to next router 
		pNextCloud = pCloud->pNext;
		// delete current router 
		SAFE_DELETE(pCloud);
		// move to next router 
		pCloud = pNextCloud;
	}
	np.pClouds = NULL;

	SAFE_DELETE(np.networkGraph);
}

void	Configuration::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLObject::Parse(tagName, s, len, conf);

	XMLTag *tag = tagList;
	
	while(tag)
	{
		if(!strcmp(tagName, tag->name))
		{
			switch(tag->type)
			{
				case TT_USER:
					if(!strcmp(tagName, XML_MAINROUTER))
					{
						if(!conf->np.szMainRouterID)
						{
							conf->np.szMainRouterID = new char[len + 1];
							NormalizeString(conf->np.szMainRouterID, len, (char*)s, len);
						}
					}
					return;
				case TT_BOOL:
				case TT_STRING:
				case TT_INT:
					break;
			}
		}
		tag = tag->pNext;
	}
}
