
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
#include "../include/router.h"

Router::Router()
{
	// Atributy z xml
	szID = szName = szAPURL = szAP2URL = szRouterURL = szLanURL = szESSID = szESSID2 = szComment = NULL;
	iAP = 0;	// bez antenky
	iEth = 0;	// bez eth segmentu
	iUnderConst = 0;	// v provozu
	rType = -1; // povinny

	// Atributy urcene za behu
	prType = NULL;
	pInterfaces = NULL;
	pNext = NULL;
	nLines = 0;
	parLines = NULL;
	rState = RS_UNKNOWN;
	iX = iRealX = -1;
	iY = iRealY = -1;
	iAPLoad = 0;
	iAP2Load = -1;
	iLANLoad = -1;		
	iHasDefault = 0;

	// atributy pro testovani
	nState  = NS_FRESH;	
	
	// atributy pro vytvareni cest
	pNode	= NULL;

	// povinne tagy
	DefineTag(XML_NAME, TT_STRING, &szName, 1);
	DefineTag(XML_POSITION, TT_USER, NULL, 1);	// pozice je prozatim povinna
	DefineTag(XML_RTYPE, TT_INT, &rType, 1);

	// nepovinne tagy
	DefineTag(XML_IP, TT_USER, NULL);
	DefineTag(XML_ROUTERURL, TT_STRING, &szRouterURL);
	DefineTag(XML_APURL, TT_STRING, &szAPURL);
	DefineTag(XML_AP2URL, TT_STRING, &szAP2URL);
	DefineTag(XML_LANURL, TT_STRING, &szLanURL);
	DefineTag(XML_COMMENT, TT_STRING, &szComment);	
	DefineTag(XML_APTYPE, TT_INT, &iAP);
	DefineTag(XML_ETH, TT_BOOL, &iEth);
	DefineTag(XML_ESSID, TT_STRING, &szESSID);
	DefineTag(XML_ESSID2, TT_STRING, &szESSID2);
	DefineTag(XML_UNDERCONST, TT_BOOL, &iUnderConst);
}

Router::~Router()
{
	SAFE_DELETE_ARRAY(szName);
	SAFE_DELETE_ARRAY(szComment);
	SAFE_DELETE_ARRAY(szAPURL);
	SAFE_DELETE_ARRAY(szAP2URL);
	SAFE_DELETE_ARRAY(szRouterURL);
	SAFE_DELETE_ARRAY(szID);
	SAFE_DELETE_ARRAY(parLines);
	SAFE_DELETE_ARRAY(szLanURL);
	SAFE_DELETE_ARRAY(szESSID);
	SAFE_DELETE_ARRAY(szESSID2);

	Interface *pToDelete;
	while(pInterfaces)
	{
		pToDelete = pInterfaces;
		pInterfaces = pInterfaces->pNext;		
		SAFE_DELETE(pToDelete);
	}
}

void	Router::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLTag *tag = tagList;
	int		ip_invalid;
	char	szPos[200];
	char	szType[200];
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
					if(!strcmp(tagName, XML_ETH) && iEth)
					{
						conf->np.iLANsCount++;
					}
					if(!strcmp(tagName, XML_APTYPE))
					{
						conf->np.iAPsCount += iAP;
					}
					return;
				case TT_USER:
					if(!strcmp(tagName, XML_IP))
					{
						ip_invalid = 0;
						/* IP adr is too long */
						if(len > MAX_IP)
						{
							ip_invalid = 1;
						}
						else
						{
							strncpy(szPos, s, len);		
							szPos[len] = 0;
							/* IP adr is totally corrupted */
							for(int i = 0; i < 4; i++)
							{
								ExtractString(i, szPos, szType, '.');
								if(!strcmp(szType, ""))
								{
									ip_invalid = 1;
								}
								int ip = atoi(szType);
								if(ip > 0xff || ip < 0)
								{
									ip_invalid = 1;
								}
							}			
						}
						if(!ip_invalid)
						{
							/* pokud se najde platna IP mimo sekci interfaces, vytvori se default interface */
							if(!pInterfaces)
							{
								pInterfaces = new Interface();
								pInterfaces->pNext = NULL;
								pInterfaces->szName = NULL; // default
								strncpy(pInterfaces->szIP, s, len);
								pInterfaces->szIP[len] = 0;
								pLastInt = pInterfaces;
							}
							else
							{
								pLastInt->pNext = new Interface();
								pLastInt->pNext->pNext = NULL;
								pLastInt->pNext->szName = NULL; // default
								strncpy(pLastInt->pNext->szIP, s, len);
								pLastInt->pNext->szIP[len] = 0;
								pLastInt = pLastInt->pNext;
							}		
							iHasDefault = 1;							
							//printf("  Warning: Tag <ip> is obsolete and will be removed in the next\n\t   netmon version, use rather <interfaces> structure. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));							
							//conf->np.iWarnings++;
						}
						else
						{
							printf("  Warning: IP address format is invalid. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
						}	
					}
					if(!strcmp(tagName, XML_POSITION))
					{
						strncpy(szPos, s, len);
						szPos[len] = 0;
						/* use temporarly szType variable */
						ExtractString(0, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router has invalid position format.\n           Should be for example <position>1 6</position>. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						iX = atoi(szType)-1;
						iRealX = (atoi(szType)-1)*conf->gd.iCellWidth+conf->gd.iCellWidth/2;
						ExtractString(1, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router has invalid position format.\n           Should be for example <position>1 6</position>. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						iY = atoi(szType)-1;
						iRealY = (atoi(szType)-1)*conf->gd.iCellHeight+conf->gd.iCellHeight/2+10;	
						tag->set = 1;
					}
					return;
			}
		}
		tag = tag->pNext;		
	}
}

void	Router::PostParse(Configuration *conf)
{
	XMLObject::PostParse(conf);

	/* find router type in the list */	
	RouterType *pRTPre;
	RouterType *pRT = conf->np.pRouterTypes;
	int iInvalidType = 1;
	while(pRT)
	{
		pRTPre = pRT;
		/* jump over the invalid types */
		/* try to find valid type */
		if(pRT->id != -1 && pRT->id == rType)
		{
			/* set AP atribute */				
			iInvalidType = 0;
			prType = pRT;
			break;
		}
		pRT = pRT->pNext;
	}
	/* set default router type */
	if(iInvalidType)
	{
		printf("  Error: Type of the '%s' does not exist and default type is invalid. Line: %d\n", szName, XML_GetCurrentLineNumber(conf->np.theParser));							
		conf->np.iErrors++;			
	}
}

Interface::Interface()
{
	pNext = NULL;
	szName = NULL;

	DefineTag(XML_NAME, TT_STRING, &szName);
	DefineTag(XML_IP, TT_USER, NULL);
}

Interface::~Interface()
{
	
}

void	Interface::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLTag *tag = tagList;
	int		ip_invalid;
	char	szPos[200];
	char	szType[200];
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
					return;
				case TT_USER:
					if(!strcmp(tagName, XML_IP))
					{
						ip_invalid = 0;
						/* IP adr is too long */
						if(len > MAX_IP)
						{
							ip_invalid = 1;
						}
						else
						{
							strncpy(szPos, s, len);		
							szPos[len] = 0;
							/* IP adr is totally corrupted */
							for(int i = 0; i < 4; i++)
							{
								ExtractString(i, szPos, szType, '.');
								if(!strcmp(szType, ""))
								{
									ip_invalid = 1;
								}
								int ip = atoi(szType);
								if(ip > 0xff || ip < 0)
								{
									ip_invalid = 1;
								}
							}			
						}
						if(!ip_invalid)
						{
							strncpy(szIP, s, len);	
							szIP[len] = 0;
						}
						else
						{
							printf("  Warning: IP address format is invalid. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
						}	
					}
					return;
			}
		}
		tag = tag->pNext;		
	}
}

RouterType::RouterType()
{
	pNext = NULL;
	sx = 68;
	sy = 38;
	id = -1;
	r  = 0;
	g  = 0;
	b  = 0;
	info = (RI_NAME|RI_IP);
	labels[0] = 0;
	labels[1] = 0;
	labels[2] = 0;
	labIndex = 0;
	/* set default values */		
	iBorderWidth = 1;
	bb = br = bg = 0; /* black */
	ipb = locb = 200;
	ipr = locr = 200;
	ipg = locg = 255;
	loc_size = DEFAULT_LOC_FONT;
	ip_size = DEFAULT_IP_FONT;
	name_size = DEFAULT_NAME_FONT;
	namr = namg = 255; /* yellow */
	namb = 0;
	bSwitch = 0;
	bShadow = 0;

	// povinne tagy
	DefineTag(XML_RT_ID, TT_INT, &id, 1);

	// nepovinne tagy
	DefineTag(XML_RT_DIM, TT_USER, NULL);
	DefineTag(XML_RT_COLOR, TT_USER, NULL);
	DefineTag(XML_RT_INFO, TT_USER, NULL);
	DefineTag(XML_RT_SWITCH, TT_BOOL, &bSwitch);
	DefineTag(XML_RT_SHADOW, TT_BOOL, &bShadow);
}

RouterType::~RouterType()
{

}

void	RouterType::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLTag *tag = tagList;
	char	szPos[200];
	char	szType[200], szType1[200], *szBuf;
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
					return;
				case TT_USER:
					if(!strcmp(tagName, XML_RT_DIM))
					{
						strncpy(szPos, s, len);
						szPos[len] = 0;
						/* use temporarly szType variable */
						ExtractString(0, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router type has invalid dimension format.\n           Should be for example <dimension>100 60</dimension>. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						sx = atoi(szType);
						ExtractString(1, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router type has invalid dimension format.\n           Should be for example <dimension>100 60</dimension>. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						sy = atoi(szType);	
						return;
					}
					if(!strcmp(conf->np.chNextTag, XML_RT_COLOR))
					{
						strncpy(szPos, s, len);
						szPos[len] = 0;
						/* use temporarly szType variable */
						ExtractString(0, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router type has invalid color format.\n           Should be RGB format (i.e. <color>100 100 125</color>). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						r = (unsigned char)atoi(szType);
						ExtractString(1, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router type has invalid color format.\n           Should be RGB format (i.e. <color>100 100 125</color>). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						g = (unsigned char)atoi(szType);
						ExtractString(2, szPos, szType, ' ');
						if(!strcmp(szType, ""))
						{
							printf("  Warning: Router type has invalid color format.\n           Should be RGB format (i.e. <color>100 100 125</color>). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
							conf->np.iWarnings++;
							return;
						}
						b = (unsigned char)atoi(szType);	
						return;
					}	
					if(!strcmp(conf->np.chNextTag, XML_RT_INFO))
					{	
						int i = 0;
						info = 0;
						strncpy(szPos, s, len);
						szPos[len] = 0;
						/* use temporarly szType variable */
						ExtractString(0, szPos, szType, ' ');
						while(strcmp(szType, ""))
						{
							ExtractString(i++, szPos, szType, ' ');
							/* extract values from the info string */			

							/*
							*	Color and size of the IP adr.
							*
							*/
							if(!strncmp(szType, RTI_IP, 2))
							{
								info |= RI_IP;
								labels[labIndex++] = RI_IP;
								/* jump over ip( */
								szBuf = szType;
								szBuf += 2;				
								/* detect color */
								if(!strncmp(szBuf, "(", 1))
								{				
									/* jump over the bracket */
									szBuf++;
									/* get color */
									ExtractString(0, szBuf, szType1, ',');					
									if(strcmp(szType1, ""))
									{
										/* red */
										ipr = (unsigned char)atoi(szType1);
										ExtractString(1, szBuf, szType1, ',');						
										if(strcmp(szType1, ""))
										{
											/* green */
											ipg = (unsigned char)atoi(szType1);
											ExtractString(2, szBuf, szType1, ',');							
											if(strcmp(szType1, ""))
											{
												/* blue */
												ipb = (unsigned char)atoi(szType1);
												ExtractString(3, szBuf, szType1, ',');	
												if(strcmp(szType1, ""))
												{
													/* size */
													ip_size = (unsigned char)atoi(szType1);
													/*if(ip_size >= UF_FONT_COUNT)
													{
														printf("  Warning: Font ID is invalid.\n           Should be in range from 0 to %d. Line: %d\n", UF_FONT_COUNT-1, XML_GetCurrentLineNumber(conf->np.theParser));
														conf->np.iWarnings++;										
													}*/
												}
												/* is not obligatory */
												continue;
											}
											else
											{
												printf("  Warning: Color of the IP has invalid format.\n           Should be RGB format (i.e. ip(100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
												conf->np.iWarnings++;
												continue;
											}
										}
										else
										{
											printf("  Warning: Color of the IP has invalid format.\n           Should be RGB format (i.e. ip(100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
											conf->np.iWarnings++;
											continue;				
										}
									}	
								}
							}

							if(!strncmp(szType, RTI_NAME, 4))
							{
								info |= RI_NAME;
								labels[labIndex++] = RI_NAME;
								/* jump over name( */
								szBuf = szType;
								szBuf += 4;
								/* detect color */
								if(!strncmp(szBuf, "(", 1))
								{				
									/* jump over the bracket */
									szBuf++;
									/* get color */
									ExtractString(0, szBuf, szType1, ',');					
									if(strcmp(szType1, ""))
									{
										/* red */
										namr = (unsigned char)atoi(szType1);
										ExtractString(1, szBuf, szType1, ',');						
										if(strcmp(szType1, ""))
										{
											/* green */
											namg = (unsigned char)atoi(szType1);
											ExtractString(2, szBuf, szType1, ',');							
											if(strcmp(szType1, ""))
											{
												/* blue */
												namb = (unsigned char)atoi(szType1);
												ExtractString(3, szBuf, szType1, ',');	
												if(strcmp(szType1, ""))
												{
													/* size */
													name_size = (unsigned char)atoi(szType1);
													/*if(name_size <= -UF_FONT_COUNT)
													{
														printf("  Warning: Font ID is invalid.\n           Should be in range from 0 to %d. Line: %d\n", UF_FONT_COUNT-1, XML_GetCurrentLineNumber(conf->np.theParser));
														conf->np.iWarnings++;										
													}*/
												}
												/* is not obligatory */
											}
											else
											{
												printf("  Warning: Color of the NAME has invalid format.\n           Should be RGB format (i.e. name(100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
												conf->np.iWarnings++;
												continue;
											}
										}
										else
										{
											printf("  Warning: Color of the NAME has invalid format.\n           Should be RGB format (i.e. name(100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
											conf->np.iWarnings++;
											continue;				
										}
									}	
								}
							}

							if(!strncmp(szType, RTI_LOC, strlen(RTI_LOC)))
							{
								info |= RI_COM;
								labels[labIndex++] = RI_COM;
								/* jump over location( */
								szBuf = szType;
								szBuf += strlen(RTI_LOC);
								/* detect color */
								if(!strncmp(szBuf, "(", 1))
								{				
									/* jump over the bracket */
									szBuf++;
									/* get color */
									ExtractString(0, szBuf, szType1, ',');					
									if(strcmp(szType1, ""))
									{
										/* red */
										locr = (unsigned char)atoi(szType1);
										ExtractString(1, szBuf, szType1, ',');						
										if(strcmp(szType1, ""))
										{
											/* green */
											locg = (unsigned char)atoi(szType1);
											ExtractString(2, szBuf, szType1, ',');							
											if(strcmp(szType1, ""))
											{
												/* blue */
												locb = (unsigned char)atoi(szType1);
												ExtractString(3, szBuf, szType1, ',');	
												if(strcmp(szType1, ""))
												{
													/* size */
													loc_size = (unsigned char)atoi(szType1);
													/*if(loc_size >= UF_FONT_COUNT)
													{
														printf("  Warning: Font ID is invalid.\n           Should be in range from 0 to %d. Line: %d\n", UF_FONT_COUNT-1, XML_GetCurrentLineNumber(conf->np.theParser));
														conf->np.iWarnings++;										
													}*/
												}
												/* is not obligatory */
											}
											else
											{
												printf("  Warning: Color of the COMMENT has invalid format.\n           Should be RGB format (i.e. comment(100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
												conf->np.iWarnings++;
												continue;
											}
										}
										else
										{
											printf("  Warning: Color of the COMMENT has invalid format.\n           Should be RGB format (i.e. comment(100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
											conf->np.iWarnings++;
											continue;				
										}
									}	
								}
							}

							if(!strncmp(szType, RTI_BORDER, 6))
							{
								info |= RI_BORDER;				
								/* jump over border( */
								szBuf = szType;
								szBuf += 6;
								/* detect color */
								if(!strncmp(szBuf, "(", 1))
								{				
									/* jump over the bracket */
									szBuf++;
									/* get width */
									ExtractString(0, szBuf, szType1, ',');
									if(strcmp(szType1, ""))
									{
										iBorderWidth = atoi(szType1);
										/* get color */
										ExtractString(1, szBuf, szType1, ',');
										if(strcmp(szType1, ""))
										{
											br = (unsigned char)atoi(szType1);
											ExtractString(2, szBuf, szType1, ',');
											if(strcmp(szType1, ""))
											{
												bg = (unsigned char)atoi(szType1);
												ExtractString(3, szBuf, szType1, ',');
												if(strcmp(szType1, ""))
												{
													bb = (unsigned char)atoi(szType1);
												}
												else
												{
													printf("  Warning: Params of the BORDER has invalid format.\n           Should be RGB format (i.e. border(1, 100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
													conf->np.iWarnings++;
													continue;				
												}
											}
											else
											{
												printf("  Warning: Params of the BORDER has invalid format.\n           Should be RGB format (i.e. border(1, 100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
												conf->np.iWarnings++;
												continue;				
											}
										}
										else
										{
											printf("  Warning: Params of the BORDER has invalid format.\n           Should be RGB format (i.e. border(1, 100,100,125)). Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
											conf->np.iWarnings++;
											continue;				
										}
									}
								}
							}			
						}
					}
					return;
			}
		}
		tag = tag->pNext;
	}
}
