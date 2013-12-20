/* Netmon 0.5.0.cpp : Defines the entry point for the console application. */

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
#include "../include/error.h"
#include "../include/xmlparse.h"
#include "../include/ping.h"
 
/* Own headers */
#include "../include/macros.h"
#include "../include/xmlobject.h"
#include "../include/network.h"
#include "../include/graph.h"
#include "../include/router.h"
#include "../include/line.h"


/* place for global variables */
char		g_szMessage[ERROR_MESSAGE_SIZE];

/* Entry point */
int main(int argc, char* argv[])
{
	/* Declare output files */
	Configuration conf;
	char	szConfig[30];
	char	szMainRouter[50];
	int iHelp = 0, iTest = 0;   
	FILE *file;
	char *tempResPng, *last;
	/* init possible parameters list */
	struct t_param params_map[] = 
	{
		{"v", CC_NUM, &(conf.iVerbose)},		
		{"config", CC_STR, szConfig},
		{"mainrouter", CC_STR, szMainRouter},
		{"sl", CC_NUM, &(conf.iSurpressLines)},		
		{"test", CC_NUM, &iTest},
		{"t", CC_NUM, &iTest},		
		{"help", CC_NUM, &iHelp},
		{"h", CC_NUM, &iHelp},
		{0, 0, 0}
	};

	/* begin the netmon */
	printf("Network Monitor %s\n", VERSION);
	printf("Last build: %s\n", __DATE__);
	printf("By Jiri Formanek (jiri.formanek@gmail.com)\n");	
	printf("(c) 2003-2006\n\n");
	/* remove first parameter, this is a program name */
	argc--; argv++;
	/* zero main structure */
	//memset(&conf, 0, sizeof(Configuration));	
	/* parse command line */
	if(ParseCmdLine(argc, argv, params_map) != OK)
	{		
		printf("Cannot parse command line. Try --help for parameter's description.\n");
		return FAILURE;
	}
	/* show help */
	if(iHelp)
	{
		printf("Usage of the Network Monitor:\n\n");
		printf("--help | --h\n\t\t\tShow this message.\n\n");
		printf("--v\n\t\t\tVerbose is useful if you want to show detailed \n\t\t\tinformation during network checking.\n\n");		
		printf("--config=file.xml\n\t\t\tNetmon will use explicit user defined \n\t\t\tconfiguration file else '/opt/netmon/config.xml'\n\t\t\tis used.\n\n");		
		printf("--sl\n\t\t\tSurpress drawing of network lines.\n");		
		printf("--mainrouter=router\n\t\t\tThis value overloads options 'mainrouter' in the config.\n\n");		
		printf("--test | --t\n\t\t\tThis setting only checks the configuration file without testing the network.\n\n");		

		printf("\nExample:\n");
		printf("Netmon.exe --config=prasatko.xml\n");
		printf(" - Netmon will now use user defined configuration 'prasatko.xml'.\n");
		printf("\nSource HTML pattern is defined in the configuration file.\n");
		/* program must exit */
		return FAILURE;
	}

	if(iTest)
	{
        printf("Netmon is used in TESTING mode. It will only check the configuration file and other resources.\n\n");
	}

	/* if main router was overloaded */
	if(strlen(szMainRouter) > 0)
	{
		/* copy it to configuration */
		conf.np.szMainRouterID = new char[(strlen(szMainRouter)+1)];
		strcpy(conf.np.szMainRouterID, szMainRouter);
	}
	
	/* no explicit config was defined */
	if(szConfig[0] == 0)
	{		
		printf("Using default config file: %s\n\n", DEFAULT_CONFIG_XML);
		/* load network */
		printf("Parsing the file...\n");
		if(OK == ParseNetwork(DEFAULT_CONFIG_XML, &conf))
		{
			if(conf.np.iWarnings > 0)
			{
				printf("\nConfiguration file '%s' contains %d warning(s).\n", DEFAULT_CONFIG_XML, conf.np.iWarnings);
			}
			else
			{
				printf("\nConfiguration file '%s' is OK.\n", DEFAULT_CONFIG_XML);
			}			
		}	
		else
		{
			printf("\nFatal errors during parsing the configuration file '%s'.\n", DEFAULT_CONFIG_XML);
			printf("Warnings: %d\nErrors: %d\n", conf.np.iWarnings, conf.np.iErrors);
			printf(GetMyLastError());			
			return FAILURE;
		}
		strcpy(conf.configFileName, DEFAULT_CONFIG_XML);
	}
	else
	{
		printf("Parsing user-defined config file: %s\n", szConfig);
		printf("Parsing the file...\n");
		if(OK == ParseNetwork(szConfig, &conf))
		{
			if(conf.np.iWarnings > 0)
			{
				printf("\nConfiguration file '%s' contains %d warning(s).\n", szConfig, conf.np.iWarnings);
			}
			else
			{
				printf("\nConfiguration file '%s' is OK.\n", szConfig);
			}
		}
		else
		{
			printf("\nFatal errors during parsing the configuration file '%s'.\n", szConfig);
			printf("Warnings: %d\nErrors: %d\n", conf.np.iWarnings, conf.np.iErrors);
			printf(GetMyLastError());	
			return FAILURE;
		}
		strcpy(conf.configFileName, szConfig);
	}

	// V teto chvili jiz musi byt jasne, zda-li je definovan zdrojovy router
	// pokud neni, netmon musi skoncit
	if(!conf.np.pMainRouter)
	{
		printf("Error: The mainrouter is not defined in the configuration file nor via command line. ");
		return FAILURE;
	}

	printf("\nNumber of router types: %d\n", conf.np.iRTCount);
	printf("Number of line types: %d\n", conf.np.iLTCount);
	printf("Number of routers: %d\n", conf.np.iRoutersCount);
	printf("Number of APs: %d\n", conf.np.iAPsCount);
	printf("Number of LAN segments: %d\n", conf.np.iLANsCount);
	printf("Number of lines: %d\n", conf.np.iLinesCount);
	printf("Main router: %s\n", conf.np.pMainRouter->szName);	

	/* If load file is defined, try to parse file and find load values */
	/*if(conf.np.szLoadURL)
	{
		//conf.np.iWarnings = 0;
		printf("\nLoad file is defined. Checking...\n");
		if(OK == ParseLineLoads(conf.np.szLoadURL, &conf))
		{
			if(conf.np.iWarnings > 0)
			{
				printf("  Load file '%s' contains %d warning(s).\n", conf.np.szLoadURL, conf.np.iWarnings);
			}
			else
			{
				printf("  Load file '%s' is OK.\n", conf.np.szLoadURL);
			}
		}
		else
		{
			printf("  Fatal error during parsing the load file '%s'.\n", conf.np.szLoadURL);
			printf(GetMyLastError());			
			// return FAILURE; 
			// pokud sou chyby v souboru zatizeni, pokracuje se 
		}			
	}*/
	printf("\n");
	/* kontrola pritomnsoti obrazku k sestaveni site - AP, CABEL*/
	printf("Checking required images...\n");
	if(CheckPictures(&conf) == OK)
	{
		printf("   Images required to create result network map are OK.\n");
	}
	else
	{
		printf("Error: Some images required to create result network map are missing.\n");
		printf("       And you must define paths in the config file using <appngpath>, \n       <lanpngpath> or <ap2pngpath> tags.\n");
		return FAILURE;		
	}

	/* check font */
	if(!conf.gd.szFontPath || !(strlen(conf.gd.szFontPath) > 0))
	{
		printf("\nDefault font is not defined, netmon will use native GD fonts.\n");		
	}
	else
	{
		/* check file of the font if exists */
		file = fopen(conf.gd.szFontPath, "r");
		if(!file)
		{
			printf("\nWarning: Default font is not found. Please, check your font path!\n         Native GD fonts will be used.\n");
			conf.gd.szFontPath[0] = 0;
		}
		else
		{
			printf("Default font is OK.\n");
			fclose(file);
		}
	}
	/* generate HTML page */
	if(!conf.np.szSourcePath)
	{
		printf("\nError: Source path is not defined in the config file.\n       Use <sourcepath> tag in the <options> section.\n");
		return FAILURE;
	}

	// pokud je zadan parametr test/t, provede se pouze parsing a program se ukonci 
	if(iTest)
	{	
		if(conf.np.iWarnings > 0)
			printf("\nSome warnings occured during parsing the configuration file (see above).\n");
	
		if(conf.np.iErrors == 0 && conf.np.iWarnings == 0)		
			printf("\nThe configuration file is perfect. Now run netmon without --t parameter.\n");			

		return OK;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// Testovani site
	///////////////////////////////////////////////////////////////////////////////////////////
	conf.np.networkGraph->BuildEmptyGraph(&conf);
	conf.np.networkGraph->PlaceRouters(&conf);
	conf.np.networkGraph->ConnectRouters(&conf);
		
	printf("\n");
	printf("Checking the network...\n");
	/* check network */
	if(OK != BreadthFirstSearch(&conf))
	{
		printf("\nCannot check the network.\n");
		return FAILURE;
	}
	
	/* draw network */
	printf("Initialization of the GD library.............");
	if(OK != InitGD(&conf))
	{
		printf("  Error during initialization GD lib.\n");		
	}

	printf("\nDrawing image................................");		
	if(OK == DrawNetwork(&conf))
	{
		printf("[   OK   ]\n");
	}
	else
	{
		printf("[ FAILED ] - invalid arguments passed\n");
	}

	/* check output paths for png and html output */
	if(!conf.np.szResHTML)
	{
		conf.np.szResHTML = (char*)malloc(sizeof(char)*30);
		strcpy(conf.np.szResHTML, DEFAULT_RES_HTML);
	}
	if(!conf.np.szResPNG)
	{
		conf.np.szResPNG = (char*)malloc(sizeof(char)*50);
		strcpy(conf.np.szResPNG, DEFAULT_RES_PNG);

		tempResPng = (char*)malloc(sizeof(char)*(strlen(conf.np.szResHTML)*2+1));
		strcpy(tempResPng, conf.np.szResHTML);

		// cut html file - try both: slash and backslash
		last = strrchr(tempResPng, '/');
		if(!last) 
			last = strrchr(tempResPng, '\\');

		// if some "slash" found, cut the string behind it
		if(last)				
            *(last+1) = 0;					
		else			
			// or cut whole filename
			*tempResPng = 0;						

		// add png file
		strcat(tempResPng, DEFAULT_RES_PNG);

		/* save PNG */
		printf("Writing image...\n");
		if(OK == mySavePng(tempResPng, conf.gd.map))
		{
			printf("  Final image saved as '%s'\n\n", conf.np.szResPNG);
		}
		free(tempResPng);
	}
	else
	{			
		/* save PNG */
		printf("Writing image...\n");
		if(OK == mySavePng(conf.np.szResPNG, conf.gd.map))
		{
			printf("  Final image saved as '%s'\n\n", conf.np.szResPNG);
		}
	}

	/* generate HTML page */
	if(!conf.np.szSourcePath)
	{
		printf("Error: Source path is not defined in the config file.\n       Use <sourcepath> tag in the <options> section.\n");
	}
	else
	{
		printf("Generating HTML page using template: %s\n", conf.np.szSourcePath);
		if(OK == GenerateHTML(conf.np.szSourcePath, &conf))
		{
			printf("  Network page has been created as '%s'.\n", conf.np.szResHTML);
		}	
		else
		{
			printf("  Network page could not be created, template file may not exist.\n");
		}
	}

	printf("\n");
	printf("Releasing network............................");
	printf("[   OK   ]\n");
	
	printf("Releasing graphics...........................");
	DestroyGD(&conf);	
	printf("[   OK   ]\n\n");		
	return OK;
}

int		GenerateHTML(char *szSrcHTML, Configuration* conf)
{
	int iRet = FAILURE;
	int iTotCount = 0, iCount = 0;
	FILE	*fSource, *fDest;
	char	*chSource, *chDest;	
	Router *pRouter = conf->np.pRouters;
	size_t readen;
	char	**szMap, *szCompleteMap;
	int i = 0;
	int	x1, x2, y1, y2, dx, dy, iSize, iTotalItems, iLen, iX, iY, iX1, iY1, width;
	RouterType *pRT = NULL;
	char	szBuffer[MAX_PATH];
	char	szConn[MAX_PATH];
	char	szTempDesc[1024];

	if(conf)
	{
		/* read source file */
		fSource = fopen(szSrcHTML, "rb");
		if(!fSource) return FAILURE;		
		/* get length of file */
		fseek(fSource, 0L, SEEK_END);
		/* Load file to buffer */
		iCount = ftell(fSource);
		fseek(fSource, 0L, SEEK_SET);	
		/* Allocate buffer for file */    
		chSource = (char*) malloc(iCount * sizeof(char)+1);	
		memset(chSource, 0, iCount+1);
		/* read whole file to buffer */	
		readen = fread(chSource, sizeof(char), iCount, fSource);			
		/* Close the file */
		fclose(fSource);
		/* check data */
		if(readen != (size_t)iCount)
		{
			printf("Cannot read template file.\n");
			return FAILURE;
		}
		/* create page string */
		/* allocate space for the new page */
		/* check input params */
		if(!conf->np.pMainRouter) return FAILURE;
	
		/* total count of the area tag items (each AP, line and router has his item)*/
		iTotalItems = conf->np.iAPsCount + conf->np.iLineSegments + conf->np.iRoutersCount + conf->np.iLANsCount;
		/* allocate space for the map's items */		
		szMap = new char*[iTotalItems];
		if(!szMap) 
		{
			printf("Cannot allocate memory for the map.\n");
			return FAILURE;
		}
		/* go through all routers and get their URL */
		while(pRouter)
		{
			dy = dx = 0;
						
			/* find router type */
			pRT = conf->np.pRouterTypes;
			while(pRT)
			{			
				/* jump over the invalid types */
				/* try to find valid type */
				if(pRT->id != -1)
				{
					if(pRT->id == pRouter->rType)
					{
						dx = pRT->sx;
						dy = pRT->sy;							
						break;
					}						
				}
				pRT = pRT->pNext;
			}
			
			/* or set router name only */
			 
			// Pokud narazime na router s chybnou polohou, uplne
			// zrusime popisek, musime vytvorit fake popisek					
			if(pRouter->iValid)
			{
				if(pRouter->szRouterURL)
				{				
					iLen = (int)(strlen(AREA_ROUTER_FORMAT) + 4*4 + strlen(pRouter->szRouterURL));/* + 2 * strlen(szBuffer));*/
					szMap[i] = (char *) malloc((iLen+1) * sizeof(char));
					sprintf(szMap[i], AREA_ROUTER_FORMAT, pRouter->iRealX-dx/2, pRouter->iRealY-dy/2, pRouter->iRealX+dx/2,pRouter->iRealY+dy/2, pRouter->szRouterURL);							
					iTotCount += iLen;
				}
				else
				{
					/* area len: format len           + rect coord len +          URL len            +             name len */
					iLen = (int)(strlen(E_AREA_ROUTER_FORMAT) + 4*4);/* + 2 * strlen(szBuffer));	*/
					szMap[i] = (char *) malloc(iLen * sizeof(char) + 1);
					sprintf(szMap[i], E_AREA_ROUTER_FORMAT, pRouter->iRealX-dx/2, pRouter->iRealY-dy/2, pRouter->iRealX+dx/2,pRouter->iRealY+dy/2);							
					iTotCount += iLen;
				}
			}
			else
			{
				szMap[i] = new char[1];
				szMap[i][0] = 0;
				iTotCount++;                
			}
			i++;

			/* AP MARK s 1 antenou */
			if(pRouter->iAP == 1)
			{
				iX = pRouter->iRealX+dx/2+conf->gd.ap->sx/2-25;
				iY = pRouter->iRealY-dy/2-conf->gd.ap->sy/2;

				if(!pRouter->iValid)
				{
					iX = 0;
					iY = 0;
				}

				memset(szTempDesc, 0, 1024);
				
				/* vytvor zacatek odkazu */
				sprintf(szConn, AREA_CIRC_BEGIN, iX, iY, conf->gd.ap->sx/2);				
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);
				/* pridej AP url */
				if(pRouter->szAPURL)
				{
					sprintf(szConn, AREA_HREF, pRouter->szAPURL);
				}
				else
				{
					sprintf(szConn, AREA_NOHREF);
				}
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);

				/* title */
				strcat(szTempDesc, AREA_TITLE);
				strcat(szTempDesc, S_QUOTE); 

				/* add ESSID if any */
				if(pRouter->szESSID)
				{				
					strcat(szTempDesc, pRouter->szESSID);                    
				}
				else
				{
					strcat(szTempDesc, AREA_AP);    
				}

				/* add stats if any */
				if(pRouter->iAPLoad != 0)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iAPLoad);
					strcat(szTempDesc, szConn);
				}				
				/* end of title tag */
				strcat(szTempDesc, S_QUOTE);

				/* alt */
				strcat(szTempDesc, S_SPACE);
				strcat(szTempDesc, AREA_ALT);
				strcat(szTempDesc, S_QUOTE); 

				/* add ESSID if any */
				if(pRouter->szESSID)
				{				
					strcat(szTempDesc, pRouter->szESSID);                    
				}
				else
				{
					strcat(szTempDesc, AREA_AP);    
				}

				/* add stats if any */
				if(pRouter->iAPLoad != -1)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iAPLoad);
					strcat(szTempDesc, szConn);
				}				
				/* end of alt tag */
				strcat(szTempDesc, S_QUOTE);
				strcat(szTempDesc, AREA_END); 

				/* copy string */
				iLen = (int)strlen(szTempDesc);
				szMap[i] = (char *) malloc(iLen * sizeof(char) + 1);
				strcpy(szMap[i], szTempDesc);

				i++;
				iTotCount += iLen;				
			}
			/* AP MARK s 2 antenama */
			if(pRouter->iAP == 2)
			{
				/* first antenna */
				iX = pRouter->iRealX+dx/2-conf->gd.ap2->sx/2;
				iY = pRouter->iRealY-dy/2-conf->gd.ap2->sy;
				iX1 = pRouter->iRealX+dx/2;
				iY1 = pRouter->iRealY-dy/2;

				if(!pRouter->iValid)
				{
					iX1 = iX = 0;
					iY1 = iY = 0;
				}

				memset(szTempDesc, 0, 1024);
				
				/* vytvor zacatek odkazu */
				sprintf(szConn, AREA_RECT_BEGIN, iX, iY, iX1, iY1);				
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);
				/* pridej AP url */
				if(pRouter->szAPURL)
				{
					sprintf(szConn, AREA_HREF, pRouter->szAPURL);
				}
				else
				{
					sprintf(szConn, AREA_NOHREF);
				}
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);

				/* title */
				strcat(szTempDesc, AREA_TITLE);
				strcat(szTempDesc, S_QUOTE); 

				/* add ESSID if any */
				if(pRouter->szESSID)
				{				
					strcat(szTempDesc, pRouter->szESSID);                    
				}
				else
				{
					strcat(szTempDesc, AREA_AP);    
				}

				/* add stats if any */
				if(pRouter->iAPLoad != 0)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iAPLoad);
					strcat(szTempDesc, szConn);
				}				
				/* end of title tag */
				strcat(szTempDesc, S_QUOTE);

				/* alt */
				strcat(szTempDesc, S_SPACE);
				strcat(szTempDesc, AREA_ALT);
				strcat(szTempDesc, S_QUOTE); 

				/* add ESSID if any */
				if(pRouter->szESSID)
				{				
					strcat(szTempDesc, pRouter->szESSID);                    
				}
				else
				{
					strcat(szTempDesc, AREA_AP);    
				}

				/* add stats if any */
				if(pRouter->iAPLoad != -1)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iAPLoad);
					strcat(szTempDesc, szConn);
				}				
				/* end of alt tag */
				strcat(szTempDesc, S_QUOTE);
				strcat(szTempDesc, AREA_END); 

				/* copy string */
				iLen = (int)strlen(szTempDesc);
				szMap[i] = (char *) malloc(iLen * sizeof(char) + 1);
				strcpy(szMap[i], szTempDesc);

				i++;
				iTotCount += iLen;	

				/* second antenna */
				iX = pRouter->iRealX+dx/2-conf->gd.ap2->sx;
				iY = pRouter->iRealY-dy/2-conf->gd.ap2->sy;
				iX1 = pRouter->iRealX+dx/2-conf->gd.ap2->sx/2;
				iY1 = pRouter->iRealY-dy/2;

				if(!pRouter->iValid)
				{
					iX1 = iX = 0;
					iY1 = iY = 0;
				}

				memset(szTempDesc, 0, 1024);
				
				/* vytvor zacatek odkazu */
				sprintf(szConn, AREA_RECT_BEGIN, iX, iY, iX1, iY1);				
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);
				/* pridej AP url */
				if(pRouter->szAP2URL)
				{
					sprintf(szConn, AREA_HREF, pRouter->szAP2URL);
				}
				else
				{
					sprintf(szConn, AREA_NOHREF);
				}
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);

				/* title */
				strcat(szTempDesc, AREA_TITLE);
				strcat(szTempDesc, S_QUOTE); 

				/* add ESSID if any */
				if(pRouter->szESSID2)
				{				
					strcat(szTempDesc, pRouter->szESSID2);                    
				}
				else
				{
					strcat(szTempDesc, AREA_AP);    
				}

				/* add stats if any */
				if(pRouter->iAP2Load != -1)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iAP2Load);
					strcat(szTempDesc, szConn);
				}				
				/* end of title tag */
				strcat(szTempDesc, S_QUOTE);

				/* alt */
				strcat(szTempDesc, S_SPACE);
				strcat(szTempDesc, AREA_ALT);
				strcat(szTempDesc, S_QUOTE); 

				/* add ESSID if any */
				if(pRouter->szESSID2)
				{				
					strcat(szTempDesc, pRouter->szESSID2);                    
				}
				else
				{
					strcat(szTempDesc, AREA_AP);    
				}

				/* add stats if any */
				if(pRouter->iAP2Load != -1)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iAP2Load);
					strcat(szTempDesc, szConn);
				}				
				/* end of alt tag */
				strcat(szTempDesc, S_QUOTE);
				strcat(szTempDesc, AREA_END); 

				/* copy string */
				iLen = (int)strlen(szTempDesc);
				szMap[i] = (char *) malloc(iLen * sizeof(char) + 1);
				strcpy(szMap[i], szTempDesc);

				i++;
				iTotCount += iLen;		
			}
			/* LAN MARK */
			if(pRouter->iEth)
			{
				iX = pRouter->iRealX-dx/2-conf->gd.ca->sx/2+23;
				iY = pRouter->iRealY-dy/2-conf->gd.ca->sy/2;

				if(!pRouter->iValid)
				{
					iX = 0;
					iY = 0;
				}

				memset(szTempDesc, 0, 1024);
				
				/* vytvor zacatek odkazu */
				sprintf(szConn, AREA_CIRC_BEGIN, iX, iY, conf->gd.ca->sx/2);				
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);
				/* pridej LAN url */
				if(pRouter->szLanURL)
				{
					sprintf(szConn, AREA_HREF, pRouter->szLanURL);
				}
				else
				{
					sprintf(szConn, AREA_NOHREF);
				}
				strcat(szTempDesc, szConn);
				strcat(szTempDesc, S_SPACE);

				/* title */
				strcat(szTempDesc, AREA_TITLE);
				strcat(szTempDesc, S_QUOTE); 
				strcat(szTempDesc, AREA_ETH);
				
				/* add stats if any */
				if(pRouter->iLANLoad != -1)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iLANLoad);
					strcat(szTempDesc, szConn);
				}				
				/* end of title tag */
				strcat(szTempDesc, S_QUOTE);

				/* alt */
				strcat(szTempDesc, S_SPACE);
				strcat(szTempDesc, AREA_ALT);
				strcat(szTempDesc, S_QUOTE); 				

				/* add stats if any */
				if(pRouter->iLANLoad != -1)
				{
					strcat(szTempDesc, S_SPACE);
					sprintf(szConn, LOAD_PER_FORMAT, pRouter->iLANLoad);
					strcat(szTempDesc, szConn);
				}				
				/* end of alt tag */
				strcat(szTempDesc, S_QUOTE);
				strcat(szTempDesc, AREA_END); 

				/* copy string */
				iLen = (int)strlen(szTempDesc);
				szMap[i] = (char *) malloc(iLen * sizeof(char) + 1);
				strcpy(szMap[i], szTempDesc);
			
				i++;
				iTotCount += iLen;				
			}
			/* get next router */
			pRouter = pRouter->pNext;			
		}
		Line * pLine = conf->np.pLines;
		/* go through all lines and get their URL */
		while(pLine)
		{			
			/* choose line title if it is defined in config */
			if(pLine->szLineTitle)
			{
				strcpy(szBuffer, pLine->szLineTitle);
			}
			else
			{				
				if(pLine->pLT->desc)
				{
					strcpy(szConn, pLine->pLT->desc);
				}
				else
				{
					strcpy(szConn, "");
				}
				/* or display standard line info */
				if(pLine->iLoad != -1)
				{
					sprintf(szBuffer, LINE_TITLE_FORMAT1, szConn, pLine->iLoad);				
				}
				else
				{
					sprintf(szBuffer, LINE_TITLE_FORMAT2, szConn);				
				}
			}			

			width = pLine->pLT->width/2;
			if(width < 2) width = 3; /* minimalni sirka */

			Line	*pOldLine = pLine;
			// Pokud je linka paralelni, musime brat segmenty z jejiho brachy
			// ale pak zas musime zpetne nastavit puvodni linku kvuli dalsim vlastnostem
			if(pLine->bParallel)
				pLine = pLine->pParallelLine;

			int segCount = pLine->SegmentsCount();
			LineSegment *segment = new LineSegment();
			LineSegment *prevSeg = NULL;

			for(int s = 0; s < segCount; s++)
			{
				pLine->GetSegment(s, segment);

				GetLinePoints(pOldLine, prevSeg, segment, conf, x1, y1, x2, y2);

				if(!prevSeg)
					prevSeg = new LineSegment();
				*prevSeg = *segment;

				double fX = segment->fX;
				double fY = segment->fY;

				// Pokud narazime na linku ktera vede do invalidnich routeru, uplne
				// zrusime popisek, musime vytvorit fake popisek
				if(pLine->pNode1->iValid && pLine->pNode2->iValid)
				{
					// Odkaz se nastavuje pouze pokud existuje a pokud je stav linek OK
					if(pLine->szLineURL && pLine->pNode1->rState == RS_OK && pLine->pNode2->rState == RS_OK)
					{
						// area len: format len           + rect coord len +          URL len            +             name len 
						iLen = (int)(strlen(AREA_LINE_FORMAT) + 8*4 + strlen(pLine->szLineURL) + 2 * strlen(szBuffer));											
						szMap[i] = new char[iLen+1];	
						sprintf(szMap[i], AREA_LINE_FORMAT, x1+(int)(width*fX), y1+(int)(width*fY), x1-(int)(width*fX), y1-(int)(width*fY), x2-(int)(width*fX), y2-(int)(width*fY), x2+(int)(width*fX), y2+(int)(width*fY), pOldLine->szLineURL, szBuffer, szBuffer);													
					}
					else
					{					
						// area len: format len           + rect coord len +          URL len            +             name len 
						iLen = (int)(strlen(E_AREA_LINE_FORMAT) + 8*4 + 2 * strlen(szBuffer));											
						szMap[i] = new char[iLen+1];	
						sprintf(szMap[i], E_AREA_LINE_FORMAT, x1+(int)(width*fX), y1+(int)(width*fY), x1-(int)(width*fX), y1-(int)(width*fY), x2-(int)(width*fX), y2-(int)(width*fY), x2+(int)(width*fX), y2+(int)(width*fY), szBuffer, szBuffer);													
					}	
				}
				else
				{
					iLen = 1;
					szMap[i] = new char[iLen+1];
					szMap[i][0] = 0;
				}

				iTotCount += iLen;
				i++;			
			}
			delete segment;
			delete prevSeg;
			pLine = pOldLine;

			/* get next line */
			pLine = pLine->pNext;			
		}	
		if(i != iTotalItems)
		{
			printf("Config file is invalid. Please, check router's types.");
			return FAILURE;
		}
		/* create anchor map */
		szCompleteMap = new char[(iTotCount+1)];
		if(!szCompleteMap) 
		{
			printf("Cannot allocate memory for the complete map.\n");
			return FAILURE;
		}
		memset(szCompleteMap, 0, iTotCount+1);
		/* copy each area to complet map */
		for(i = 0; i < iTotalItems; i++)
		{
			strcat(szCompleteMap, szMap[i]);
		}
		/* release all maps */
		for(i = 0; i < iTotalItems; i++)
		{
			delete [] szMap[i];
			szMap[i] = NULL;
		}
		delete szMap;
		szMap = NULL;
		/* pass NULL to get len of the destination buffer */
		iSize = GenerateResult(NULL, chSource, conf, szCompleteMap);
		chDest = new char[(iSize + 1)];
		if(!chDest) 
		{
			printf("Cannot allocate memory for the destination file.\n");
			return FAILURE;
		}
		/* real generation of the destination buffer */
		GenerateResult(chDest, chSource, conf, szCompleteMap);
		/* write new file */
		fDest = fopen(conf->np.szResHTML, "w");
		if(!fDest) 
		{
			printf("Cannot open destination file.\n");
			return FAILURE;
		}
		fwrite(chDest, iSize, sizeof(char), fDest);
		fclose(fDest);		
		/* release all memory */
		free(szCompleteMap);
		free(chDest);
		free(chSource);
			
		iRet = OK;		
	}	
	return iRet;
}


/* parse CMD line */
int ParseCmdLine(int argc, char *argv[], struct t_param *params) 
{

	char c_arg[50];		/* nazev aktualniho parametru*/
	char *cc_arg;
	int i = 0;
	while(params[i].promenna) {		/* iniciaizace parametru */
		*(int *)params[i].promenna = 0;
		i++;
	}
	
	while(argc && (*argv)[0] == '-' && (*argv)[1] == '-') {
		/* zkopirovani nazvu parametru */
		cc_arg = c_arg;
		*argv += 2;
		while(**argv && (*cc_arg = **argv) != '=')
			(*argv)++, cc_arg++;
		*cc_arg = 0;
		(*argv)++;

		i = 0;
		/* nalezeni parametru v tabulce povolenych */
		while(params[i].jmeno && strcmp(params[i].jmeno, c_arg))
		{
			i++;
		}
		
		if (params[i].jmeno)
		{
			switch(params[i].typ) {
				case CC_NONE : {		
					/* nepouzity parametr */
					*(int *)params[i].promenna = 0;
					break;
				}
				case CC_STR : {
					strcpy((char *)params[i].promenna, *argv);
					break;
				}
				case CC_NUM : {			
					/* zapnuti priznaku */
					*(int *)params[i].promenna = 1;
					break;
				}		
			}
		}
		else
		{
			printf("Invalid parameter's name passed. Parameter '%s' is invalid.\n", c_arg);			
			return INVALID_PARAMETER_NAME;				
		}
		/* move to next parameter */	
		argc--, argv++;
	}
	/* nejake parametry po parsingu je chyba */
	if(argc > 0) 
	{		
		printf("Invalid parameters passed.\n");
		return INVALID_PARAMETER;					
	}
	else
	{
		return OK;
	}
}

