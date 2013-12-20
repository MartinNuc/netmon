/*
 *	Main program definitions
 *	Author: Jiri Formanek
 *	Date: 5.11. 2003
 */

#ifdef WIN32
	#include "../gdwin32/gd.h"
#else
	#include "gd.h"
#endif

#include "xmlparse.h"

/* netmon version */ 
#define VERSION "0.7.1"


/* strings */
#define		MAX_MAP				200
#define		MAX_PATH			0x1FF

#define		ERRORMSG_OK		"[   OK   ]\n"
#define		ERRORMSG_FAILED	"[ FAILED ]\n"

#define		S_TRUE				"true"
#define		S_FALSE				"false"


#define		DEFAULT_CONFIG_XML	"/opt/netmon/config.xml"
#define		DEFAULT_SOURCE_HTML	"source.html"
#define		DEFAULT_RES_HTML	"index.html"
#define		DEFAULT_RES_PNG		"result.png"

#define		XML_CODING			"UTF-8"

/* tags */
#define		XML_NET				"net"
#define		XML_OPTIONS			"configuration"
#define		XML_PATHS			"paths"
#define		XML_ROUTER_TYPES	"routertypes"
#define		XML_ROUTER_TYPE		"routertype"
#define		XML_LINE_TYPE		"linetype"
#define		XML_LINE_TYPES		"linetypes"
#define		XML_ROUTERS			"routers"
#define		XML_ROUTER			"router"
#define		XML_LINES			"lines"
#define		XML_LINE			"line"

/* line load tag */
#define		XML_LINELOAD		"lineload"
#define		XML_ROUTERLOAD		"routerload"
#define		XML_LOAD			"load"
#define		XML_APLOAD			"apload"
#define		XML_AP2LOAD			"ap2load"
#define		XML_LANLOAD			"lanload"


/* space between line and router */
#define		SPACE		0
enum LINE_STATE {	
	LS_OK		= 0,	/* Server is ok */
	LS_KO		,		/* Server is down */
	LS_UNKNOWN	,		/* Server is behind the server with RS_KO state */
	LS_INVALID			/* Server hs invalid params */
};
#define		MAX_STATES	4

enum TEXT_ALIGN {
	TA_CENTERED = 0,
	TA_TOPLEFT,
	TA_RIGHTBOTTOM
};


#define	PC_NONE			0x00
#define PC_ROUTER		0x01
#define PC_INTERFACE	0x04
#define PC_LINE			0x08
#define PC_LINE_TYPE	0x10
#define PC_ROUTER_TYPE	0x20
#define PC_OPTIONS		0x40
#define PC_PATHS		0x80
#define	PC_CLOUD		0x100

#define		RI_NAME		0x0001
#define		RI_IP		0x0002
#define		RI_COM		0x0004
#define		RI_BORDER	0x0010

/* posible fonts */
#define		UF_TINY			0
#define		UF_SMALL		-1
#define		UF_MEDIUM		-2
#define		UF_LARGE		-3
#define		UF_GIANT		-4

#define		UF_FONT_COUNT	5

#define		DEFAULT_NAME_FONT	10
#define		DEFAULT_LOC_FONT	8
#define		DEFAULT_IP_FONT		8

#define		MAP_EDGE			10

class Line;
class Interface;
class Router;
class RouterType;
class LineType;
class Network;
class Cloud;

class Configuration : public XMLObject
{
public:
	/* network configuration */
	struct NetworkParams
	{
		int				iRoutersCount;
		int				iAPsCount;
		int				iLANsCount;		
		int				iLinesCount;
		int				iLineSegments;
		int				iRTCount;
		int				iLTCount;
		int				iCloudsCount;

		/* pointer to the main router */
		Router		*pMainRouter;
		char		*szMainRouterID;

		// List of clouds
		Cloud		*pClouds;

		/* list of network objects */
		Router		*pRouters;
		Line		*pLines;	// seznam linek
		
		/* router types array */
		RouterType*	pRouterTypes;
		LineType*	pLineTypes;
	
		/* file's paths */
		char		*szResHTML;
		char		*szResPNG;
		char		*szSourcePath;
		char		*szLoadURL;	

		/* optimalizacni ukazatele pamatuji se posledni vytvorene objekty a nemusi se hledat */
		XMLObject	*lastSibling;
		XMLObject	*lastParent;

		/* parser object */
		XML_Parser	theParser;
		char		chNextTag[20];
		int			iParserContext;

		int			iWarnings;
		int			iErrors;		


		Network		*networkGraph;
		
	} np;

	/* ping parameters */
	struct	Ping
	{
		/* ping options - got from config */
		int		iMaxAttempt;
		int		iTimeout;
	} ping;

	/*
	 * Structure which contains information about GD graphic system
	 * Input pattern and output file etc.
	 */
	struct GDInfo {

		/* temporary picture */
		gdImagePtr	map; 
		gdImagePtr	ap; 
		gdImagePtr	ap2; 
		gdImagePtr	ca; 

		int			colRouter;
		int			colTitle;
		int			colText;
		int			colWhite;
		int			colBlack;

		/* size of result picture */
		int			sx, sy;	
		char		*szFontPath;
		char		*szAPPath;
		char		*szAP2Path;
		char		*szLANPath;

		// rozmery a pocet bunek
		int			iCellWidth;
		int			iCellHeight;
		int			iHorizontalCells;
		int			iVerticalCells;

		// matice routeru na mape
		Router		***pppRouterMatrix;
	} gd;

	int		iVerbose; /* disable output */	
	int		iSurpressLines;	
	char	configFileName[MAX_PATH];

public:
	Configuration();
	virtual ~Configuration();

	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);
};

/* Functions protypes for network */
int		ParseNetwork(char	*chConfigFile, Configuration *conf);
int		BreadthFirstSearch(Configuration * conf);


struct LineSegment;
/* Functions protypes for graphics */
int		InitGD(Configuration* conf);
int		DestroyGD(Configuration* conf);
void	DrawRouter(Router * pRouter, Configuration* conf);
void	DrawRouterShadow(Router * pRouter, Configuration* conf);
int		DrawNetwork(Configuration* conf);
void	DrawLine(Line * pLine, Configuration* conf);
void	GetLinePoints(Line *pLine, LineSegment *prevSeg, LineSegment *segment,  Configuration* conf, int &x1, int &y1, int &x2, int &y2);
int		CheckPictures(Configuration *conf);

#define		LINE_TITLE_FORMAT1		"%s (%d%%)"
#define		LINE_TITLE_FORMAT2		"%s"

#define		LOAD_PER_FORMAT			" (%d%%)"

#define		AREA_LINE_FORMAT		"<area shape=poly coords=\"%d,%d,%d,%d,%d,%d,%d,%d\" href=\"%s\" alt=\"%s\" title=\"%s\">\n"
#define		AREA_ROUTER_FORMAT		"<area shape=rect coords=\"%d,%d,%d,%d\" href=\"%s\">\n"
#define		E_AREA_LINE_FORMAT		"<area shape=poly coords=\"%d,%d,%d,%d,%d,%d,%d,%d\" nohref alt=\"%s\" title=\"%s\">\n"
#define		E_AREA_ROUTER_FORMAT	"<area shape=rect coords=\"%d,%d,%d,%d\" nohref>\n"

#define		S_SPACE					" "
#define		S_QUOTE					"\""
#define		AREA_CIRC_BEGIN			"<area shape=circ coords=\"%d,%d,%d\""
#define		AREA_RECT_BEGIN			"<area shape=rect coords=\"%d,%d,%d,%d\""
#define		AREA_NOHREF				"nohref"
#define		AREA_HREF				"href=\"%s\""
#define		AREA_TITLE				"title="
#define		AREA_ALT				"alt="
#define		AREA_AP					"Access point"
#define		AREA_ETH				"Ethernet segment"
#define		AREA_END				">"

#define		DATE_FORMAT				"%02d:%02d %d.%d.%d"

#define		TEMPLATE_DATE			"%%DATE%%"
#define		TEMPLATE_SRCPIC			"%%SRCPIC%%"
#define		TEMPLATE_MAINROUTER		"%%MAINROUTER%%"
#define		TEMPLATE_AREAMAP		"%%AREA_MAP%%"
#define		TEMPLATE_VERSION		"%%VERSION%%"

/* generate HTML page */
int		GenerateHTML(char *szSrcHTML, Configuration* conf);

/* net functions */
int ping(char	*szIP, int iCount, int iTimeout);

/* 
 *	Parser handlers
 *	- for start tags (and attributes)
 *	- for end tags
 *	- for data between tags
 */
void	startElementHandler(void *userData, const char *name, const char **atts);
void	endElementHandler(void *userData, const char *name);
void	dataHandler(void *userData, const char *s, int len);
void	ExtractString(int iOrder, const char* chInput, char	* chOutput, char chSeparator);
int		CountReplacedString(char *szDest, char *szToReplace);
int		GetReplacedString(char *szDest);
int		GenerateResult(char *szDest, char *szFormat, Configuration *conf, char* szMap);

/* methods and types for CMD parser */
#define CC_NONE	0
#define CC_STR	1
#define CC_NUM	2

struct t_param 
{
	char *jmeno;
	int typ;
	void *promenna;
};

/* Parses input cmd line */
int ParseCmdLine(int argc, char *argv[], struct t_param *params);
int	ParseLineLoads(char	*chLoadFile, Configuration* conf);
long LoadFileIntoBuffer(char *szFileName, char* chBuffer);
int mySavePng(char *filename,   gdImagePtr im);
int	GetStringWidth(char *szString, char* font, int size);
int	GetStringHeight(char *szString, char* font, int size);
int	DrawString(gdImagePtr image, char *szString, int x, int y, char* font, int size, int color, TEXT_ALIGN align);
void DrawDashedLine(Configuration* conf, int color, int width, int x1, int y1, int x2, int y2, double fX, double fY);
void	NormalizeString(char *szDest, int destLen, char *szSrc, int srcLen);
char*	ConvertStringToUppercase(char* in);
char *StringReverse(char *str);

/* computes cross product of two vectors */
void CrossProduct(int *returnValueX, int *returnValueY, int *returnValueZ, 
				  int leftX, int leftY, int leftZ, 
				  int rightX, int rightY, int rightZ);
void CrossProduct(double *returnValueX, double *returnValueY, double *returnValueZ, 
				  double leftX, double leftY, double leftZ, 
				  double rightX, double rightY, double rightZ);
