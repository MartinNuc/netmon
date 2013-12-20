/* line atr */
#define		XML_LTYPE			"ltype"
#define		XML_LINEURL			"lineurl"
#define		XML_LTITLE			"ltitle"
#define		XML_LUNDERCONST		"underconst"
#define		XML_LPARALLEL		"parallel"

/* line type */
#define		XML_LT				"linetype"
#define		XML_LT_ID			"lid"
#define		XML_LT_WIDTH		"width"
#define		XML_LT_DESC			"linedesc"

class	Router;

class LineType : public XMLObject
{
public:
	/* unique id */
	int id;
	/* width in px */
	int	width;
	/* desc */
	char *desc;
	/* next type */
	LineType *pNext;

public:
	LineType();
	virtual ~LineType();

};


class Line : public XMLObject
{
public:
	int			lid;
	/* static parameters */
	/* IDs to line's routers - each line has only two routers */ 
	char		*szID1, *szID2;
	char		*szInt1, *szInt2;
	/* information about line */
	char		*szLineURL;
	char		*szLineTitle;	
	bool		bUnderConst;
	/* line bandwidth */
	int			iLoad;

	/* type of the line */
	LineType	*pLT;

	/* dynamic paremeters */	
	/* pointers to nodes */
	Router *pNode1, *pNode2;

	Line	*pNext;
	LineSegment	*pPath;
	LineSegment	*pLastPath;

	// Paralelni linka - prvni linka ma odkaz na paralelni linku
	Line	*pParallelLine;
	// druha linka ma v sobe informaci o tom ze je paralelni
	bool	bParallel;

public:
	Line();
	virtual ~Line();

	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);
	virtual void	PostParse(Configuration *conf);

	int				SegmentsCount();
	int				LineState();
	void			BaseColors(int &r, int &g, int &b);
	LineSegment*	GetSegment(int index, LineSegment *output);
};

void ParseLineType(struct _Configuration* conf, const char *s, int len);
void ParseLine(struct _Configuration* conf, const char *s, int len);
