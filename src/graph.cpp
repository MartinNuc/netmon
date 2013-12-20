
/*
 * Standard headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <sys/stat.h>

#include "../include/error.h"
#include "../include/macros.h"
#include "../include/xmlobject.h"
#include "../include/network.h"
#include "../include/options.h"
#include "../include/graph.h"
#include "../include/router.h"
#include "../include/line.h"


Network::Network(void)
{
	m_pGraph = NULL;
}

Network::~Network(void)
{
}

void	Network::BuildEmptyGraph(Configuration *conf)
{
	graphSizeX = conf->gd.iHorizontalCells*2+1;
	graphSizeY = conf->gd.iVerticalCells*2+1;

	// matice hran
	m_edgeMatrix = new int*[graphSizeX*graphSizeY];
	for(int i = 0; i < graphSizeX*graphSizeY; i++)
	{
		m_edgeMatrix[i] = new int[graphSizeX*graphSizeY];

		for(int j = 0; j < graphSizeX*graphSizeY; j++)
			m_edgeMatrix[i][j] = (i == j) ? 0 : 0xFFFFFFFF;		
	}


	// matice uzlu
	m_pGraph = new Node*[graphSizeX];	
	for(int i = 0; i < graphSizeX; i++)
	{
		m_pGraph[i] = new Node[graphSizeY];
		for(int j = 0; j < graphSizeY; j++)
		{
			m_pGraph[i][j].x = i;
			m_pGraph[i][j].y = j;
			m_pGraph[i][j].pRouter = NULL;
			m_pGraph[i][j].canHoldRouter = (i % 2 && j % 2) ? true : false;

			for(int t = 0; t < 8; t++)
				m_pGraph[i][j].pAdjs[t] = NULL;
		}
	}
	int s, t;
	for(int i = 0; i < graphSizeX; i++)
	{
		for(int j = 0; j < graphSizeY; j++)
		{
			s = GetEdgeIndex(i, j);
			if(i > 0)
			{
				m_pGraph[i][j].pLeft = &m_pGraph[i-1][j];
				// zapis hrany do matice hran				
				t = GetEdgeIndex(i-1, j);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita
			}
			if(i < graphSizeX-1)
			{
				m_pGraph[i][j].pRight = &m_pGraph[i+1][j];
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i+1, j);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2;	// reflexivita
			}
			
			if(j > 0)
			{
				m_pGraph[i][j].pUp = &m_pGraph[i][j-1];
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i, j-1);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita
			}

			if(j < graphSizeY-1)
			{
				m_pGraph[i][j].pDown = &m_pGraph[i][j+1];
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i, j+1);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita
			}			

			if(m_pGraph[i][j].canHoldRouter)
			{
				// inicializace sousedu na sikmo jen u nodu na kterych muze byt umisten router
				// inicializace ukazatele i od souseda
				m_pGraph[i][j].pUpRight = &m_pGraph[i+1][j-1];
				m_pGraph[i+1][j-1].pDownLeft = &m_pGraph[i][j];
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i+1, j-1);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita

				m_pGraph[i][j].pUpLeft = &m_pGraph[i-1][j-1];
				m_pGraph[i-1][j-1].pDownRight = &m_pGraph[i][j];
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i-1, j-1);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita

				m_pGraph[i][j].pDownRight = &m_pGraph[i+1][j+1];
				m_pGraph[i+1][j+1].pUpLeft = &m_pGraph[i][j];
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i+1, j+1);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita

				m_pGraph[i][j].pDownLeft = &m_pGraph[i-1][j+1];
				m_pGraph[i-1][j+1].pUpRight = &m_pGraph[i][j];			
				// zapis hrany do matice hran
				//s = GetEdgeIndex(i, j);
				t = GetEdgeIndex(i-1, j+1);
				m_edgeMatrix[s][t] = 2;
				m_edgeMatrix[t][s] = 2; // reflexivita
			}			
		}
	}

	int line = 0;
	m_pLinks = m_pLastLink = NULL;
	for(int u = 0 ; u < graphSizeX*graphSizeY; u++)
	{	
		int i, j;
		
		for(int v = u+1; v < graphSizeX*graphSizeY; v++)
		{
			if(m_edgeMatrix[u][v] == 2)
			{
				GetNodeIndices(u, i, j);
				GetNodeIndices(v, s, t);				

				if(!m_pLinks)
				{
					m_pLastLink = m_pLinks = new Link(&m_pGraph[i][j], &m_pGraph[s][t], &m_edgeMatrix[u][v]);
				}
				else
				{
					m_pLastLink->pNext = new Link(&m_pGraph[i][j], &m_pGraph[s][t], &m_edgeMatrix[u][v]);
					m_pLastLink = m_pLastLink->pNext;
				}
				line++;
			}
		}
	}
}

void	Network::PlaceRouters(Configuration* conf)
{
	int i, j;
	Router* pRouter = conf->np.pRouters;
	RouterContainer* pFixedRouters = NULL;
	RouterContainer* pLastFixedRouters = NULL;
	RouterContainer* pFlyingRouters = NULL;
	RouterContainer* pLastFlyingRouters = NULL;
	int	flyingCount = 0;

	while(pRouter)
	{
		// place routers only with defined position
		if(pRouter->iX == -1 || pRouter->iY == -1) 
		{
			// router ktery nema definovanou pozici se prida do fronty
			// routeru ktery jsou plovouci
			// seznam se vytvari podle poctu linek (nejdrive se tedy budou umistovat ty s vice spojenimi)
			if(!pFlyingRouters)
			{
				pFlyingRouters = new RouterContainer(pRouter);		
				flyingCount++;
			}
			else
			{
				RouterContainer *pPrev = pFlyingRouters;
				pLastFlyingRouters = pFlyingRouters;
				while(pLastFlyingRouters)
				{
					if(pLastFlyingRouters->pRouter->nLines <= pRouter->nLines)
					{
						if(pPrev != pLastFlyingRouters)
						{
							pPrev->pNext = new RouterContainer(pRouter);	
							pPrev->pNext->pNext = pLastFlyingRouters;
						}
						else
						{
							pFlyingRouters = new RouterContainer(pRouter);	
							pFlyingRouters->pNext = pPrev;							
						}
						flyingCount++;
						break;
					}
					if(!pLastFlyingRouters->pNext)
					{
						pLastFlyingRouters->pNext = new RouterContainer(pRouter);
						flyingCount++;
						break;
					}

					pPrev = pLastFlyingRouters;
					pLastFlyingRouters = pLastFlyingRouters->pNext;
				}
			}
			pRouter = pRouter->pNext;
			continue;
		}

		// indexy v matici
		i = pRouter->iX*2+1;
		j = pRouter->iY*2+1;

		// router has invalid position
		if(i > graphSizeX-1 || j > graphSizeY-1 || i < 0 || j < 0) 
		{
			pRouter->iValid = 0;
			printf("  Warning: The router %s has invalid position.\n\t   Its position is outside of the network.\n", pRouter->szName);			
			conf->np.iWarnings++;
			pRouter = pRouter->pNext;			
			continue;
		}
		else
		{
			pRouter->iValid = 1;
		}


		// tento router se prida do inserted, ma spravne definovanou pozizi
		if(!pLastFixedRouters)
		{
			pLastFixedRouters = pFixedRouters = new RouterContainer(pRouter);	
		}
		else
		{
			pLastFixedRouters->pNext = new RouterContainer(pRouter);
			pLastFixedRouters = pLastFixedRouters->pNext;
		}
		// tento seznam je zatim bez poradi


		// router je umisten do matice
		m_pGraph[i][j].pRouter = pRouter;


		// a vsechny linky okolo upraveny, tvori barieru pro budouci linky
		int s = GetEdgeIndex(i, j);
		for(int q = 0; q < 8; q++)
		{
			if(m_pGraph[i][j].pAdjs[q])
			{
				int t = GetEdgeIndex(m_pGraph[i][j].pAdjs[q]->x, m_pGraph[i][j].pAdjs[q]->y);

				m_edgeMatrix[s][t] = ROUTER_EDGE_VALUE;
				m_edgeMatrix[t][s] = ROUTER_EDGE_VALUE;				
			}
		}
		pRouter = pRouter->pNext;
	}

	// ted se budou umistovat litajici krabicky
	/*RouterContainer *pFlyingRouter = pFlyingRouters;
	Router* adj;
	while(pFlyingRouter)
	{
		for(int n = 0; n < pFlyingRouter->pRouter->nLines; n++)
		{
			if(pFlyingRouter->pRouter->parLines[n]->pNode1 == pFlyingRouter->pRouter)
				adj = pFlyingRouter->pRouter->parLines[n]->pNode2;
			else
				adj = pFlyingRouter->pRouter->parLines[n]->pNode1;

			if(adj->Fly())
			{
			}
		}
        
		pFlyingRouter = pFlyingRouter->pNext;
	}

	RouterContainer *pFlyingRouter;
	int pointThreshold = (graphSizeX + graphSizeY);
		
	int index = 0;
	int originalFlyingCount = flyingCount;

	// 1. pass - nulovani bodu
	for(i = 0; i < graphSizeX; i++)
	{
		for(j = 0; j < graphSizeY; j++)
		{
			// kazdy router bude mit vlastni bodovani
			m_pGraph[i][j].points = new int[originalFlyingCount];
			m_pGraph[i][j].bestPoints = 0;
			m_pGraph[i][j].theBest = NULL;			
		}
	}

	bool somethingChanged = true;
	pFlyingRouter = pFlyingRouters;
	index = 0;	
	int att = 8;
	// jedeme tak dlouho dokud jsou vsechny litajici krabky umistene
	while(att)
	{
		
		somethingChanged = false;	
		att--;

		// 1. pass - nulovani bodu
		for(i = 0; i < graphSizeX; i++)
		{
			for(j = 0; j < graphSizeY; j++)
			{
				// kazdy router bude mit vlastni bodovani				
				for(int r = 0; r < originalFlyingCount; r++)
				{
					m_pGraph[i][j].points[r] = 0;
				}
			}
		}

			
		// postupne se prochazi vsechny litajici krabky a hleda se pro ne vhodne misto v matici
		//while(pFlyingRouter)
		{
			int max;
			// 3. pass - hledani nodu s nejvetsim poctem bodu		
			Node *pWinner = &m_pGraph[0][0];

			// preskakujeme ty hotove
			if(!pFlyingRouter->pRouter->Fly())
			{
				index++;
				// Dalsi pan na holeni
				pFlyingRouter = pFlyingRouter->pNext;
				continue;
			}

			// 2. pass - obodovani vsech prazdnych pozic
			for(i = 0; i < graphSizeX; i++)
			{
				for(j = 0; j < graphSizeY; j++)
				{
					// prochazi se jen prazdne pole a ty ktere vubec umoznuji drzet krabku
					if(!m_pGraph[i][j].pRouter && m_pGraph[i][j].canHoldRouter)
					{
						// body za polohu (uprednostnuji se polohy ke stredu mapy)
						int dx, dy;
						if(i > graphSizeX/2)
							dx = graphSizeX - i;
						else
							dx = i;
						if(j > graphSizeY/2)
							dy = graphSizeY - j;
						else
							dy = j;
						
						//if(pFlyingRouter->pRouter->nLines > 2)
						//	m_pGraph[i][j].points[index] += (dx + dy)/2;

						int adj = 0;

						// body za sousedy (vhodni sousede = vice bodu)
						for(int n = 0; n < pFlyingRouter->pRouter->nLines; n++)
						{
							// left
							if(i > 2)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i-2][j].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									m_pGraph[i][j].points[index] += graphSizeX;
									adj++;
								}
								if(m_pGraph[i-2][j].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									m_pGraph[i][j].points[index] += graphSizeX;
									adj++;
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									if(m_pGraph[i-2][j].theBest == pFlyingRouter->pRouter->parLines[n]->pNode1)
									{
										m_pGraph[i][j].points[index] += graphSizeX;
										adj++;
									}
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									if(m_pGraph[i-2][j].theBest == pFlyingRouter->pRouter->parLines[n]->pNode2)
									{
										m_pGraph[i][j].points[index] += graphSizeX;
										adj++;
									}
								}
							}
							// right
							if(i < graphSizeX-3)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i+2][j].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									m_pGraph[i][j].points[index] += graphSizeX;
									adj++;
								}
								if(m_pGraph[i+2][j].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									m_pGraph[i][j].points[index] += graphSizeX;
									adj++;
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									if(m_pGraph[i+2][j].theBest == pFlyingRouter->pRouter->parLines[n]->pNode1)
									{
										m_pGraph[i][j].points[index] += graphSizeX;
										adj++;
									}
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									if(m_pGraph[i+2][j].theBest == pFlyingRouter->pRouter->parLines[n]->pNode2)
									{
										m_pGraph[i][j].points[index] += graphSizeX;
										adj++;
									}
								}
							}
							// up
							if(j > 2)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i][j-2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									m_pGraph[i][j].points[index] += graphSizeY;
									adj++;
								}
								if(m_pGraph[i][j-2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									m_pGraph[i][j].points[index] += graphSizeY;
									adj++;
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									if(m_pGraph[i][j-2].theBest == pFlyingRouter->pRouter->parLines[n]->pNode1)
									{
										m_pGraph[i][j].points[index] += graphSizeY;
										adj++;
									}
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									if(m_pGraph[i][j-2].theBest == pFlyingRouter->pRouter->parLines[n]->pNode2)
									{
										m_pGraph[i][j].points[index] += graphSizeY;
										adj++;
									}
								}
							}
							// down
							if(j < graphSizeY-3)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i][j+2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									m_pGraph[i][j].points[index] += graphSizeY;
									adj++;
								}
								if(m_pGraph[i][j+2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									m_pGraph[i][j].points[index] += graphSizeY;
									adj++;
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									if(m_pGraph[i][j+2].theBest == pFlyingRouter->pRouter->parLines[n]->pNode1)
									{
										m_pGraph[i][j].points[index] += graphSizeY;
										adj++;
									}
								}
								if(pFlyingRouter->pRouter != pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									if(m_pGraph[i][j+2].theBest == pFlyingRouter->pRouter->parLines[n]->pNode2)
									{
										m_pGraph[i][j].points[index] += graphSizeY;
										adj++;
									}
								}
							}
							// left&up
							if(i > 2 && j > 2)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i-2][j-2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
								{
									m_pGraph[i][j].points[index] += graphSizeX/2;
									adj++;
								}
								if(m_pGraph[i-2][j-2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
								{
									m_pGraph[i][j].points[index] += graphSizeX/2;
									adj++;
								}
							}
							// left&down
							if(i > 2 && j < graphSizeY-3)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i-2][j+2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
									m_pGraph[i][j].points[index] += graphSizeX/2;
								if(m_pGraph[i-2][j+2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
									m_pGraph[i][j].points[index] += graphSizeX/2;
							}
							// right&up
							if(i < graphSizeX-3 && j > 2)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i+2][j-2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
									m_pGraph[i][j].points[index] += graphSizeX/2;
								if(m_pGraph[i+2][j-2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
									m_pGraph[i][j].points[index] += graphSizeX/2;
							}
							// right&down
							if(i < graphSizeX-3 && j < graphSizeY-3)
							{
								// pokud je v teto pozici spravny soused, prictou se body
								if(m_pGraph[i+2][j+2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode1)
									m_pGraph[i][j].points[index] += graphSizeX/2;
								if(m_pGraph[i+2][j+2].pRouter == pFlyingRouter->pRouter->parLines[n]->pNode2)
									m_pGraph[i][j].points[index] += graphSizeX/2;
							}
						}

						//if(adj == pFlyingRouter->pRouter->nLines)
						//	m_pGraph[i][j].points[index] += graphSizeX;
						if(adj == pFlyingRouter->pRouter->nLines-1) ;
						adj = 0;
							//m_pGraph[i][j].points[index] += graphSizeX/2;
						//if(adj == pFlyingRouter->pRouter->nLines-2)
						//	m_pGraph[i][j].points[index] += graphSizeX/4;

					}
				}
			}
			//index++;
			// Dalsi pan na holeni
			//pFlyingRouter = pFlyingRouter->pNext;	
		//}

		//int max;
		// 3. pass - hledani nodu s nejvetsim poctem bodu		
		//Node *pWinner = &m_pGraph[0][0];

		//index = 0;
		//pFlyingRouter = pFlyingRouters;								
							
		//while(pFlyingRouter)
		//{			
				
			// preskakujeme ty hotove
			if(!pFlyingRouter->pRouter->Fly())
			{
				index++;
				// Dalsi pan na holeni
				pFlyingRouter = pFlyingRouter->pNext;
				continue;
			}

			max = 0;
			// nejprve se najde vitezne policko pro dany router
			for(i = 0; i < graphSizeX; i++)
			{
				for(j = 0; j < graphSizeY; j++)
				{
					// prochazi se jen prazdne pole a ty ktere vubec umoznuji drzet krabku
					if(!m_pGraph[i][j].pRouter && m_pGraph[i][j].canHoldRouter)
					{						
						// ulozeni routeru s maximalnim poctem bodu na teto pozici
						if(m_pGraph[i][j].points[index] > max)
						{
							// na teto pozici je jiz umisten nejaky flying router a je na tom lip
							if(m_pGraph[i][j].theBest && m_pGraph[i][j].bestPoints >= m_pGraph[i][j].points[index])
								continue;

							bool ekv = false;
							// Na tyhle pozici uz jeden router, jdu zkusit najit ekvivalentni misto bez
							if(m_pGraph[i][j].theBest)
							{
								for(int ix = 0; ix < graphSizeX; ix++)
								{
									for(int jx = 0; jx < graphSizeY; jx++)
									{
										if(!m_pGraph[ix][jx].theBest)
										{
											if(m_pGraph[i][j].points[index] == m_pGraph[ix][jx].points[index])
											{
												ekv = true;											
												break;
											}
										}
									}
								}
							}
							
							if(!ekv)
							{
								pWinner = &m_pGraph[i][j];
								max = m_pGraph[i][j].points[index];
							}
						}		
					}
				}
			}			
			// pWinner je vitezne pole pro dany router
			// max je pocet bodu ktere tento router na tomto poli ziskal


			// nyni se podivame jestli uz na danem poli neni nejaky vyhodnejsi router
			if(pWinner->bestPoints < max)
			{
				if(pFlyingRouter->oldPosition)
				{
					pFlyingRouter->oldPosition->theBest = NULL;
					pFlyingRouter->oldPosition->bestPoints = 0;
				}
				if(pWinner->theBest)
				{
					
				}

				// a pokud ne, prepiseme ten aktualni tim co jsme spocitali nyni
				pWinner->theBest = pFlyingRouter->pRouter;
				pWinner->bestPoints = max; // upravime novy pocet bodu
				pFlyingRouter->oldPosition = pWinner;
				
				somethingChanged = true;
			}
			// dany router bude umisten na poli pWinner pokud uz tam nelezi nejaky vyhodnejsi
			// v opacnym pripade bude tento router umisten az v dalsim pruchodu			
		}

		index++;
		// Dalsi pan na holeni
		pFlyingRouter = pFlyingRouter->pNext;
		if(!pFlyingRouter) 
			pFlyingRouter = pFlyingRouters;
		// nyni je na kazde prazdne poloze ulozen router ktery se na toho misto nejvice hodi
		// ne vsechny routery musi byt nutne na nejakym poli!!!	
		printf(".");
	}

	// projedou se vsechny pole a umisti se routery ktere jsou k nim asociovane
	for(i = 0; i < graphSizeX; i++)
	{
		for(j = 0; j < graphSizeY; j++)
		{
			// prochazi se jen prazdne pole a ty ktere vubec umoznuji drzet krabku
			if(!m_pGraph[i][j].pRouter && m_pGraph[i][j].canHoldRouter)
			{
				// 4. pass - finalni umisteni routeru
				if(m_pGraph[i][j].theBest && m_pGraph[i][j].bestPoints > 0)
				{
					// Umisteni krabky na nejlepsi misto
					m_pGraph[i][j].pRouter = m_pGraph[i][j].theBest;
					flyingCount--;
					// Poloha
					int u = (m_pGraph[i][j].x-1)/2;
					int v = (m_pGraph[i][j].y-1)/2;
					m_pGraph[i][j].pRouter->iX = u;
					m_pGraph[i][j].pRouter->iY = v;
					m_pGraph[i][j].pRouter->iRealX = u*conf->gd.iCellWidth+conf->gd.iCellWidth/2;
					m_pGraph[i][j].pRouter->iRealY = v*conf->gd.iCellHeight+conf->gd.iCellHeight/2+10;

					// a vsechny linky okolo upraveny, tvori barieru pro budouci linky
					int s = GetEdgeIndex(m_pGraph[i][j].x, m_pGraph[i][j].y);
					for(int q = 0; q < 8; q++)
					{
						if(m_pGraph[i][j].pAdjs[q])
						{
							int t = GetEdgeIndex(m_pGraph[i][j].pAdjs[q]->x, m_pGraph[i][j].pAdjs[q]->y);

							m_edgeMatrix[s][t] = ROUTER_EDGE_VALUE;
							m_edgeMatrix[t][s] = ROUTER_EDGE_VALUE;				
						}
					}					
				}
			}
		}
	}
	

			for(int i = 0; i < graphSizeX; i++)
			{
				for(int j = 0; j < graphSizeY; j++)
				{
					// testy se provadi jen pro mista kde muze byt router a uz tam nejaky neni
					if(m_pGraph[i][j].canHoldRouter && !m_pGraph[i][j].pRouter)
					{
						// 1. pass - nulovani bodu
						pFlyingRouter = pFlyingRouters;
						while(pFlyingRouter)
						{
							pFlyingRouter->points = 0;
							pFlyingRouter = pFlyingRouter->pNext;
						}

						// 2. pass
						pFlyingRouter = pFlyingRouters;
						while(pFlyingRouter)
						{
							// seznam pravidel ktery musi router splnit
							
							// 1. test na pocet okolnich bunek ktery muzou nest router
							int n = 0;
							for(int q = 0; q < 8; q++)
							{
								if(m_pGraph[i][j].pAdjs[q])
								{
									n++;
								}
							}
							if(pFlyingRouter->pRouter->nLines <= n)
								pFlyingRouter->points++;


							for(int q = 0; q < 8; q++)
							{
								if(m_pGraph[i][j].pAdjs[q])
								{
									for(int w = 0; w < 8; w++)
									{
										if(m_pGraph[i][j].pAdjs[q]->pAdjs[w] && m_pGraph[i][j].pAdjs[q]->pAdjs[w]->pRouter)
										{
											for(int l = 0; l < pFlyingRouter->pRouter->nLines; l++)
											{
												if(		pFlyingRouter->pRouter->parLines[l]->pNode1 == m_pGraph[i][j].pAdjs[q]->pAdjs[w]->pRouter
													||	pFlyingRouter->pRouter->parLines[l]->pNode2 == m_pGraph[i][j].pAdjs[q]->pAdjs[w]->pRouter)
												{
													if(w == NODE_UP || w == NODE_DOWN || w == NODE_RIGHT || w == NODE_LEFT)
														pFlyingRouter->points += 4;
													else
														pFlyingRouter->points += 3;
												}
											}
										}
									}
								}
							}					

							pFlyingRouter = pFlyingRouter->pNext;
						}

						// 3. pass - nalezne viteze, tedy router s nejvetsim poctem bodu
						pFlyingRouter = pFlyingRouters;
						pWinner = NULL;
						while(pFlyingRouter)
						{
							if(pFlyingRouter->points > pFlyingRouter->lastPoints)
							{
								if(!pWinner && pFlyingRouter->points > 0)
									pWinner = pFlyingRouter;
								if(pWinner && pWinner->points < pFlyingRouter->points)
									pWinner = pFlyingRouter;
							}
							
							pFlyingRouter = pFlyingRouter->pNext;
						}

						// Umisteni viteze
						if(pWinner)
						{					
							if(pWinner->pRouter->pNode)
								pWinner->pRouter->pNode->pRouter = NULL;

							pWinner->lastPoints = pWinner->points;

							m_pGraph[i][j].pRouter = pWinner->pRouter;
							m_pGraph[i][j].pRouter->iX = ((i-1)/2);
							m_pGraph[i][j].pRouter->iY = ((j-1)/2);
							m_pGraph[i][j].pRouter->iRealX = ((i-1)/2)*conf->gd.iCellWidth+conf->gd.iCellWidth/2;
							m_pGraph[i][j].pRouter->iRealY = ((j-1)/2)*conf->gd.iCellHeight+conf->gd.iCellHeight/2+10;
							m_pGraph[i][j].pRouter->pNode = &m_pGraph[i][j];

							// a vsechny linky okolo upraveny, tvori barieru pro budouci linky
							int s = GetEdgeIndex(i, j);
							for(int q = 0; q < 8; q++)
							{
								if(m_pGraph[i][j].pAdjs[q])
								{
									int t = GetEdgeIndex(m_pGraph[i][j].pAdjs[q]->x, m_pGraph[i][j].pAdjs[q]->y);

									m_edgeMatrix[s][t] = ROUTER_EDGE_VALUE;
									m_edgeMatrix[t][s] = ROUTER_EDGE_VALUE;				
								}
							}
						}
					}
				}
				
			}*/
		





	/*FILE* f = fopen("log", "w");
	fprintf(f, "    ");

	for(int j = 0; j < graphSizeX*graphSizeY; j++)
		fprintf(f, "%2d ", j);

	fprintf(f, "\n");
	for(int i = 0; i < graphSizeX*graphSizeY; i++)
	{
		fprintf(f, "%2d: ", i);
		for(int j = 0; j < graphSizeX*graphSizeY; j++)
			fprintf(f, "%2d ", m_edgeMatrix[i][j]);
		
		fprintf(f, "\n");		
	}
	fclose(f);*/
}

void	Network::ConnectRouters(Configuration* conf)
{
	int i, j, s ,t, x, y, u, a, b;
	Line* pLine = conf->np.pLines;
	time_t	writtenTime;
	bool readFromFile = false;
	char conFileName[MAX_PATH];
	int index[4];
	u = 0;

	// Pripravi jmeno souboru pro connections
	strcpy(conFileName, conf->configFileName);
	// nahradi priponu
	StringReverse(conFileName);
	conFileName[0] = 'n';
	conFileName[1] = 'o';
	conFileName[2] = 'c';
	StringReverse(conFileName);

#ifdef WIN32
	struct _stat configFileStatus;
	// otevreni konfiguracniho souboru a zjisteni datum modifikace
	FILE* fConfig = fopen(conf->configFileName, "r");
	if(fConfig) 
	{
		if(_fstat( _fileno(fConfig), &configFileStatus ) != 0)
			printf("Error: Cannot obtain stat from config file.\n");
	}
	else
	{
		printf("Fatal error: Cannot open configuration file '%s' for reading.\n", conf->configFileName);
		return;
	}
	fclose(fConfig);
#else
	struct stat configFileStatus;
	// otevreni konfiguracniho souboru a zjisteni datum modifikace
	FILE* fConfig = fopen(conf->configFileName, "r");
	if(fConfig) 
	{
		if(fstat( fileno(fConfig), &configFileStatus ) != 0)
			printf("Error: Cannot obtain stat from config file.\n");
	}
	else
	{
		printf("Fatal error: Cannot open configuration file '%s' for reading.\n", conf->configFileName);
		return;
	}
	fclose(fConfig);
#endif

	// try to open file with saved connections
	FILE* fConnections = fopen(conFileName, "r");
	if(fConnections) // pokud je ulozen soubor se linkami, ma cenu neco testovat
	{		
		// nacteni ulozeneho casu v souboru
		fread(&writtenTime, sizeof(time_t), 1, fConnections);

		// porovnani casu
		if(fabs(difftime(configFileStatus.st_mtime, writtenTime)) < 1)
		{
			// casy jsou temer shodne, takze precteme linky z tohoto souboru
			readFromFile = true;
		}			
	}
	// je treba vytvorit novy soubor protoze ten co mame je stary nebo neexistuje
	if(!readFromFile)
	{
		// uzavrit predchozi soubor pro cteni
		if(fConnections) 
			fclose(fConnections);

		// otevrit novy soubor pro zapis
		fConnections = fopen(conFileName, "w");
		
		// ihned se zapise cas modifikace konfiguracniho souboru
		fwrite(&configFileStatus.st_mtime, sizeof(time_t), 1, fConnections);		
	}
	

	printf("\nCreating links between routers..");
	while(pLine)
	{
		// pocatecni a koncovy uzel
		i = pLine->pNode1->iX * 2 + 1;
		j = pLine->pNode1->iY * 2 + 1;
		s = pLine->pNode2->iX * 2 + 1;
		t = pLine->pNode2->iY * 2 + 1;

		// Linky ktere vedou do neexistujiciho nodu se preskakuji
		if(i < 0 || j < 0 || s < 0 || t < 0)
		{
			pLine = pLine->pNext;
			continue;
		}
		if(i > graphSizeX-1 || j > graphSizeY-1 || s > graphSizeX-1 || t > graphSizeY-1)
		{
			pLine = pLine->pNext;
			continue;
		}


		// dijksrtuv algoritmus mezi uzly m_pGraph[i][j] a m_pGraph[s][t]
		// vsechny uzle maji na zacatku nekonecnou vzdalenost a predchudce NULL
		for(y = 0; y < graphSizeY; y++)
		{
			for(x = 0; x < graphSizeX; x++)
			{
				m_pGraph[x][y].d = 0xFFFF;
				m_pGraph[x][y].p = NULL;
			}
		}
		m_pGraph[i][j].d = 0; // pocatecni uzel ma vzdalenost 0
		m_pGraph[i][j].p = NULL;
		
		// pro kazdy uzel		
		if(!readFromFile)
		{
			printf(".");
			for(int u = 1; u < graphSizeY*graphSizeX; u++)
			{			
				// pro kazdou hranu
				Link *link = m_pLinks;
				while(link) // proved relaxaci
				{
					Relax(link->pNodes[0], link->pNodes[1]);
					Relax(link->pNodes[1], link->pNodes[0]);
					link = link->pNext;
				}			
			}
		}
		else
		{
			for(;;)
			{
				fread(index, sizeof(int), 4, fConnections);
				if(index[0] == (int)0xffffffff) break; // precten oddelovac 
				m_pGraph[index[0]][index[1]].p = &m_pGraph[index[2]][index[3]];
			} 			
		}
		

		// minimalni cesty nalezeny
		// nyni se projde zpetne 
		Node *prev = &m_pGraph[s][t];
		Node *min = prev->p;
		
		while(min)
		{			
			// novy segment
			if(!pLine->pPath)
			{
				pLine->pLastPath = pLine->pPath = new LineSegment(prev, min);
			}
			else
			{
				pLine->pLastPath->pNext = new LineSegment(prev, min);
				pLine->pLastPath = pLine->pLastPath->pNext;
			}	

			if(!readFromFile)
			{
				// zapis daneho segmentu do souboru
				fwrite(&pLine->pLastPath->pNodes[0]->x, sizeof(int), 1, fConnections);
				fwrite(&pLine->pLastPath->pNodes[0]->y, sizeof(int), 1, fConnections);
				fwrite(&pLine->pLastPath->pNodes[1]->x, sizeof(int), 1, fConnections);
				fwrite(&pLine->pLastPath->pNodes[1]->y, sizeof(int), 1, fConnections);
			}
						
			// upravit hrany vedouci k mezilehlym uzlum
			// okolo linek se udelaji jakesi bariery ktere zabranuji krizeni
			if(min != &m_pGraph[s][t] && min != &m_pGraph[i][j])
			{
				a = GetEdgeIndex(min->x, min->y);
				for(int u = 0; u < 8; u++)
				{
					if(min->pAdjs[u])
					{
						b = GetEdgeIndex(min->pAdjs[u]->x, min->pAdjs[u]->y);
						m_edgeMatrix[a][b] = LINE_EDGE_VALUE;
						m_edgeMatrix[b][a] = LINE_EDGE_VALUE;
					}
				}
			}
			// dalsi segment
			prev = min;
			min = min->p;			
		}

		// zapis oddelovace mezi linkami
		if(!readFromFile)
		{
			int r = 0xffffffff;
			fwrite(&r, sizeof(int), 1, fConnections);
			fwrite(&r, sizeof(int), 1, fConnections);
			fwrite(&r, sizeof(int), 1, fConnections);
			fwrite(&r, sizeof(int), 1, fConnections);
		}

		if(pLine->bParallel)
			conf->np.iLineSegments += pLine->pParallelLine->SegmentsCount();
		else
			conf->np.iLineSegments += pLine->SegmentsCount();

		// dalsi linka
		pLine = pLine->pNext;
	} 
	printf("\n");
	if(!readFromFile)
	{
		printf("   The new connection file '%s' has been generated.\n", conFileName);
	}
	else
	{
		printf("   Connections have been loaded from file '%s'.\n", conFileName);
	}

	fclose(fConnections);
}

int Network::Relax(Node* u, Node* v)
{
	int s = GetEdgeIndex(u->x, u->y);
	int t = GetEdgeIndex(v->x, v->y);

	unsigned int newDist = u->d + m_edgeMatrix[s][t];
	// nasla se lepsi cesta k uzlu v cili se upravi
	// vzdalenost k tomuto uzlu a predchudce
	if(v->d >= newDist)
	{
		if(v->p && v->d == newDist)
		{
			// vzajemna pozice noveho uzlu(u) a uzlu do ktereho hledame cestu(v)
			int pos1 = Position(v, u);
			// vzajemna pozice puvodniho predchudce a uzlu do ktereho hledame cestu (v)
			int pos2 = Position(v, v->p);

			//if(pos1 >= 0 && pos2 >= 0)
			{
				// davame prednost nizsim hodnotam (left, right, up, down)
				if(pos1 > pos2)
					return 1;
			}			
		}
		
		v->d = newDist;
		v->p = u;	
		return 0;
	}
	return 1;
}

int	Network::Position(Node* u, Node* v)
{
	for(int i = 0; i < 8; i++)
	{
		if(u->pAdjs[i] && u->pAdjs[i] == v)
			return i;
	}
	return -1;
}
