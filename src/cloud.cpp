
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
#include "../include/cloud.h"


#ifdef WIN32
	#include "../gdwin32/gd.h"
	#include "../gdwin32/gdfontl.h"
	#include "../gdwin32/gdfontmb.h"
	#include "../gdwin32/gdfontt.h"
	#include "../gdwin32/gdfonts.h"
	#include "../gdwin32/gdfontg.h"
#else
	#include "gd.h"
	#include "gd.h"
	#include "gdfontl.h"
	#include "gdfontmb.h"
	#include "gdfontt.h"
	#include "gdfonts.h"
	#include "gdfontg.h"
#endif



Cloud::Cloud(void)
{
	pRouters = pLastRouter = NULL;
	pNext = NULL;
	border = false;
	cldColor[0] = 255;
	cldColor[1] = 255;
	cldColor[2] = 200;
	iEdge = CLOUD_EDGE;

	DefineTag(XML_CBORDER, TT_USER, NULL);
	DefineTag(XML_CCOLOR, TT_USER, NULL);
	DefineTag(XML_CEDGE, TT_INT, &iEdge);
}

Cloud::~Cloud(void)
{
	RouterList* rl,* rld;
	rld = pRouters;
	while(rld)
	{
		rl = rld->pNext;
		SAFE_DELETE(rld);
		rld = rl;
	}
}

void	Cloud::Parse(char* tagName, const char *s, int len, Configuration *conf)
{
	XMLTag *tag = tagList;	
	char	szTemp[MAX_PATH];
	char	szPos[MAX_PATH];	
	
	while(tag)
	{
		if(!strcmp(tagName, tag->name))
		{
			switch(tag->type)
			{				
				case TT_USER:	
					if(!strcmp(tagName, XML_CBORDER))
					{
						strncpy(szPos, s, len);
						szPos[len] = 0;
						/* use temporarly szType variable */

						for(int i = 0; i < 3; i++)
						{
							ExtractString(i, szPos, szTemp, ' ');
							if(!strcmp(szTemp, ""))
							{
								printf("  Warning: Cloud has invalid border color format.\n           Should be for example <cborder>0 100 50</cborder>. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
								conf->np.iWarnings++;
								return;
							}
							borColor[i] = atoi(szTemp);
						}
					
						tag->set = 1;
						border = true;
					}
					if(!strcmp(tagName, XML_CCOLOR))
					{
						strncpy(szPos, s, len);
						szPos[len] = 0;
						/* use temporarly szType variable */

						for(int i = 0; i < 3; i++)
						{
							ExtractString(i, szPos, szTemp, ' ');
							if(!strcmp(szTemp, ""))
							{
								printf("  Warning: Cloud has invalid color format.\n           Should be for example <ccolor>0 100 50</ccolor>. Line: %d\n", XML_GetCurrentLineNumber(conf->np.theParser));
								conf->np.iWarnings++;
								return;
							}
							cldColor[i] = atoi(szTemp);
						}					
						tag->set = 1;						
					}
					break;
				default:
					XMLObject::Parse(tagName, s, len, conf);
					break;
			}
		}
		tag = tag->pNext;
	}
}

void	Cloud::PostParse(Configuration *conf)
{
	XMLObject::PostParse(conf);
}

void	Cloud::AddRouter(Router*	pRouter)
{
	RouterList	*rl = new RouterList;
	rl->pNext = NULL;
	rl->pRouter = pRouter;
	if(!pRouters)
	{		
		pRouters = rl;
	}
	else
	{
		pLastRouter->pNext = rl;
	}
	pLastRouter = rl;
}

void	Cloud::DrawCloud(Configuration *conf)
{
	if(conf->gd.map)
	{		
		gdPoint point;
		gdPoint	finalPoints[100];
		ClNode	nodes[100];				
		int numPoints = 0;		
		RouterList	*rl = pRouters;
		int p = 0;
		int n = 0;

		while(rl)
		{
			int offsetX = rl->pRouter->prType->sx/2 + iEdge;
			int offsetY = rl->pRouter->prType->sy/2 + iEdge;

			for(int i = 0; i < 4; i++)
			{
				point.x = rl->pRouter->iRealX + MAP_EDGE - offsetX;
				point.y = rl->pRouter->iRealY + MAP_EDGE - offsetY;

				offsetY *= -1;
				if(i == 1) 
					offsetX *= -1;

				for(p = 0; p < numPoints; p++)
				{
					if(point.x == nodes[p].pos.x && point.y == nodes[p].pos.y)
						break;
				}
				if(p >= numPoints)
				{
					nodes[numPoints].pos = point;
					nodes[numPoints].level = 0;
					nodes[numPoints].open = true;
					nodes[numPoints].dist[0] = nodes[numPoints].dist[1] = nodes[numPoints].dist[2] = nodes[numPoints].dist[3] = 5000;
					nodes[numPoints].adj[0] = nodes[numPoints].adj[1] = nodes[numPoints].adj[2] = nodes[numPoints].adj[3] = NULL;                   
					numPoints++;
				}
			}
			rl = rl->pNext;
		}

		// Urci se sousedi
		for(n = 0; n < numPoints; n++)
		{
			for(int a = 0; a < numPoints; a++)
			{
				if(a != n)
				{
					int d = Dist(nodes[a], nodes[n]);
					// UP
					if(nodes[a].pos.x == nodes[n].pos.x && nodes[a].pos.y < nodes[n].pos.y)
					{
						if(d < nodes[n].dist[CD_UP])
						{
							nodes[n].adj[CD_UP] = &nodes[a];
							nodes[n].dist[CD_UP] = d;
						}						
					}
					// DOWN
					if(nodes[a].pos.x == nodes[n].pos.x && nodes[a].pos.y > nodes[n].pos.y)
					{
						if(d < nodes[n].dist[CD_DOWN])
						{
							nodes[n].adj[CD_DOWN] = &nodes[a];
							nodes[n].dist[CD_DOWN] = d;
						}
					}
					// RIGHT
					if(nodes[a].pos.x > nodes[n].pos.x && nodes[a].pos.y == nodes[n].pos.y)
					{
						if(d < nodes[n].dist[CD_RIGHT])
						{
							nodes[n].adj[CD_RIGHT] = &nodes[a];
							nodes[n].dist[CD_RIGHT] = d;
						}
					}
					// LEFT
					if(nodes[a].pos.x < nodes[n].pos.x && nodes[a].pos.y == nodes[n].pos.y)
					{
						if(d < nodes[n].dist[CD_LEFT])
						{
							nodes[n].adj[CD_LEFT] = &nodes[a];
							nodes[n].dist[CD_LEFT] = d;
						}
					}
				}
			}
		}

		// Urci se stupne uzlu
		for(n = 0; n < numPoints; n++)
		{
			for(int i = 0; i < 4; i++)
			{
				if(nodes[n].adj[i])							
					nodes[n].level++;
			}
		}

		
		// Urci se hrany
		for(n = 0; n < numPoints; n++)
		{
			if(nodes[n].level == 2)
				break;
		}
		ClNode* currentNode = &nodes[n];
		ClNode* startNode = currentNode;

		int currentPoint = 0;
		int currentDir = CD_RIGHT;
		currentNode->open = false;
		finalPoints[currentPoint++] = startNode->pos;	

		for(n = 0; n < numPoints; n++)
		{
			// podivame se doprava
			currentDir = currentDir - 1;
			if(currentDir < 0) currentDir = 3;

			if(currentNode->adj[currentDir] && currentNode->adj[currentDir]->open)
			{				
				currentNode = currentNode->adj[currentDir];
				currentNode->open = false;
				finalPoints[currentPoint++] = currentNode->pos;					
			}
			else
			{
				// zkusime primy smer
				currentDir = currentDir + 1;
				if(currentDir > 3) currentDir = 0;
				if(currentNode->adj[currentDir] && currentNode->adj[currentDir]->open)
				{					
					currentNode = currentNode->adj[currentDir];
					currentNode->open = false;
					finalPoints[currentPoint++] = currentNode->pos;									
				}
				else
				{
					// musime doleva
					currentDir = currentDir + 1;
					if(currentDir > 3) currentDir = 0;
					if(currentNode->adj[currentDir] && currentNode->adj[currentDir]->open)
					{						
						currentNode = currentNode->adj[currentDir];
						currentNode->open = false;
						finalPoints[currentPoint++] = currentNode->pos;									
					}
				}
			}
			if(currentNode == startNode)
				break;
		}		

		gdImageFilledPolygon(conf->gd.map, finalPoints, currentPoint, gdImageColorResolve(conf->gd.map, cldColor[0], cldColor[1], cldColor[2]));
		if(border)
		{
			gdImagePolygon(conf->gd.map, finalPoints, currentPoint, gdImageColorResolve(conf->gd.map, borColor[0], borColor[1], borColor[2]));
		}
	}
}

int	Cloud::Dist(ClNode & n1, ClNode & n2)
{
	return (int)sqrt(pow(n1.pos.x-n2.pos.x, 2) + pow(n1.pos.y-n2.pos.y, 2));
}

