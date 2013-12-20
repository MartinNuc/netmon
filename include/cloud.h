/* cloud atr */
#define		XML_CLOUDS			"clouds"
#define		XML_CLOUD			"cloud"
#define		XML_CBORDER			"cborder"
#define		XML_CCOLOR			"ccolor"
#define		XML_CEDGE			"cedge"


#define		CLOUD_EDGE	7

struct RouterList
{
	Router	*pRouter;
	RouterList *pNext;
};

struct ClNode
{
	gdPoint pos;	// poloha
	int		level;	// stupen uzlu
    int		dist[4];
	ClNode*	adj[4];
	bool	open;
};

#define		CD_UP		0
#define		CD_DOWN		2
#define		CD_RIGHT	1
#define		CD_LEFT		3


class Cloud :	public XMLObject
{
public:
	// Border
	bool	border;
	int		borColor[3];

	// Telo
	int		cldColor[3];
	int		iEdge;

	// Pole routeru patrici tomuto cloudu
	RouterList*		pRouters;
	RouterList*		pLastRouter;
	Cloud*			pNext;

public:
	Cloud(void);
	virtual ~Cloud(void);

	// Parent methods
	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);
	virtual void	PostParse(Configuration *conf);

	// Own methods
	void	DrawCloud(Configuration* conf);
	void	AddRouter(Router*	pRouter);

	static int	Dist(ClNode & n1, ClNode & n2);
};
