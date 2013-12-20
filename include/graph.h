#ifndef GRAPH_H
#define GRAPH_H


struct Link;
struct Node;

struct	RouterContainer
{
	Router*		pRouter;
	RouterContainer	*pNext;

	Node*		oldPosition;

	RouterContainer(Router * router)
	{
		pRouter = router;
		pNext = NULL;
		oldPosition = NULL;
	}
};

struct Node
{
	// jen urcite uzly mohou nest router
	int			canHoldRouter;
	Router*		pRouter; // ukazatel na router pokud je pritomen
	
	// pozice
	int			x, y;

	// body pri urcovani polohy routeru
	int			*points;
	Router		*theBest;
	int			bestPoints;

	// dijskra algoritmus
	unsigned int	d; // vzdalenost
	Node*			p; // predchudce


	// 8 hran
	union
	{
		#define		NODE_UP				0	
		#define		NODE_DOWN			1	
		#define		NODE_LEFT			2	
		#define		NODE_RIGHT			3	
		#define		NODE_UP_LEFT		4	
		#define		NODE_UP_RIGHT		5	
		#define		NODE_DOWN_LEFT		6	
		#define		NODE_DOWN_RIGHT		7	
		Node		*pAdjs[8];
		struct 
		{
			Node* pUp, *pDown, *pRight, *pLeft, *pUpLeft, *pUpRight, *pDownLeft, *pDownRight;
		} adjs;
	};
	#define	pUp			adjs.pUp
	#define	pDown		adjs.pDown
	#define	pLeft		adjs.pLeft
	#define	pRight		adjs.pRight
	#define	pUpLeft		adjs.pUpLeft
	#define	pUpRight	adjs.pUpRight
	#define	pDownLeft	adjs.pDownLeft
	#define	pDownRight	adjs.pDownRight

public:
	Node()
	{
		d = (unsigned int)0xFFF0;
		p = NULL;
		canHoldRouter = false;
		pRouter = NULL;	
		points = 0;
		theBest = NULL;
		bestPoints = 0;
	}
};

struct Link
{
	int  *value;
	Node *pNodes[2];
	Link *pNext;

	Link(Node * first, Node* second, int *val)
	{
		pNodes[0] = first;
		pNodes[1] = second;
		value = val;
		pNext = NULL;
	}
};

struct	LineSegment
{	
	// odkazy na koncove body
	Node	*pNodes[2];
	// odkaz na dalsi segment
	LineSegment *pNext;
	// normala tohoto segmentu
	double		fX, fY;	
	// smerovy vektor
	double		sx, sy;

	LineSegment(Node *first, Node* second)
	{
		int kX, kY, kZ;	
		double len;
		int vX, vY, vZ;

		pNodes[0] = first;
		pNodes[1] = second;
		pNext = NULL;

		vX = second->x - first->x;
		vY = second->y - first->y;
		vZ = 0;		

		/* vektor ktery je kolmy na vektor mezi routery a 0,0,1 */
		CrossProduct(&kX, &kY, &kZ, vX, vY, vZ, 0, 0, 1);
		
		/* normalizivat vektor */
		len = sqrt((double)(kX * kX + kY * kY + kZ * kZ));
		fX = kX / len;
		fY = kY / len;
		sx = vX / len;
		sy = vY / len;
	}

	LineSegment()
	{

	}
};

// radsi krizit linky
#define			ROUTER_EDGE_VALUE		20	
#define			LINE_EDGE_VALUE			29

class Network
{
	Node ** m_pGraph;
	Link *	m_pLinks;
	Link *  m_pLastLink;
	int	 ** m_edgeMatrix;

	int		graphSizeX, graphSizeY;

	int		GetEdgeIndex(int i, int j) { return i + j * graphSizeX; }
	void	GetNodeIndices(int edgeIndex, int &i, int &j) { i = edgeIndex % graphSizeX; j = (int)(edgeIndex / graphSizeX); }

	int		Relax(Node* u, Node*  v);
	int		Position(Node* u, Node* v);

public:
	void	BuildEmptyGraph(Configuration *conf);
	void	PlaceRouters(Configuration* conf);
	void	ConnectRouters(Configuration* conf); // disjkra

	Network(void);
	~Network(void);
};

#endif
