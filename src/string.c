/* 
 * Some strings functions
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
#include <stdarg.h>
#include <ctype.h>

#include "../include/error.h"
#include "../include/macros.h"
#include "../include/xmlobject.h"
#include "../include/network.h"
#include "../include/graph.h"
#include "../include/router.h"

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
 *	Extract substring from the string separated by space
 *
 *	Example:	iOrder = 0 chInput = "Fenner XSpec"
 *				Function returns: Fenner
 *				In case iOrder = 1 (same input string)
 *				Function returns: XSpec
 *
 *	Returned value is a string chOutput
 */
void	ExtractString(int iOrder, const char* chInput, char	* chOutput, char chSeparator)
{
	int i = 0;
	char *chSpace,  *chTemp,  *chToDelete;
	/* create copy of input string */
	int l = (int)strlen(chInput);
	/* create temporary pointer to delete this allocated array */
	chToDelete = chTemp = (char*) malloc((l + 1) * sizeof(char));
	strcpy(chTemp, chInput);
	/* find spaces and return valid router name */
	if(chOutput) {
		for(i = 0; i < iOrder; i++) {
			char * chSpace = strchr(chTemp, chSeparator);				
			if(chSpace) {
				/* replace space */				
				chTemp = ++chSpace;				
			}	
			else
			{
				chOutput[0] = '\0';
				return;
			}	
		}
		chSpace = strchr(chTemp, chSeparator);
		if(chSpace) {			
			*chSpace = '\0';
		}			
		strcpy(chOutput, chTemp);		
	}	
	free(chToDelete);
}


int	CountReplacedString(char *szDest, char *szToReplace)
{
	int iToRepLen, iCount = 0;
	char *cPos;
	/* check input parameters */
	if(!(szDest && szToReplace))
	{
		return -1;
	}
	/* get lens of all input strings */
	iToRepLen = (int)strlen(szToReplace);
	/*iRepByLen = (int)strlen(szReplaceBy);*/
	/* find desired string */
	while((cPos = strstr(szDest, szToReplace)) != 0)
	{			
		/* jump over found string */
		szDest = cPos + iToRepLen;
		/* iCount is s number of found and replaced strings */
		iCount++;
		/* find next string */
	};
	return iCount;
}


/*
 * Replaces ;amp string with ampersand
 */
void	NormalizeString(char *szDest, int destLen, char *szSrc, int srcLen)
{	
	int j, i;
	for(j = 0, i = 0; i < destLen; i++, j++)
	{
		if(j >= srcLen)	
			break;

		if(szSrc[j+0] != 0 && szSrc[j+0] == ';')		
			if(szSrc[j+1] != 0 && szSrc[j+1] == 'a')
				if(szSrc[j+2] != 0 && szSrc[j+2] == 'm')
					if(szSrc[j+3] != 0 && szSrc[j+3] == 'p')
					{
						j += 3;			
						szDest[i] = '&';
						continue;
					}		

		szDest[i] = szSrc[j];		
	}	
	szDest[i] = 0;
}

/* 
 *	Find first occurence of the string to replace and replace it by %s.
 *	Returns type of the replaced string.
 */
int GetReplacedString(char *szDest)
{
	int i, iType = 0, iLen;
	char *cTemp, *cStart = NULL;
	char ch, tag[20];
	/* check input parameters */
	if(!(szDest))
	{
		return -1;
	}
	cTemp = szDest;

	do {
		ch = *cTemp; 
		iLen = 0;
		/* check begining tag % */
		if(ch == '%')
		{
			/* check once more % */
			ch = *(cTemp + 1);
			if(ch == '%')
			{		
				/* save start position for replace and jump over first % */
				cStart = cTemp;
				/* copy next tag to tag char */
				for(i = 0; i < 15; i++)
				{
					/* check once more % */
					ch = *(cTemp + 1);

					/* check ending tag % */
					if(*cTemp == '%' && ch == '%' && cStart != cTemp)
					{						
						break;						
					}
					/* copy char and count len */
					iLen++;
					tag[i] = *cTemp++;									
				}
				/* end tag string; add two % chars and null char */
				tag[i++] = '%'; iLen++;
				tag[i++] = '%'; iLen++;
				tag[i] = '\0';
				/* check tag string */
				if(!strcmp(tag, TEMPLATE_DATE))
				{
					iType = 1;
					break;
				}
				/* check tag string */
				if(!strcmp(tag, TEMPLATE_VERSION))
				{
					iType = 2;
					break;
				}
				/* check tag string */
				if(!strcmp(tag, TEMPLATE_MAINROUTER))
				{
					iType = 3;
					break;
				}
				/* check tag string */
				if(!strcmp(tag, TEMPLATE_AREAMAP))
				{
					iType = 4;
					break;
				}
				/* check tag string */
				if(!strcmp(tag, TEMPLATE_SRCPIC))
				{
					iType = 5;
					break;
				}
			}
		}
		/* next character */
		cTemp++;
	}
	while(ch != '\0');

	/* replace string by %s */
	if(iType != 0 && cStart != NULL)
	{
		strcpy(tag, "%s\0");
		for(i = 0; i < iLen; i++)
		{
			if(i < 2)
			{
				*cStart++ = tag[i];
			}
			else
			{
				*cStart++ = ' ';			
			}
		}
	}

    return iType;
}

int	GenerateResult(char *szDest, char *szFormat, Configuration *conf, char* szMap)
{
	char	szDate[100];	
	va_list  *szTemp;
	time_t timer;
	struct tm *tblock;
	static int iTotalLen = 0;	
	int iD = 0;
	int i, iType;
	int iCount = 0;  
	
	/* generate time string */
	timer = time(NULL);
	tblock = localtime(&timer);
	sprintf(szDate, DATE_FORMAT, tblock->tm_hour,tblock->tm_min, tblock->tm_mday, tblock->tm_mon+1, tblock->tm_year+1900);
	/* return only len if user set szDest as NULL */
	if(!szDest)
	{
		/* check input parameters */
		if(szFormat)
		{
			iTotalLen = (int)strlen(szFormat);	
		}
		else
		{
			return -1;
		}
		/* find all occurences and prepare format to sprintf */
		iD = CountReplacedString(szFormat, TEMPLATE_DATE);
		for(i = 0; i < iD; i++)
		{
			iTotalLen += (int)strlen(szDate) - 2;
		}
		iD = CountReplacedString(szFormat, TEMPLATE_SRCPIC);
		for(i = 0; i < iD; i++)
		{
			iTotalLen += (int)strlen(conf->np.szResPNG) - 2;
		}
		iD = CountReplacedString(szFormat, TEMPLATE_AREAMAP);
		for(i = 0; i < iD; i++)
		{
			iTotalLen += (int)strlen(szMap) - 2;
		}
		iD = CountReplacedString(szFormat, TEMPLATE_MAINROUTER);
		for(i = 0; i < iD; i++)
		{
			iTotalLen += (int)strlen(conf->np.pMainRouter->szName) - 2;
		}
		iD = CountReplacedString(szFormat, TEMPLATE_VERSION);
		for(i = 0; i < iD; i++)
		{
			iTotalLen += (int)strlen(VERSION) - 2;
		}		
		return iTotalLen;
	}
	else
	{	
		/* compute number of string to replace */
		iCount += CountReplacedString(szFormat, TEMPLATE_VERSION);
		iCount += CountReplacedString(szFormat, TEMPLATE_AREAMAP);
		iCount += CountReplacedString(szFormat, TEMPLATE_MAINROUTER);
		iCount += CountReplacedString(szFormat, TEMPLATE_SRCPIC);
		iCount += CountReplacedString(szFormat, TEMPLATE_DATE);
		/* allocate space for pointer to arguments */
		szTemp = (char**) malloc(iCount * sizeof(char*));
		/* find all strings to replace and replace them and save the right argument pointer */
		for(i = 0; i < iCount; i++)
		{
			iType = GetReplacedString(szFormat);
			switch(iType)
			{
				/* date */
				case 1:
					szTemp[i] = szDate;
					break;
				case 2:
					szTemp[i] = VERSION;
					break;
				case 3:
					szTemp[i] = conf->np.pMainRouter->szName;
					break;
				case 4:
					szTemp[i] = szMap;
					break;
				case 5:
					szTemp[i] = conf->np.szResPNG;
					break;
			}
		}
		/* write all string into main buffer */
		iD = vsprintf(szDest, szFormat, (va_list)szTemp);

		/* release all buffers */
		free(szTemp);
	}

	return 0;
}



/*
 *	Loads file into memory buffer
 *
 *	If chBuffer is NULL, function returns only size of required buffer
 *	to allocate outside the function and recall it once more with
 *	the user buffer.
 */

long	LoadFileIntoBuffer(char *szFileName, char* chBuffer)
{
	FILE*	file;
	long iCount;
	size_t readen;

	file = fopen(szFileName, "rb");
	if(!file) 
	{
		printf("  Error: Cannot open '%s' file.", szFileName);
		return FAILURE;
	}

	/* get length of file */
	if(fseek(file, 0L, SEEK_END) != 0)
	{
		SetMyLastError("  Error during: fseek(file, 0L, SEEK_END)\n");
		return FAILURE;
	}
	/* Load file to buffer */
	iCount = ftell(file);
	if(iCount == 1L)
	{
		SetMyLastError("  Error during: ftell(file);\n");
		return FAILURE;
	}
	if(fseek(file, 0L, SEEK_SET) != 0)	
	{
		SetMyLastError("  Error during: fseek(file, 0L, SEEK_SET)\n");
		return FAILURE;
	}		

	if(chBuffer)	
	{
		/* read whole file to buffer */	
		readen = fread(chBuffer, sizeof(char), iCount, file);	
		if(readen != (size_t)iCount) 
		{
			free(chBuffer);	
			SetMyLastError("  Error: Cannot read whole file into memory.\n");
			return FAILURE;
		}

	}
	/* Close the file */
	if(fclose(file) != 0)
	{		
		SetMyLastError("  Error: Cannot close file.\n");
		return FAILURE;
	}
	return iCount;
}


int mySavePng(char *filename,   gdImagePtr im)
{
  FILE *out;
  int size;
  int iRet = OK;
  char *data = NULL;

  out = fopen(filename, "wb");
  if (!out) {
    /* Error */
	  printf("  Error: Cannot open destination image '%s' for writing.\n", filename);
	  iRet = FAILURE;
  }
  else
  {
	data = (char *) gdImagePngPtr(im, &size);
	if (!data) {
		printf("  Error: Cannot read data from gdImagePng structure.\n");
		iRet = FAILURE;
		/* Error */
	}
	else
	{
		if (fwrite(data, 1, size, out) != (size_t)size) {
			/* Error */
			printf("  Error: Cannot write image into file.\n");
			iRet = FAILURE;
		}
		gdFree(data);
	}
	if (fclose(out) != 0) {
		/* Error */
		printf("  Error: Cannot close image.\n");
		iRet = FAILURE;
	}
	if(iRet != OK)
	{
		printf("\n");
	}	
  }   
  return iRet;
}

int	DrawString(gdImagePtr image, char *szString, int x, int y, char* font, int size, int color, TEXT_ALIGN align)
{
	int brect[8];
	gdFontPtr	pFont;
	char* err;
	int offsetX = 0, offsetY = 0;

	if(image)
	{
		/* use TTF */
		if(font && strlen(font) != 0 && size > 0)
		{
			err = gdImageStringFT(NULL, brect, 0, font,size,0.,0,0,szString);
			if (err) 
			{
				/* if error occur, use native font */				
				return DrawString(image, szString, x, y, "", size, color, align);				
			}
			switch(align) {
				case TA_CENTERED:
					offsetX = -(brect[2]-brect[0])/2;
					offsetY = +(brect[3]-brect[5])/2;
					break;
				case TA_TOPLEFT:
					offsetX = 0;
					offsetY = (brect[3]-brect[5]);
					break;
				case TA_RIGHTBOTTOM:
					offsetX = -(brect[2]-brect[0]);
					offsetY = 0;
					break;
			}

			err = gdImageStringFT(	image,brect,
									color,font, size, 0.,
									x+offsetX,
									y+offsetY,
									szString);

			if (err) 
			{
				/* if error occur, use native font */				
				return DrawString(image, szString, x, y, "", size, color, align);				
			}
		}
		/* use native GD fonts */
		else
		{
			/* negative size */
			switch(size) {
				case UF_TINY:
					pFont = gdFontGetTiny();
					break;
				case UF_SMALL:
					pFont = gdFontGetSmall();
					break;
				case UF_MEDIUM:
					pFont = gdFontGetMediumBold();
					break;
				case UF_LARGE:
					pFont = gdFontGetLarge();
					break;
				case UF_GIANT:
					pFont = gdFontGetGiant();
					break;			
				default:
					pFont = gdFontGetMediumBold();
					break;
			}
			/* positive size */
			if(size > 0)
			{
				if(size > 9)
				{
					pFont = gdFontGetGiant();
				}
				else
				{
					pFont = gdFontGetSmall();
				}
			}
			switch(align) {
				case TA_CENTERED:
					offsetX = -(int)(strlen(szString) * pFont->w)/2;
					offsetY = (int)-pFont->h/2;
					break;
				case TA_TOPLEFT:
					offsetX = offsetY = 0;
					break;
				case TA_RIGHTBOTTOM:
					offsetX = -(int)(strlen(szString) * pFont->w);
					offsetY = -pFont->h;
					break;
			}
			gdImageString(	image, pFont, 
							x+offsetX, 
							y+offsetY, 
							(unsigned char*)szString, color);
		}

		return OK;
	}
	return FAILURE;
}

int	GetStringWidth(char *szString, char* font, int size)
{
	int brect[8];
	char* err;
	gdFontPtr	pFont;

	if(strlen(font) != 0 && size > 0)
	{
		err = gdImageStringFT(NULL, brect, 0, font,size,0.,0,0,szString);
		if (err) 
		{
			/* if error occur, use native font */				
			return GetStringWidth(szString, "", size);			
		}
		return brect[2]-brect[0];	
	}
	else
	{		
		/* negative size */
		switch(size) {
			case UF_TINY:
				pFont = gdFontGetTiny();
				break;
			case UF_SMALL:
				pFont = gdFontGetSmall();
				break;
			case UF_MEDIUM:
				pFont = gdFontGetMediumBold();
				break;
			case UF_LARGE:
				pFont = gdFontGetLarge();
				break;
			case UF_GIANT:
				pFont = gdFontGetGiant();
				break;	
			default:
				pFont = gdFontGetMediumBold();
				break;
		}
		/* positive size */
		if(size > 0)
		{
			if(size > 9)
			{
				pFont = gdFontGetGiant();
			}
			else
			{
				pFont = gdFontGetSmall();
			}
		}
		return (int)strlen(szString) * pFont->w;
	}	
}

int	GetStringHeight(char *szString, char* font, int size)
{
	int brect[8];
	char* err;
	gdFontPtr	pFont;

	if(font && strlen(font) != 0 && size > 0)
	{
		err = gdImageStringFT(NULL, brect, 0, font,size,0.,0,0,szString);
		if (err) 
		{
			/* if error occur, use native font */				
			return GetStringHeight(szString, "", size);	
		}
		return brect[3]-brect[5];		
	}
	else
	{		
		/* negative size */
		switch(size) {
			case UF_TINY:
				pFont = gdFontGetTiny();
				break;
			case UF_SMALL:
				pFont = gdFontGetSmall();
				break;
			case UF_MEDIUM:
				pFont = gdFontGetMediumBold();
				break;
			case UF_LARGE:
				pFont = gdFontGetLarge();
				break;
			case UF_GIANT:
				pFont = gdFontGetGiant();
				break;		
			default:
				pFont = gdFontGetMediumBold();
				break;
		}
		/* positive size */
		if(size > 0)
		{
			if(size > 9)
			{
				pFont = gdFontGetGiant();
			}
			else
			{
				pFont = gdFontGetSmall();
			}
		}
		return pFont->h;
	}	
	
}

char*	ConvertStringToUppercase(char* in)
{
	int len = (int)strlen(in);
	for(int i = 0; i < len; i++)
	{
		in[i] = (char)toupper(in[i]);
	}
	return in;
}

char *StringReverse(char *str)
{
      char *p1, *p2;
      if (!str || !*str)
         return str;

      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}
