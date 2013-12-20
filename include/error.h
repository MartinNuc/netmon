/*
 *	Error.h
 *	Error handling and error codes
 *	Author: Jiri Formanek
 *	Date: 29.8. 2004
 *
 */

#ifndef ERROR_H
#define ERROR_H

/* size of the error message */
#define		ERROR_MESSAGE_SIZE	200

/* error information structure holds error message and code */
typedef struct _ErrorInfo
{
	unsigned int	uiErrorCode;
	char			szMessage[ERROR_MESSAGE_SIZE];	

} ErrorInfo;


/* global object of the error info */
extern ErrorInfo g_objErrorInfo;

/* SetLastError() sets last error message  */
void	SetMyLastError(char *szMessage);

/* GetLastError() returns info about last error */
char*	GetMyLastError();

/* error codes */
#define	OK						(unsigned int) 0
#define FAILURE					(unsigned int)-1
#define UNKNOWN					(unsigned int)-2 
#define	INVALIDIP				(unsigned int)-3
#define	WSAFAILED				(unsigned int)-4
#define	SOCKETFAILED			(unsigned int)-5
#define	INVALID_PARAMETER		(unsigned int)-6
#define	INVALID_PARAMETER_NAME	(unsigned int)-7

#endif /* ERROR_H */
