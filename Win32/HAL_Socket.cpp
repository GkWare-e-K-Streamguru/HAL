// ********************************************************
//
//   Author/Copyright 	Gero Kuehn / GkWare e.K.
//						Hatzper Strasse 172B
//						D-45149 Essen, Germany
//						Tel: +49 174 520 8026
//						Email: support@gkware.com
//						Web: http://www.gkware.com
//
//
// ********************************************************

#pragma warning(disable:4710) // WSPiApi.h causes warnings in the browser project
#pragma warning(disable:4702)
#define _WINSOCKAPI_
#include <WinSock2.h>
#include "Ws2tcpip.h"
#include "all.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <conio.h>
#pragma warning(default:4702)

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
static void NETAPIAddrToWinsockAddr(const NETAPI_sockaddr_in *pAddr1, SOCKADDR_STORAGE *pAddr2)
{
	memset(pAddr2, 0x00, sizeof(SOCKADDR_STORAGE));
	if(pAddr1->sin_family == AF_INET) {
		struct sockaddr_in *p4 = (struct sockaddr_in *)pAddr2;
		p4->sin_addr.S_un.S_addr = pAddr1->sin_addr.S_un.S_addr;
		p4->sin_family = pAddr1->sin_family;
		p4->sin_port = pAddr1->sin_port;
	} else if(pAddr1->sin_family == AF_INET6) {
		struct sockaddr_in6 *p6 = (struct sockaddr_in6 *)pAddr2;
		memcpy(&p6->sin6_addr , pAddr1->sin_addr.S_un.bIPv6Data, sizeof(p6->sin6_addr));
		p6->sin6_flowinfo = 0;
		p6->sin6_scope_id = 0;
		p6->sin6_family = pAddr1->sin_family;
		p6->sin6_port = pAddr1->sin_port;
	} else {
		assert(0);
	}
}

static void WinsockAddrToNETAPIAddr(const SOCKADDR_STORAGE *pAddr1, NETAPI_sockaddr_in *pAddr2)
{
	memset(pAddr2, 0x00, sizeof(struct NETAPI_sockaddr_in));
	if(pAddr1->ss_family == AF_INET) {
		struct sockaddr_in *p4 = (struct sockaddr_in *)pAddr1;
		pAddr2->sin_addr.S_un.S_addr = p4->sin_addr.S_un.S_addr;
		pAddr2->sin_family = p4->sin_family;
		pAddr2->sin_port = p4->sin_port;	
	} else if(pAddr1->ss_family == AF_INET6) {
		struct sockaddr_in6 *p6 = (struct sockaddr_in6 *)pAddr1;
		memcpy(pAddr2->sin_addr.S_un.bIPv6Data, &p6->sin6_addr, sizeof(p6->sin6_addr));
		assert(p6->sin6_flowinfo == 0);
		assert(p6->sin6_scope_id == 0);
		pAddr2->sin_family = p6->sin6_family;
		pAddr2->sin_port = p6->sin6_port;	
	} else {
		assert(0);
	}
}

BOOL NETAPI_Init(const BYTE *bMAC)
{
	UNUSED_PARAMETER(bMAC);
	WSADATA wsa;
	WSAStartup(0x0202,&wsa);
	return TRUE;
}

NETAPI_SOCKET NETAPI_socket(int af, int type, int protocol)
{
	UNUSED_PARAMETER(protocol);
	return socket(af, type, 0/*protocol*/);
}

int NETAPI_connect(NETAPI_SOCKET s, const NETAPI_sockaddr_in *name)
{
	SOCKADDR_STORAGE addr;
 	int ret;
	NETAPIAddrToWinsockAddr(name, &addr);
	ret = connect(s,(struct sockaddr *)&addr,sizeof(addr));
	return ret;
}

int NETAPI_listen(NETAPI_SOCKET s, int backlog)
{
	return listen(s, backlog);
}

NETAPI_SOCKET NETAPI_accept(NETAPI_SOCKET s, NETAPI_sockaddr_in *addr)
{
	NETAPI_SOCKET s2;
	SOCKADDR_STORAGE addr2;
	int addrlen = sizeof(addr2);
	s2 = accept(s, (struct sockaddr *) &addr2, &addrlen);
	if((s2 != -1) && addr) {
		WinsockAddrToNETAPIAddr(&addr2, addr);
	}
	return s2;
}

int NETAPI_bind(NETAPI_SOCKET s, const NETAPI_sockaddr_in *pAddr)
{
	SOCKADDR_STORAGE addr;
	NETAPIAddrToWinsockAddr(pAddr, &addr);
	return bind(s, (struct sockaddr *)&addr,sizeof(addr));
}

int NETAPI_send(NETAPI_SOCKET s, const void *buf, int len)
{
	return send(s, (const char*)buf, len, 0);
}

int NETAPI_sendto(NETAPI_SOCKET s, const void *buf, int len, const NETAPI_sockaddr_in *to)
{
	SOCKADDR_STORAGE addr;
	NETAPIAddrToWinsockAddr(to, &addr);
	return sendto(s, (char*)buf, len, 0, (struct sockaddr *)&addr,sizeof(addr));
}


int NETAPI_recv(NETAPI_SOCKET s, void *buf, int len)
{
	return recv(s, (char*)buf, len, 0);
}

int NETAPI_recvfrom(NETAPI_SOCKET s, void* buf, int len, NETAPI_sockaddr_in *from)
{
	SOCKADDR_STORAGE fromaddr;
	int nFromLen = sizeof(fromaddr);
	int ret;
	ret = recvfrom(s, (char*)buf, len, 0, (struct sockaddr *)&fromaddr, &nFromLen);
	if(ret >=0 )
		WinsockAddrToNETAPIAddr(&fromaddr, from);
	return ret;
}

int NETAPI_closesocket(NETAPI_SOCKET s)
{
	return closesocket(s);
}

BOOL NETAPI_GetHostByName(const char *pszHostName, NETAPI_sockaddr_in *pAddrOut, short family)
{
	struct addrinfo *ai;
	struct addrinfo ai2;
	memset(&ai2, 0x00, sizeof(ai2));
	if(family)
		ai2.ai_family = family;
	if(getaddrinfo(pszHostName,NULL,&ai2,&ai) != 0)
		return FALSE;

	pAddrOut->sin_family = (short)ai->ai_family;

	if(ai->ai_family == AF_INET) {
		// for IPv4
		struct sockaddr_in* saddr = (struct sockaddr_in*)ai->ai_addr;
		pAddrOut->sin_addr.S_un.S_addr = saddr->sin_addr.S_un.S_addr;
		freeaddrinfo(ai);
		return TRUE;

	} else if(ai->ai_family == AF_INET6) {
		// for IPv6
		struct sockaddr_in6 *saddr6 = (struct sockaddr_in6*)ai->ai_addr;
		assert(sizeof(saddr6->sin6_addr) == sizeof(pAddrOut->sin_addr.S_un.bIPv6Data));
		//if(sizeof(saddr6->sin6_addr) > sizeof(pAddrOut->sin_addr.S_un.bIPv6Data))
		//	return FALSE;

		memcpy(pAddrOut->sin_addr.S_un.bIPv6Data, &saddr6->sin6_addr, sizeof(saddr6->sin6_addr));
		freeaddrinfo(ai);
		return TRUE;
	} else {
		assert(0);
		freeaddrinfo(ai);
		return FALSE;
	}
	// unreachable
}


int NETAPI_setsockopt(NETAPI_SOCKET s, int level, int optname, void *optval, int optlen)
{
	int ret;
	assert(s != INVALID_SOCKET_HANDLE);
	assert((level == SOL_SOCKET) || (level == IPPROTO_IP));
	assert(   (optname==SO_SNDBUF) 
		   || (optname==SO_RCVBUF) 
		   || (optname==SO_REUSEADDR) 
		   || (optname==SO_BROADCAST) 
		   || (optname==IP_MULTICAST_TTL) 
		   || (optname==IP_ADD_MEMBERSHIP) 
		   || (optname==IP_DROP_MEMBERSHIP));

	ret = setsockopt(s, level, optname, (const char*)optval, optlen);
	return ret;
}

BOOL NETAPI_IsLinkConnected(void)
{
	return TRUE;
}


#ifdef __cplusplus
extern "C" {
#endif
	
	unsigned short ___htons(unsigned short s)
	{
		return htons(s);
	}
	
#ifdef __cplusplus
}
#endif

