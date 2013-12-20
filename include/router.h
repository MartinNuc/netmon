
#ifndef ROUTER_H
#define ROUTER_H

/* router atr */
#define		XML_NAME			"name"
#define		XML_IP				"ip"
#define		XML_ROUTERURL		"routerurl"
#define		XML_APURL			"apurl"
#define		XML_AP2URL			"ap2url"
#define		XML_LANURL			"lanurl"
#define		XML_COMMENT			"comment"
#define		XML_POSITION		"position"
#define		XML_RTYPE			"rtype"
#define		XML_APTYPE			"accesspointtype"
#define		XML_ETH				"lansegment"
#define		XML_ESSID			"essid"
#define		XML_ESSID2			"essid2"
#define		XML_UNDERCONST		"underconst"
#define		XML_ROUTER_STATE	"routerstate"
#define		XML_INTERFACES		"interfaces"
#define		XML_INTERFACE		"interface"

/* router type */
#define		XML_RT				"routertype"
#define		XML_RT_ID			"rid"
#define		XML_RT_DIM			"dimension"
#define		XML_RT_COLOR		"color"
#define		XML_RT_INFO			"information"
#define		XML_RT_SWITCH		"switch"
#define		XML_RT_SHADOW		"shadow"

// information field
#define		RTI_NAME			"name"
#define		RTI_LOC				"comment"
#define		RTI_IP				"ip"
#define		RTI_BORDER			"border"

// delky retezcu
#define		MAX_NAME			20
#define		MAX_URL				50
#define		MAX_LOCATION		25
#define		MAX_IP				16		/* XXX:XXX:XXX:XXX = 15 + null char*/

enum ROUTER_STATE{	
	RS_OK		= 1,	/* Server is ok */
	RS_KO		= 2,	/* Server is down */
	RS_UNKNOWN	= 4,	/* Server is behind the server with RS_KO state */
	RS_INVALID	= 8		/* Server hs invalid params */
};
#define		MAX_STATES	4

// tohle se vyuziva pri prohledavani grafiku do sirky
enum NODE_STATE{	
	NS_FRESH = 0,
	NS_OPEN,
	NS_CLOSED,	
};



class	RouterType : public XMLObject
{
public:
	/* unique id */
	int		id;
	/* dim of the box */
	int		sx, sy;
	/* color */
	unsigned char	r, g, b;
	/* allocated color */
	int		color;
	/* information to show on the box */
	int		info;
	// stores order of name, comment and ip
	int		labels[3];
	int		labIndex;

	// Display shadow
	int		bShadow;
	
	/* true or false - router as switch is not pinged and line to the switch is permanently green */
	int		bSwitch;
	/* border params */
	int		iBorderWidth;
	int		iBorderColor;
	unsigned char	br, bg, bb;
	/* text colors */
	unsigned char	ipr, ipb, ipg; /* IP */
	char			ip_size;
	unsigned char	locr, locb, locg;/* location */
	char			loc_size;
	unsigned char	namr, namb, namg; /* name */
	char			name_size;
	int		iNameColor, iIPColor, iLocColor;
	/* next type */
	RouterType	*pNext;

public:
	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);
	//virtual void	PostParse(Configuration *conf);	// tato metoda bude volana po uspesnem parsingu

	RouterType();
	virtual ~RouterType();
}; 

class	Line;

class Interface : public XMLObject
{
public:
	char*		szName;
	char		szIP[MAX_IP];

	Interface	*pNext;

public:
	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);
	//virtual void	PostParse(Configuration *conf);	// tato metoda bude volana po uspesnem parsingu

	Interface();
	virtual ~Interface();
};

class Router : public XMLObject
{
public:
	/* static parameters */
	/* unique ID of the router */
	char			*szID;
	/* name of the router */
	char			*szName;
	/* information about the router */
	char			*szAPURL;
	char			*szAP2URL;
	char			*szRouterURL;
	char			*szLanURL;
	/* router's IP address */
	Interface		*pInterfaces;
	Interface		*pLastInt;
	int				iHasDefault;
	/* location in the real world */
	char			*szComment;
	char			*szESSID;
	char			*szESSID2;
	
	/* position on the map */
	int				iRealX, iRealY;
	int				iX, iY;
	int				iValid; // valid position
	/* AP and ETH mark */
	int				iAP;
	char			iEth, iUnderConst;	
	/* zatizeni AP */
	int				iAPLoad;
	int				iAP2Load;
	int				iLANLoad;
	/* router's type */
	int				rType;
	RouterType		*prType;
	Router			*pParent;

	/* dynamic parameters */
	/* state of the router */
	ROUTER_STATE	rState;
	NODE_STATE		nState;
	/* distance from the main router */
	int				iDistance;

	/* number of lines attached to this node */
	int				nLines;
	/* dynamic array of pointers to lines */
	Line			**parLines;

	// Ukazatel na uzel v grafu kde je tento router umisten
    Node			*pNode;

	Router			*pNext;

public:
	virtual void	Parse(char* tagName, const char *s, int len, Configuration *conf);
	virtual void	PostParse(Configuration *conf);	// tato metoda bude volana po uspesnem parsingu

	bool			Fly() { return (iX == -1 && iY == -1) ? true : false; }

	Router();
	virtual ~Router();
};

void ParseRouter(Configuration* conf, const char *s, int len);
void ParseRouterType(Configuration* conf, const char *s, int len);

#endif 
