
#ifdef WIN32
/* Funkce checksum je opsana z prislusnych RFC dokumentu. */
unsigned short checksum(unsigned char *addr, int count)
{
  register long sum = 0;
  
  while( count > 1 )  
  {
    /*  This is the inner loop */
    sum += *((unsigned short*)addr);
	addr+=2;
    count -= 2;
  }  
  /*  Add left-over byte, if any */
  if( count > 0 )
    sum += * (unsigned char *) addr;
  /*  Fold 32-bit sum to 16 bits */
  while (sum>>16)
    sum = (sum & 0xffff) + (sum >> 16);
  return (unsigned short)~sum;
}

#endif

/* linux ping! */
#ifndef WIN32
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <stdlib.h>
	#include <netinet/ip.h>
	#include <netinet/ip_icmp.h>
	#include <unistd.h>
	#include <string.h>
    #include "../include/error.h"
	#include "../include/ping.h"
	#include "../include/xmlobject.h"	
	#include "../include/network.h"	

	int ping(char	*szIP, int iCount, int iTimeout)
	{
		int i, pingres = 0;
		for(i = 0; i < iCount; i++)
		{
			pingres = pingthost(szIP, iTimeout/1000);
			if(pingres > 0)
			{
				return pingres;
			}			
		}
		/* Socket error */
		if(pingres == -2)
		{			
			return SOCKETFAILED;
		}
		if(pingres <= 0)
		{
			return FAILURE;
		}
		return FAILURE;
	}
#endif

/* windows ping */
#ifdef WIN32
	#include <Winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>	
	#include "../include/error.h"
	#include "../include/ip_icmp.h"
	#include "../include/iphdr.h"


	#include <time.h>
	#include "../include/macros.h"
	#include "../include/xmlobject.h"
	#include "../include/network.h"
	#define BUFSIZE 1024

	int ping(char	*szIP, int iCount, int iTimeout)
	{
	#ifdef WIN32
		WORD wVersionRequested = MAKEWORD(2,2); 
		WSADATA wsaData;  	
		unsigned short int pid = (unsigned short int)GetCurrentProcessId();
	#else
		unsigned short int pid = (unsigned short int)getpid();	
	#endif

		struct hostent *host; 
		struct icmphdr *icmp, *icmpRecv;
		struct iphdr *ip;  
		int lenght, size, sock;
		unsigned int ttl;
		struct sockaddr_in sendSockAddr, receiveSockAddr;
		char buffer[BUFSIZE];
		fd_set mySet;
		struct timeval tv;
		char *addrString;
		unsigned short p;
		clock_t dwTime, dwNewTime, time;
		clock_t dwTotal = 0;
		int iRet = FAILURE;
		int iExit = 0;	
		int localTO = iTimeout;

		//return FAILURE;
	#ifdef WIN32
		if (WSAStartup(wVersionRequested, &wsaData) != 0)
		{
			//printf("Nelze inicializovat sokety");
			return WSAFAILED;
		}
	#endif
		/* Zjistime info o vzdalenem pocitaci */
		if ((host = gethostbyname(szIP)) == NULL)
		{
			//printf("Spatna adresa\n");
			return INVALIDIP;
		}
	#ifdef WIN32
		/* Vytvorim soket */
		if ((sock = (int)socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == INVALID_SOCKET)
		{
			//printf("Nelze vytvorit socket.\n");
			return SOCKETFAILED;
		}
	#else
		/* Vytvorim soket */
		if ((sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
		{
			//printf("Nelze vytvorit socket.\n");
			return SOCKETFAILED;
		}
	#endif
		ttl = 255;
		setsockopt(sock, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl));
		/* Zaplnime polozky ICMP hlavicky */
		icmp = (struct icmphdr *)malloc(sizeof( struct icmphdr));
		icmp->type = ICMP_ECHO;
		icmp->code = 0;
		icmp->un.echo.id = pid;
		/* Pripravime sendto */
		sendSockAddr.sin_family = AF_INET;
		sendSockAddr.sin_port = 0;
		memcpy(&(sendSockAddr.sin_addr), host->h_addr, host->h_length);
		for (p = 1; p <= iCount; p++)
		{				
			if(iExit) break;
			icmp->checksum = 0;
			icmp->un.echo.sequence = p;
			icmp->checksum = checksum((unsigned char *)icmp, sizeof(struct icmphdr));
			sendto(sock, (char *)icmp, sizeof(struct icmphdr), 0, (struct sockaddr *)&sendSockAddr, sizeof(struct sockaddr));
			dwTime = GetTickCount();	

			printf(".");
			localTO = iTimeout;

			//do
		//	{
				FD_ZERO(&mySet);
				FD_SET(sock, &mySet);
				time = GetTickCount();
				tv.tv_sec = localTO / 1000;
				tv.tv_usec = localTO % 1000;
				/* wait for the event on the socket */
				iRet = select(0, &mySet, NULL, NULL, &tv);
				if(iRet < 0)
				{
                    //printf("Error occurred when the program was waiting for reply.\n");
					iRet = SOCKETFAILED;
					break;
				}
				if(iRet == 0)
				{            
					//printf("Request time out.\n");
					iRet = FAILURE;
					//break;
				}	  
				/* are there any data on the socket? */
				if(FD_ISSET(sock, &mySet))
				{
						size = sizeof(struct sockaddr_in);
						if ((lenght = recvfrom(sock, buffer, BUFSIZE, 0, (struct sockaddr *)&receiveSockAddr, &size)) == -1)
						{
							printf("Problem pri prijimani dat.");
						}
						dwNewTime = GetTickCount();
						dwTotal += (dwNewTime - dwTime);
						/* Prisel blok dat IP hlavicka + ICMP hlavicka. */ 
						ip = (struct iphdr *) buffer;
						icmpRecv = (struct icmphdr *) (buffer + ip->ihl * 4);
						if ((icmpRecv->un.echo.id == pid) && (icmpRecv->type == ICMP_ECHOREPLY) && (icmpRecv->un.echo.sequence == p))
						{
							addrString = strdup(inet_ntoa(receiveSockAddr.sin_addr));                
							//printf("Reply from %s -> icmp_seq=%d, time=%dms\n", addrString, icmpRecv->un.echo.sequence, dwNewTime - dwTime);
							free(addrString);
							iRet = (int)dwTotal / iCount + 1;
							iExit = 1; /* end of pinging */
							break;
						}
						else
						{
							//printf("%d - %d = %d\n", GetTickCount(), time, GetTickCount() - time);
							localTO -= (GetTickCount() - time);							

							if(localTO <= 0) break;
						}
				}				
			//} while(localTO > 0);
		}
		
	#ifdef WIN32
		closesocket(sock);
		WSACleanup(); 
	#else
		close(sock);
	#endif

		free(icmp);

		return iRet; 
	}
#endif 
