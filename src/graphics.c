/*
	Function for drawing network elements
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

/*
 *	Users headers
 */
#include "../include/error.h"
#include "../include/macros.h"
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



/*
 *	Inits all graphic objects
 *	- main image map
 *	- ap image
 *	- brushes
 *	- colors
 *  - background
 */
int	InitGD(Configuration* conf)
{
	char	szDate[100];
	time_t timer;
	struct tm *tblock;
	int iBgColor;
	RouterType *pRT = NULL;	
	char *chBuffer, *err;
	long iCount;
	int brect[8];

	conf->gd.sx = conf->gd.iCellWidth*conf->gd.iHorizontalCells+MAP_EDGE*2;
	conf->gd.sy = conf->gd.iCellHeight*conf->gd.iVerticalCells+MAP_EDGE*2;

	/* Allocate the image: sx pixels across by sy pixels tall */
	conf->gd.map = gdImageCreateTrueColor(conf->gd.sx, conf->gd.sy);
	
	/* first allocated color is background */
	iBgColor = gdImageColorAllocate(conf->gd.map, 255, 255, 255);
	gdImageFill(conf->gd.map, 10,10,iBgColor);

	/* load images of AP and ETH segment */
	chBuffer = new char[LoadFileIntoBuffer(conf->gd.szAPPath, NULL)];
	if(chBuffer)
	{
		iCount = LoadFileIntoBuffer(conf->gd.szAPPath, chBuffer);	
		if(iCount > 0)
		{
			conf->gd.ap = gdImageCreateFromPngPtr(iCount, chBuffer);		
		} 
		else
		{
			printf("Loading image '%s'...\n%s\n", conf->gd.szAPPath, GetMyLastError());
			free(chBuffer);
			return FAILURE;
		}
		free(chBuffer);
	}	
	chBuffer = new char[LoadFileIntoBuffer(conf->gd.szAP2Path, NULL)];
	if(chBuffer)
	{
		iCount = LoadFileIntoBuffer(conf->gd.szAP2Path, chBuffer);	
		if(iCount > 0)
		{
			conf->gd.ap2 = gdImageCreateFromPngPtr(iCount, chBuffer);		
		} 
		else
		{
			printf("Loading image '%s'...\n%s\n", conf->gd.szAP2Path, GetMyLastError());
			free(chBuffer);
			return FAILURE;
		}
		free(chBuffer);
	}	
	chBuffer = new char[LoadFileIntoBuffer(conf->gd.szLANPath, NULL)];
	if(chBuffer)
	{
		iCount = LoadFileIntoBuffer(conf->gd.szLANPath, chBuffer);	
		if(iCount > 0)
		{
			conf->gd.ca = gdImageCreateFromPngPtr(iCount, chBuffer);		
		}		
		else
		{
			printf("Loading image '%s'...\n%s\n", conf->gd.szLANPath, GetMyLastError());
			free(chBuffer);
			return FAILURE;
		}
		free(chBuffer);
	}	
	
	/* allocate some colors */
	conf->gd.colWhite = gdImageColorAllocate(conf->gd.map, 255, 255, 255);	
	conf->gd.colBlack = gdImageColorAllocate(conf->gd.map, 0, 0, 0);

	// Set transparent color(s)
	gdImageColorTransparent(conf->gd.ap, gdImageColorClosest(conf->gd.ap, 255, 255, 255));
	gdImageColorTransparent(conf->gd.ap2, gdImageColorClosest(conf->gd.ap2, 255, 255, 255));
	
	/* allocate color for router types */
	pRT = conf->np.pRouterTypes;
	while(pRT)
	{		
		/* jump over the invalid types */		
		if(pRT->id != -1)
		{
			pRT->color = gdImageColorResolveAlpha(conf->gd.map, pRT->r, pRT->g, pRT->b,0 );
			pRT->iBorderColor = gdImageColorAllocate(conf->gd.map, pRT->br, pRT->bg, pRT->bb);
			pRT->iNameColor = gdImageColorAllocate(conf->gd.map, pRT->namr, pRT->namg, pRT->namb);
			pRT->iIPColor = gdImageColorAllocate(conf->gd.map, pRT->ipr, pRT->ipg, pRT->ipb);
			pRT->iLocColor = gdImageColorAllocate(conf->gd.map, pRT->locr, pRT->locg, pRT->locb);
		}
		pRT = pRT->pNext;
	}

	// Clear background
	gdImageFilledRectangle(conf->gd.map, 0, 0, conf->gd.sx, conf->gd.sy, conf->gd.colWhite);

	/* draw small generation time to the corner */
	/* generate time string */
	timer = time(NULL);
	tblock = localtime(&timer);
	sprintf(szDate, DATE_FORMAT, tblock->tm_hour,tblock->tm_min, tblock->tm_mday, tblock->tm_mon+1, tblock->tm_year+1900);
	/* draw date */
	DrawString(	conf->gd.map, szDate, 
				conf->gd.sx-2, 
				conf->gd.sy-2,
				conf->gd.szFontPath, 8, conf->gd.colBlack, TA_RIGHTBOTTOM);

	/* test TT fonts if font file is valid */
	if(conf->gd.szFontPath && strlen(conf->gd.szFontPath) > 0)
	{
		printf("\n\nTesting TT font....\n");
		err = gdImageStringFT(	NULL,brect,
								0,conf->gd.szFontPath, 5, 0.,
								0,0," ");
		if (err) 
		{
			printf("  Error: %s.\n",	err);			
			return FAILURE;
		}
		else
		{
			printf("  TT font testing was successful.\n");
		}		
	}

	return OK;
}


/*
 *	Releases all graphic objects
 *	- main image map
 *	- ap image
 *	- brushes
 */
int	DestroyGD(Configuration* conf)
{
	if(conf && conf->gd.map)
	{
		gdImageDestroy(conf->gd.ap);
		gdImageDestroy(conf->gd.ap2);
		gdImageDestroy(conf->gd.map);
		gdImageDestroy(conf->gd.ca);

		return OK;
	}
	return FAILURE;
}

int	DrawNetwork(Configuration* conf)
{	
	Router *pRouter = conf->np.pRouters;
	Line *pLine = conf->np.pLines;
	Cloud* pCloud = conf->np.pClouds;
	
	// check input params 
	if(!conf->np.pMainRouter || !conf->gd.map) 
		return FAILURE;	

	// pres vsechny cloudy
	while(pCloud)
	{
		pCloud->DrawCloud(conf);
		pCloud = pCloud->pNext;
	}

	pRouter = conf->np.pRouters;
	/* go through all routers and render them */
	while(pRouter)
	{
		DrawRouterShadow(pRouter, conf);		
		pRouter = pRouter->pNext;
	}

	/* go through all lines and render them */
	/* surpress lines drawing? */
	if(!conf->iSurpressLines)		
	{
		pLine = conf->np.pLines;
		while(pLine)
		{
			// Oba routery musi mit validni pozici			
			if(pLine->pNode1->iValid && pLine->pNode2->iValid)
				DrawLine(pLine, conf);		
			pLine = pLine->pNext;
		}	
	}

	pRouter = conf->np.pRouters;
	/* go through all routers and render them */
	while(pRouter)
	{
		DrawRouter(pRouter, conf);		
		pRouter = pRouter->pNext;
	}
	return OK;	
}

void	DrawRouterShadow(Router * pRouter, Configuration* conf)
{
	RouterType *pRT;
	pRT = pRouter->prType;
	if(pRT->bShadow && pRouter->iValid)
	{
		for(int s = 0; s < 4; s++)
		{
			gdImageFilledRectangle(	conf->gd.map, 
								pRouter->iRealX - pRT->sx / 2+4-s + MAP_EDGE, 
								pRouter->iRealY - pRT->sy / 2+4-s + MAP_EDGE, 
								pRouter->iRealX + pRT->sx / 2+4-s + MAP_EDGE, 
								pRouter->iRealY + pRT->sy / 2+4-s + MAP_EDGE,
								gdImageColorAllocateAlpha(conf->gd.map, 255-s*20, 255-s*20, 255-s*20, 127-s*35));

		}
	}
}

void	DrawRouter(Router * pRouter, Configuration* conf)
{	
	RouterType *pRT = NULL;
	Interface *pInt = NULL;
	
	char bNameValid = 0;
	char bLocValid = 0;
	char bIpValid = 0;
	int iLocSize, iNameSize, iIpSize;	
	int iNameOffset, iLocOffset, iIpOffset;
	iNameOffset = iLocOffset = iIpOffset = 0;
	iNameSize = DEFAULT_NAME_FONT;
	iIpSize = DEFAULT_IP_FONT;
	iLocSize = DEFAULT_LOC_FONT;

	if(!pRouter->iValid) return;	
	
	if(conf->gd.map && pRouter)
	{		
		pRT = pRouter->prType;
		/* type was found */
		if(pRT)
		{
			if(pRouter->iRealX < 0 || pRouter->iRealX > conf->gd.sx) return;
			if(pRouter->iRealY < 0 || pRouter->iRealY > conf->gd.sy) return;

			pRouter->iRealX += MAP_EDGE;
			pRouter->iRealY += MAP_EDGE;

			/* draw LAN mark */
			if(pRouter->iEth)
			{
				gdImageSetBrush(conf->gd.map, conf->gd.ca);		
				gdImageLine(conf->gd.map, 
					pRouter->iRealX - pRT->sx / 2 + conf->gd.ca->sx/2+2, 
					pRouter->iRealY - pRT->sy / 2 - conf->gd.ca->sy/2+3,
					pRouter->iRealX - pRT->sx / 2 + conf->gd.ca->sx/2+2, 
					pRouter->iRealY - pRT->sy / 2 - conf->gd.ca->sy/2+3, gdBrushed);				
			}

			/* draw AP mark */
			if(pRouter->iAP)
			{ 				
				if(pRouter->iAP == 1)
				{
					gdImageSetBrush(conf->gd.map, conf->gd.ap);		
				}
				if(pRouter->iAP == 2)
				{
					gdImageSetBrush(conf->gd.map, conf->gd.ap2);		
				}
				gdImageLine(conf->gd.map, 
					pRouter->iRealX + pRT->sx / 2 + conf->gd.ap->sx/2-26, 	
					pRouter->iRealY - pRT->sy / 2 - conf->gd.ap->sy/2+1 , 
					pRouter->iRealX + pRT->sx / 2 + conf->gd.ap->sx/2-26, 
					pRouter->iRealY - pRT->sy / 2 - conf->gd.ap->sy/2+1, gdBrushed);						
			}			

			
			
			if(pRT->info & RI_BORDER)
			{
				/// draw outer router box 
				gdImageRectangle(	conf->gd.map, pRouter->iRealX - pRT->sx / 2, pRouter->iRealY - pRT->sy / 2, 
									pRouter->iRealX + pRT->sx / 2, 
									pRouter->iRealY + pRT->sy / 2,
									pRT->iBorderColor);
				// draw inner router box 
				gdImageFilledRectangle(	conf->gd.map, pRouter->iRealX - pRT->sx / 2 +pRT->iBorderWidth, pRouter->iRealY - pRT->sy / 2+pRT->iBorderWidth, 
									pRouter->iRealX + pRT->sx / 2-pRT->iBorderWidth, 
									pRouter->iRealY + pRT->sy / 2-pRT->iBorderWidth,
									pRT->color);
			}
			else
			{
				/// draw router box 
				gdImageFilledRectangle(	conf->gd.map, pRouter->iRealX - pRT->sx / 2, pRouter->iRealY - pRT->sy / 2, 
									pRouter->iRealX + pRT->sx / 2, 
									pRouter->iRealY + pRT->sy / 2,
									pRT->color);
			}

			/* computes texts vertical offsets */
			if(pRT->info & RI_NAME && pRouter->szName)
			{
				bNameValid = 1;
				iNameSize = GetStringHeight(pRouter->szName, conf->gd.szFontPath, pRT->name_size);
			}
			if(pRT->info & RI_IP && pRouter->pInterfaces)
			{
				bIpValid = 0;
				// pokud existuje default IP, tak se najde				
				if(pRouter->iHasDefault)
				{
					pInt = pRouter->pInterfaces;
					while(pInt)
					{
						// default se pozna podle toho, ze ma NULL name
						if(!pInt->szName)
						{
							bIpValid = 1;
							break;
						}
						pInt = pInt->pNext;
					}
				}
				// pokud ne, vybere se prvni interface
				if(!bIpValid)
				{
					bIpValid = 1;
					pInt = pRouter->pInterfaces;
				}			
				iIpSize = GetStringHeight(pInt->szIP, conf->gd.szFontPath, pRT->ip_size);
			}
			if(pRT->info & RI_COM && pRouter->szComment)	
			{
				bLocValid = 1;
				iLocSize = GetStringHeight(pRouter->szComment, conf->gd.szFontPath, pRT->loc_size);
			}	

			/* Name and IP */
			if(bNameValid && bIpValid)
			{		
				if(pRT->labels[0] & RI_NAME && (pRT->labels[1] & RI_IP||pRT->labels[2] & RI_IP)
					||pRT->labels[1] & RI_NAME && pRT->labels[2] & RI_IP)		
				{
					iIpOffset = pRT->sy / 4;
					iNameOffset = - pRT->sy / 4;
				}
				if(pRT->labels[0] & RI_IP && (pRT->labels[1] & RI_NAME||pRT->labels[2] & RI_NAME)
					||pRT->labels[1] & RI_IP && pRT->labels[2] & RI_NAME)					
				{
					iIpOffset = - pRT->sy / 4;
					iNameOffset = pRT->sy / 4;
				}				
			}
			/* Name and Location */
			if(bNameValid && bLocValid)
			{	
				if(pRT->labels[0] & RI_NAME && (pRT->labels[1] & RI_COM||pRT->labels[2] & RI_COM)
					||pRT->labels[1] & RI_NAME && pRT->labels[2] & RI_COM)				
				{
					iNameOffset = -pRT->sy / 4;
					iLocOffset = pRT->sy / 4;
				}
				if(pRT->labels[0] & RI_COM && (pRT->labels[1] & RI_NAME||pRT->labels[2] & RI_NAME)
					||pRT->labels[1] & RI_COM && pRT->labels[2] & RI_NAME)					
				{
					iNameOffset = pRT->sy / 4;
					iLocOffset = -pRT->sy / 4;
				}				
			}
			/* Location and IP */
			if(bLocValid && bIpValid)
			{
				if(pRT->labels[0] & RI_COM && (pRT->labels[1] & RI_IP||pRT->labels[2] & RI_IP)
					||pRT->labels[1] & RI_COM && pRT->labels[2] & RI_IP)
				{
					iIpOffset = pRT->sy / 4;
					iLocOffset = -pRT->sy / 4;
				}
				if(pRT->labels[0] & RI_IP && (pRT->labels[1] & RI_COM||pRT->labels[2] & RI_COM)
					||pRT->labels[1] & RI_IP && pRT->labels[2] & RI_COM)
				{
					iIpOffset = - pRT->sy / 4;
					iLocOffset = pRT->sy / 4;
				}					
			}
			/* All texts */
			if(bNameValid && bIpValid && bLocValid)
			{
				if(pRT->labels[0] & RI_NAME)				
					iNameOffset = - pRT->sy / 4 - iNameSize / 4;
				if(pRT->labels[1] & RI_NAME)				
					iNameOffset = 0;	
				if(pRT->labels[2] & RI_NAME)				
					iNameOffset = pRT->sy / 4 + iNameSize / 4;	

				if(pRT->labels[0] & RI_IP)				
					iIpOffset = - pRT->sy / 4 - iIpSize / 4;
				if(pRT->labels[1] & RI_IP)				
					iIpOffset = 0;	
				if(pRT->labels[2] & RI_IP)				
					iIpOffset = pRT->sy / 4 + iIpSize / 4;	

				if(pRT->labels[0] & RI_COM)				
					iLocOffset = - pRT->sy / 4 - iLocSize / 4;
				if(pRT->labels[1] & RI_COM)				
					iLocOffset = 0;	
				if(pRT->labels[2] & RI_COM)				
					iLocOffset = pRT->sy / 4 + iLocSize / 4;				
			}
			/* for single texts, remains offsets 0 */


			/* draw router name */
			if(bNameValid)
			{ 
				/* Draw Name string */
				DrawString(	conf->gd.map, pRouter->szName, pRouter->iRealX, pRouter->iRealY+iNameOffset,
							conf->gd.szFontPath, pRT->name_size, pRT->iNameColor, TA_CENTERED);
			
			}

			/* draw IP */
			if(bIpValid && pInt)
			{ 	
				/* Draw IP */
				DrawString(	conf->gd.map, pInt->szIP, pRouter->iRealX, pRouter->iRealY+iIpOffset,
							conf->gd.szFontPath, pRT->ip_size, pRT->iIPColor, TA_CENTERED);
			}

			/* draw location */
			if(bLocValid)			
			{ 				
				/* Draw Location string */
				DrawString(	conf->gd.map, pRouter->szComment, pRouter->iRealX, pRouter->iRealY+iLocOffset,
							conf->gd.szFontPath, pRT->loc_size, pRT->iLocColor, TA_CENTERED);
			}
		}
		else
		{
			/* draw warning message if router type is corrupted */
			DrawString(	conf->gd.map, "Invalid RT", pRouter->iRealX, pRouter->iRealY, 
						conf->gd.szFontPath, 10, conf->gd.colText, TA_CENTERED);			
		}		
	}
}

void	DrawLine(Line * pLine, Configuration* conf)
{
	int x1, y1, x2, y2;	
	Line	*pOldLine = pLine;

	if(conf->gd.map && pLine)
	{	
		// Pokud je linka paralelni, musime brat segmenty z jejiho brachy
		// ale pak zas musime zpetne nastavit puvodni linku kvuli dalsim vlastnostem
		if(pLine->bParallel)
			pLine = pLine->pParallelLine;

		gdPoint points[100];
		int numPoints = 0;
		int segCount = pLine->SegmentsCount();
		LineSegment *segment = new LineSegment();
		LineSegment *prevSeg = NULL;

		for(int i = 0; i < segCount; i++)
		{
			pLine->GetSegment(i, segment);

			GetLinePoints(pOldLine, prevSeg, segment, conf, x1, y1, x2, y2);

			if(!prevSeg)
				prevSeg = new LineSegment();
			*prevSeg = *segment;

			points[numPoints].x = x1;
			points[numPoints].y = y1;
			numPoints++;
			points[numPoints].x = x2;
			points[numPoints].y = y2;
			numPoints++;
		}
		delete segment;
		delete prevSeg;		

		// Zpet nastavujeme puvodni linku kvuli napr. sirce, typu
		pLine = pOldLine;

		int r, b, g, a;
		int da;

		// Velikost stetce
		int brushSize = pLine->pLT->width+3;
		// Stred stetce
		int cx = max(1, brushSize/2);
				
		gdImagePtr target1;
		target1 = gdImageCreateTrueColor(brushSize, brushSize);
		
		gdImageColorTransparent(target1, conf->gd.colWhite);
		gdImageAlphaBlending(target1, 0);
			
		pLine->BaseColors(r, g, b);		
		a = 127;
		da = 15;				

		gdImageFilledRectangle(	target1, 0, 0, 
									brushSize, brushSize, 
									conf->gd.colWhite);	
		
		// Antialising
		for(int i = brushSize; i >= pLine->pLT->width; i--)		
		{
			a -= da;
			if(a < 0) a = 0;				
			
			gdImageFilledEllipse(target1, 
				cx, cx,	// stred
				i,	i,	// sirka a vyska
				gdImageColorResolveAlpha(target1, r, g, b, a)); // barva							
		}		
		gdImageSetBrush(conf->gd.map, target1);			
		for(int j = 0; j < numPoints; j+=2)
		{		
			gdImageLine(conf->gd.map, points[j].x, points[j].y, points[j+1].x, points[j+1].y, gdBrushed);				
		}

		// Gradient
		for(int i = pLine->pLT->width; i >= 1; i--)		
		{
			r+=8;
			g+=8;
			b+=8;
			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;						
			
			gdImageFilledRectangle(	target1, 0, 0, 
									brushSize, brushSize, 
									conf->gd.colWhite);	
			gdImageFilledEllipse(target1, 
				cx, cx,	// stred
				i,	i,	// sirka a vyska
				gdImageColorResolveAlpha(target1, r, g, b, 0)); // barva	

			gdImageSetBrush(conf->gd.map, target1);			
			for(int j = 0; j < numPoints; j+=2)
			{		
				gdImageLine(conf->gd.map, points[j].x, points[j].y, points[j+1].x, points[j+1].y, gdBrushed);				
			}
		}	
		
		gdImageDestroy(target1);
	}
}

/* returns pos of line between pRouter and pRouter->pParent */
void	GetLinePoints(Line *pLine, LineSegment *prevSeg, LineSegment *segment,  Configuration* conf, int &x1, int &y1, int &x2, int &y2)
{	
	/* prirazeni spocitanych souradnic vystupnim parametrum */
	x1 = int((segment->pNodes[0]->x)*conf->gd.iCellWidth/2  + 0*segment->sx*pLine->pLT->width+MAP_EDGE);//+conf->gd.iCellWidth/2;
	y1 = int((segment->pNodes[0]->y)*conf->gd.iCellHeight/2 +10+ 0*segment->sy*pLine->pLT->width+MAP_EDGE);//+conf->gd.iCellHeight/2+10;
	x2 = int((segment->pNodes[1]->x)*conf->gd.iCellWidth/2  - 0*segment->sx*pLine->pLT->width+MAP_EDGE);//+conf->gd.iCellWidth/2;
	y2 = int((segment->pNodes[1]->y)*conf->gd.iCellHeight/2 +10- 0*segment->sy*pLine->pLT->width+MAP_EDGE);//+conf->gd.iCellHeight/2+10;	

	if(pLine->pParallelLine)
	{
		double a1, a2, b1, b2, c1, c2, z;
		double offset = (double)(pLine->pLT->width+pLine->pParallelLine->pLT->width)/2;

		a1 = segment->fX;
		b1 = segment->fY;

		if(pLine->bParallel)
			offset *= -1;		
		
		double x = x1 + a1 * offset;
		double y = y1 + b1 * offset;
		c1 = - a1 * x - b1 * y;

		// Zaciname u prvniho segmentu, cili vystup je trivialni
		if(!prevSeg)
		{
			x1 = (int)x;
			y1 = (int)y;
		}
		else
		{
			// jinak se vezme posledni vypocteny bod z posledniho segmentu
			x1 = (int)prevSeg->sx;
			y1 = (int)prevSeg->sy;
		}

		// Koncime u posledniho segmentu a v tom pripade je druhy bod trivialni
		if(!segment->pNext)
		{
			x2 = (int)(x2 + a1 * offset);
			y2 = (int)(y2 + b1 * offset);
		}
		else
		{
			a2 = segment->pNext->fX;
			b2 = segment->pNext->fY;	

			x = x2 + a2 * offset;
			y = y2 + b2 * offset;
			c2 = - a2 * x - b2 * y;

			CrossProduct(&x, &y, &z, a2, b2, c2, a1, b1, c1);

			x2 = (int)(x/z);
			y2 = (int)(y/z);
			segment->sx = x2;
			segment->sy = y2;
		}		
	}
}

void CrossProduct(int *returnValueX, int *returnValueY, int *returnValueZ, 
				  int leftX, int leftY, int leftZ, 
				  int rightX, int rightY, int rightZ)
{
	(*returnValueX) = leftY * rightZ - leftZ * rightY;
    (*returnValueY) = leftZ * rightX - leftX * rightZ;
	(*returnValueZ) = leftX * rightY - leftY * rightX;
}

void CrossProduct(double *returnValueX, double *returnValueY, double *returnValueZ, 
				  double leftX, double leftY, double leftZ, 
				  double rightX, double rightY, double rightZ)
{
	(*returnValueX) = leftY * rightZ - leftZ * rightY;
    (*returnValueY) = leftZ * rightX - leftX * rightZ;
	(*returnValueZ) = leftX * rightY - leftY * rightX;
}

int	CheckPictures(Configuration *conf)
{
	int iRet = OK;
	FILE *pAPFile, *pCAFile;	
	if(strlen(conf->gd.szAPPath) > 0)
	{
		pAPFile = fopen(conf->gd.szAPPath, "rb");
		if(!pAPFile) 
		{
			printf("   Cannot find '%s' file needed to show tiny antennas.\n", conf->gd.szAPPath);
			iRet = FAILURE;
		}
		else
		{
			fclose(pAPFile);
		}
	}
	else
	{
		printf("   Warning: Path for image needed to show tiny antennas is missing\n            in the conf file.\n");
		iRet = FAILURE;
	}
	if(strlen(conf->gd.szAP2Path) > 0)
	{
		pAPFile = fopen(conf->gd.szAP2Path, "rb");
		if(!pAPFile) 
		{
			printf("   Cannot find '%s' file needed to show multi antennas.\n", conf->gd.szAP2Path);
			iRet = FAILURE;
		}
		else
		{
			fclose(pAPFile);
		}
	}
	else
	{
		printf("   Warning: Path for image needed to show tiny antennas is missing\n            in the conf file.\n");
		iRet = FAILURE;
	}
	
	if(strlen(conf->gd.szLANPath) > 0)
	{		
		pCAFile = fopen(conf->gd.szLANPath, "rb");
		if(!pCAFile) 
		{
			printf("   Cannot find '%s' file needed to show ethernet segment.\n", conf->gd.szLANPath);
			iRet = FAILURE;
		}
		else
		{
			fclose(pCAFile);
		}
	}
	else
	{
		printf("   Warning: Path for image needed to show ethernet segment is missing\n            in the conf file.\n");
		iRet = FAILURE;
	}

	return iRet;
}

void DrawDashedLine(Configuration* conf, int color, int width, int x1, int y1, int x2, int y2, double fX, double fY)
{
	int x_start, x_end;
	int y_start, y_end;
	double k;
	double x, ky;
	double y;
	double px, py, sx = 0, sy = 0;
	double len = width/2+1;
	double d = 0;
	gdPoint points[4];
	gdImageSetThickness(conf->gd.map, 1);	

	if(conf->gd.map)
	{
		if(x1 == x2)
			k = 2;
		else
			k = (double)(y2 - y1) / (double)(x2 - x1);

		if(fabs(k) <= 1)
		{
			if(x1 > x2)
			{
				x = x_start = x2;
				x_end = x1;
				y = y2;
			}
			else
			{
				x = x_start = x1;
				x_end = x2;
				y = y1;
			}

			while(x < x_end)
			{			
				if(d == 0)
				{					
					sx = x; sy = y;
				}
				if(d > len)
				{
					px = (width/2) * fX;
					py = (width/2) * fY;
				
					points[1].x = (int)(sx + px);
					points[1].y = (int)(sy + py);
					points[0].x = (int)(sx - px);
					points[0].y = (int)(sy - py);

					points[2].x = (int)(x + px);
					points[2].y = (int)(y + py);
					points[3].x = (int)(x - px);
					points[3].y = (int)(y - py);
			
					if(width > 1)
					{
						gdImageFilledPolygon(conf->gd.map, points, 4, color);
					}
					else
					{
						gdImageLine(conf->gd.map, points[0].x, points[0].y, points[2].x, points[2].y, color);
					}
					d = -2;
				}			

				y += k;
				x++;
				d++;
			}
		}
		else
		{		
			if(x1 == x2)
				ky = 0;
			else
				ky = 1/k;

			if(y1 > y2)
			{
				y = y_start = y2;
				y_end = y1;
				x = x2;
			}
			else
			{
				y = y_start = y1;
				y_end = y2;
				x = x1;
			}

			while(y < y_end)
			{
				if(d == 0)
				{					
					sx = x; sy = y;
				}
				if(d > len)
				{
					px = (width/2) * fX;
					py = (width/2) * fY;
				
					points[1].x = (int)(sx + px);
					points[1].y = (int)(sy + py);
					points[0].x = (int)(sx - px);
					points[0].y = (int)(sy - py);

					points[2].x = (int)(x + px);
					points[2].y = (int)(y + py);
					points[3].x = (int)(x - px);
					points[3].y = (int)(y - py);
			
					if(width > 1)
					{
						gdImageFilledPolygon(conf->gd.map, points, 4, color);
					}
					else
					{
						gdImageLine(conf->gd.map, points[0].x, points[0].y, points[2].x, points[2].y, color);
					}
					d = -2;
				}			

				x += ky;
				y++;
				d++;
			}

		}

	}
}
