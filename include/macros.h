#define	SAFE_DELETE(p)			if (p) { delete (p); p = NULL;}
#define	SAFE_DELETE_ARRAY(p)	if (p) { delete [](p); p = NULL;}

#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))
