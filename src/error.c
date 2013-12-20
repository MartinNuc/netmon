/*
 *	Error.h
 *	Error handling and error codes
 *	Author: Jiri Formanek
 *	Date: 29.8. 2004
 *
 */

#include <string.h>

#include "../include/error.h"

/* global object fo the error info */
ErrorInfo g_objErrorInfo;

/* sets last error message */
void	SetMyLastError(char *szMessage)
{
	strcpy(g_objErrorInfo.szMessage, szMessage);
	g_objErrorInfo.uiErrorCode = FAILURE; 
}

/* gets last error message structure */
char*	GetMyLastError() 
{ 
	return g_objErrorInfo.szMessage; 
}

