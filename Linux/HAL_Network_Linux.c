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
#include "all.h"

#ifdef ENABLE_NETWORK

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <signal.h>
#include <errno.h>


static const char *NETAPI_Config2Text(NETAPI_CONFIG eCfgVal);


// #define NETAPI_DEBUG_ENABLE

#ifdef NETAPI_DEBUG_ENABLE
#define NETAPI_DEBUG(x) UART_printf x
FAILRELEASEBUILD
#else
#define NETAPI_DEBUG(x)
#endif


// **IPv6 see /proc/net/if_inet6
// with help from http://www.raspberry-projects.com/pi/programming-in-c/tcpip/network-interface-code-snippets

#define BUFSIZE 8192
#if MAX_NETWORK_INTERFACES > 1
static pid_t dhcppid[MAX_NETWORK_INTERFACES] = {-1,-1};
#else
static pid_t dhcppid[MAX_NETWORK_INTERFACES] = {-1};
#endif




static int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;
	
	do {
		// Receive response from the kernel
		readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0);
		assert(readLen>=0);
        if (readLen < 0)
            return -1;        
		
        nlHdr = (struct nlmsghdr *) bufPtr;
		
		// Check if the header is valid 
		assert(NLMSG_OK(nlHdr, readLen));
		assert(nlHdr->nlmsg_type != NLMSG_ERROR);
        if (   (NLMSG_OK(nlHdr, readLen) == 0)
            || (nlHdr->nlmsg_type == NLMSG_ERROR))            
            return -1;        
		
		// Check if the its the last message
        if (nlHdr->nlmsg_type == NLMSG_DONE) {
            break;
        } else {
			// Else move the pointer to buffer appropriately
            bufPtr += readLen;
            msgLen += readLen;
        }
		
		// Check if its a multi part message
        if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
			// return if its not
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
    return msgLen;
}


static BOOL parseRoutes(struct nlmsghdr *nlHdr, struct in_addr *pDestAddr,struct in_addr *pGateway, int *pnOIF)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;

	assert(pDestAddr);
	assert(pGateway);
    rtMsg = (struct rtmsg *) NLMSG_DATA(nlHdr);
	memset(pDestAddr,0x00,sizeof(struct in_addr));
	memset(pGateway,0x00,sizeof(struct in_addr));

	// If the route is not for AF_INET or does not belong to main routing table then return.
    if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return FALSE;

	// get the rtattr field
    rtAttr = (struct rtattr *) RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
        switch (rtAttr->rta_type) 
		{
        case RTA_GATEWAY:
            pGateway->s_addr= *(u_int *) RTA_DATA(rtAttr);			
            break;
        case RTA_DST:
			pDestAddr->s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
		case RTA_OIF:
			*pnOIF =  *(u_int *) RTA_DATA(rtAttr);
			break;

        }
    }

    return TRUE;
}

static DWORD GetDefaultGW(int nInterfaceIndex)
{
    struct nlmsghdr *nlMsg;
    struct rtmsg *rtMsg;
	DWORD dwGW = 0;
    
	char msgBuf[BUFSIZE];
	
    int sock, len, msgSeq = 0;
	
	/* Create Socket */
    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
        perror("Socket Creation: ");
	
    memset(msgBuf, 0, BUFSIZE);
	
	/* point the header and the msg structure pointers into the buffer */
    nlMsg = (struct nlmsghdr *) msgBuf;
    rtMsg = (struct rtmsg *) NLMSG_DATA(nlMsg);

	/* Fill in the nlmsg header*/
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));  // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE;   // Get the routes from kernel routing table .
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;    // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++;    // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid();    // PID of process sending the request.
	
	/* Send the request */
    if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        printf("Write To Socket Failed...\n");
        return -1;
    }
	
	/* Read the response */
    if ((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
        printf("Read From Socket Failed...\n");
		return -1;
    }
	/* Parse and print the response */
	//UART_printf("Looking for Interface %d\r\n",nInterfaceIndex);
    for (; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
		struct in_addr DestAddr;
		struct in_addr Gateway;
		int nIF = 0;
        if(parseRoutes(nlMsg, &DestAddr, &Gateway, &nIF)) {
			//UART_printf("DestAddr %s",inet_ntoa(DestAddr));
			//UART_printf("Route %s IF %d\r\n",inet_ntoa(Gateway), nIF);
			if(DestAddr.s_addr == 0) {
				if(   ((nIF==2)&&(nInterfaceIndex==0))
					||((nIF!=2)&&(nInterfaceIndex!=0)))  // **TODO** this stinks, proper IF lookup needed
				{
					dwGW = htonl(Gateway.s_addr);
					break;
				}
			}
			//htonl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
		}
    }
    
    close(sock);
	return dwGW;
}

static void set_default_gw( const char *pszInterface, DWORD dwIP )
{
	struct sockaddr_in *dst, *gw, *mask;
	struct rtentry route;
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	int ret;
	assert(sockfd != -1);

	in_addr_t gip = htonl(dwIP);
	
	memset(&route,0,sizeof(struct rtentry));
	
	dst = (struct sockaddr_in *)(&(route.rt_dst));
	gw = (struct sockaddr_in *)(&(route.rt_gateway));
	mask = (struct sockaddr_in *)(&(route.rt_genmask));
	
	/* Make sure we're talking about IP here */
	dst->sin_family = AF_INET;
	gw->sin_family = AF_INET;
	mask->sin_family = AF_INET;
	
	/* Set up the data for removing the default route */
	dst->sin_addr.s_addr = 0;
	gw->sin_addr.s_addr = 0;
	mask->sin_addr.s_addr = 0;
	route.rt_flags = RTF_UP | RTF_GATEWAY;
	route.rt_dev = (char*)pszInterface;
	
	/* Remove the default route */
	ioctl(sockfd,SIOCDELRT,&route);
	
	/* Set up the data for adding the default route */
	dst->sin_addr.s_addr = 0;
	gw->sin_addr.s_addr = gip;
	mask->sin_addr.s_addr = 0;
	route.rt_metric = 1;
	route.rt_flags = RTF_UP | RTF_GATEWAY;
	route.rt_dev = (char*)pszInterface;
	
	/* Remove this route if it already exists */
	ioctl(sockfd,SIOCDELRT,&route);
	
	/* Add the default route */
	ret = ioctl(sockfd,SIOCADDRT,&route);
	assert(ret != -1);
	close(sockfd);

	return ;
}

static DWORD GetDNS(int nIndex)
{
	int hFile = open("/etc/resolv.conf", O_RDONLY);
	BYTE bBuf[512];
	int ret;
	memset(bBuf,0x00,sizeof(bBuf));
	assert(hFile != -1);
	if(hFile == -1)
		return 0;
	ret = read(hFile,bBuf,511);
	close(hFile);
	//UART_printf("read %d for %d\r\n",ret,nIndex);
	char *pNameServer = (char*)bBuf;
	while(pNameServer)
	{
		pNameServer = strstr(pNameServer,"nameserver ");
		if(!pNameServer) {
			//UART_printf("nameserver not found\r\n");
			return 0;
		}
		//UART_printf("hit:%s\r\n",pNameServer);
		if(nIndex==0) {
			return ntohl(inet_addr(pNameServer+11));
		}
		pNameServer++;

		nIndex--;
	}
	return 0;
}

static void SetDNS(DWORD dwNS1,DWORD dwNS2)
{
	char pszBuf[512];
	int ret;
	pszBuf[0] = 0;

	struct in_addr ns1,ns2;
	ns1.s_addr = htonl(dwNS1);
	ns2.s_addr = htonl(dwNS2);

	if(dwNS1)
		sprintf(pszBuf+strlen(pszBuf),"nameserver %s\n",inet_ntoa(ns1));
	if(dwNS2)
		sprintf(pszBuf+strlen(pszBuf),"nameserver %s\n",inet_ntoa(ns2));

	int hFile = open("/etc/resolv.conf", S_IRUSR|S_IWUSR, O_RDWR | O_CREAT | O_TRUNC);
	assert(hFile != -1);
	if(hFile == -1)
		return;
	ret = write(hFile,pszBuf,strlen(pszBuf));
	close(hFile);
	//UART_printf("wrote %d bytes to resolv.conf\r\n%s\r\n",ret,pszBuf);
}

BOOL NETAPI_Init(const BYTE *bMAC)
{
	int ret;
	//UART_printf("NETAPI_Init %08X\r\n",bMAC);

	// prevent process termination on a broken socket event
	signal(SIGPIPE, SIG_IGN);

	if(bMAC)
		NETAPI_SetConfiguration(0,NETCFG_MACADDRESS,(void*)bMAC);

	return TRUE;
}

BOOL NETAPI_IsLinkConnected(void)
{
	NETAPI_LINK_STATE eLinkState;
	NETAPI_GetConfiguration(0,NETCFG_LINKSTATE,&eLinkState);
	return eLinkState == NETAPI_LS_LINK_UP;
}

NETAPI_SOCKET NETAPI_socket(int af, int type, int protocol)
{
	return socket(AF_INET, type, protocol);
}
int NETAPI_bind(NETAPI_SOCKET s, const NETAPI_sockaddr_in *pAddr)
{
	//UART_printf("NETAPI_bind\r\n");
	switch(pAddr->sin_family)
	{
	case AF_INET:
		{
		struct sockaddr_in addr4;
		addr4.sin_family = pAddr->sin_family;
		addr4.sin_port = pAddr->sin_port;
		addr4.sin_addr.s_addr = pAddr->sin_addr.S_un.S_addr;
		return bind(s, (struct sockaddr*)&addr4,  sizeof(struct sockaddr_in));
		}
#ifdef ENABLE_NETAPI_IPV6
	case AF_INET6:
		{
		struct sockaddr_in6 addr6;
		addr6.sin6_family = pAddr->sin_family;
		addr6.sin6_port = pAddr->sin_port;
		memcpy(addr6.sin6_addr.s6_addr, pAddr->sin_addr.S_un.bIPv6Data,  sizeof(pAddr->sin_addr.S_un.bIPv6Data));
		return bind(s,(struct sockaddr*)&addr6,sizeof(struct sockaddr_in6));
		}
#endif
	default:
		return -1;
	}
}

int NETAPI_connect(NETAPI_SOCKET s, const NETAPI_sockaddr_in *pAddr)
{
	switch(pAddr->sin_family)
	{
	case AF_INET:
		{
		struct sockaddr_in addr4;
		addr4.sin_family = pAddr->sin_family;
		addr4.sin_port = pAddr->sin_port;
		addr4.sin_addr.s_addr = pAddr->sin_addr.S_un.S_addr;
		return connect(s, (struct sockaddr*)&addr4, sizeof(struct sockaddr_in));
		}
#ifdef ENABLE_NETAPI_IPV6
	case AF_INET6:
		{
		struct sockaddr_in6 addr6;
		addr6.sin6_family = pAddr->sin_family;
		addr6.sin6_port = pAddr->sin_port;
		memcpy(addr6.sin6_addr.s6_addr, pAddr->sin_addr.S_un.bIPv6Data, sizeof(pAddr->sin_addr.S_un.bIPv6Data));
		return connect(s, (struct sockaddr*)&addr6, sizeof(struct sockaddr_in6));
		}
#endif
	default:
		return -1;
	}
}

int NETAPI_listen(NETAPI_SOCKET s, int backlog)
{
	return listen(s, backlog);
}

NETAPI_SOCKET NETAPI_accept(NETAPI_SOCKET s, NETAPI_sockaddr_in *pAddr)
{
	union {
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	}u;
	int nAddrLen = max(sizeof(struct sockaddr_in),sizeof(struct sockaddr_in6));
	if(pAddr)
		u.addr.sin_family = pAddr->sin_family;
	else
		u.addr.sin_family = 0;

	NETAPI_SOCKET s2 = accept(s, (struct sockaddr*)&u.addr,(socklen_t*)&nAddrLen);
	//UART_printf("NETAPI_accept returned new socket %d\r\n",s2);
	if(pAddr) {
		pAddr->sin_family = u.addr.sin_family;
		switch(u.addr.sin_family)
		{
		case AF_INET:
			{
			struct sockaddr_in *pAddr4 = (struct sockaddr_in*)&u.addr;
			pAddr->sin_port = pAddr4->sin_port;
			pAddr->sin_addr.S_un.S_addr = pAddr4->sin_addr.s_addr;
			}
			break;
#ifdef ENABLE_NETAPI_IPV6
		case AF_INET6:
			{
			struct sockaddr_in6 *pAddr6 = (struct sockaddr_in6*)&u.addr6;
			pAddr->sin_port = pAddr6->sin6_port;
			memcpy(pAddr->sin_addr.S_un.bIPv6Data, pAddr6->sin6_addr.s6_addr, sizeof(pAddr->sin_addr.S_un.bIPv6Data));
			}
			break;
#endif
		default:
			assert(0);
			break;
		}
	}
	return s2;
}

int NETAPI_send(NETAPI_SOCKET s, const void *buf, int len)
{
	return send(s,buf,len,0);
}

int NETAPI_sendto(NETAPI_SOCKET s, const void *buf, int len, const NETAPI_sockaddr_in *to)
{
	int ret;
	assert(to);

	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(addr));
	//UART_printf("NETAPI_sendto %d %08X %d\r\n",s,to->sin_addr.S_un.S_addr,to->sin_port);
	addr.sin_family = to->sin_family;
	addr.sin_port = to->sin_port;
	// **TODO** IPv6
	addr.sin_addr.s_addr = to->sin_addr.S_un.S_addr;

	if(to->sin_addr.S_un.S_addr == INADDR_BROADCAST) {
		int broadcastPermission = 1;
		ret = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission));
		assert(ret == 0);
		if(ret != 0)
			return ret;
	}

	ret = sendto(s,buf,len,0,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));
	return ret;
}

int NETAPI_recv(NETAPI_SOCKET s, void *buf, int len)
{
	int ret = recv(s,buf,len,0);
	return ret;
}

int NETAPI_recvfrom(NETAPI_SOCKET s, void* buf, int len, NETAPI_sockaddr_in *from)
{
	int ret;
	int nFromLen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	addr.sin_family = from->sin_family;
	ret = recvfrom(s,buf,len,0,(struct sockaddr*)&addr,(socklen_t*)&nFromLen);
	from->sin_port = addr.sin_port;
	from->sin_addr.S_un.S_addr = addr.sin_addr.s_addr;
	return ret;
}

int NETAPI_closesocket(NETAPI_SOCKET s)
{
	if(s>0) {
		// the shutdown is required to get other threads out of recv with an error
		shutdown(s, SHUT_RDWR);

		return close(s);
	}
	return -1;
}

BOOL NETAPI_SetConfiguration(int nIfIndex, NETAPI_CONFIG eCfgVal, void *pData)
{
	struct ifreq ifr;
	DWORD *pdwAddr = (DWORD *)pData;
	BYTE *pAddr = (BYTE *)pData;
	//NETAPI_LINK_STATE *pLinkState = (NETAPI_LINK_STATE *)pData;
	BOOL *pBOOL = (BOOL*)pData;
	int ret;
	int s;

    NETAPI_DEBUG(("NETAPI_SetConfiguration %d, %s, %08X\r\n", nIfIndex, NETAPI_Config2Text(eCfgVal), pData));
#ifdef ENABLE_WLAN
	if(nIfIndex>1)
		return FALSE;
#else
	if(nIfIndex>0)
		return FALSE;
#endif

#ifdef PLATFORM_LINUX // we do not want to reconfigure the build&test VM
	return FALSE;
#endif

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth0");
#ifdef ENABLE_WLAN
	if(nIfIndex==1) {
		strcpy(ifr.ifr_name, "wlan0");

		static BOOL fWiFiStarted = false;
		if(!fWiFiStarted) {
			fWiFiStarted = true;
			_wpa_ctrl_command("ADD_NETWORK",0, NULL);
			_wpa_ctrl_command("ENABLE_NETWORK 0",0, NULL);
		}
	}
#endif
	
	switch(eCfgVal)
	{

	case NETCFG_MACADDRESS: //!< BYTE[6]
		s = socket(AF_INET,SOCK_DGRAM,0);
		ret = ioctl(s, SIOCSIFFLAGS, &ifr);
		assert(ret == 0);
		if(ifr.ifr_flags & IFF_RUNNING)
			ifr.ifr_flags -= IFF_RUNNING;
		if(ifr.ifr_flags & IFF_UP)
			ifr.ifr_flags -= IFF_UP;
		ret = ioctl(s, SIOCSIFFLAGS, &ifr);
		assert(ret == 0);
		ifr.ifr_addr.sa_family = AF_UNIX;
		ifr.ifr_hwaddr.sa_data[0] = pAddr[0];
		ifr.ifr_hwaddr.sa_data[1] = pAddr[1];
		ifr.ifr_hwaddr.sa_data[2] = pAddr[2];
		ifr.ifr_hwaddr.sa_data[3] = pAddr[3];
		ifr.ifr_hwaddr.sa_data[4] = pAddr[4];
		ifr.ifr_hwaddr.sa_data[5] = pAddr[5];
		ret = ioctl(s, SIOCSIFHWADDR, &ifr);
		assert(ret == 0);
		//UART_printf("ioctl SIOCSIFHWADDR %d MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n",ret,pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]);
		ifr.ifr_flags |= (IFF_UP | IFF_RUNNING | IFF_MULTICAST);
		ret = ioctl(s, SIOCSIFFLAGS, &ifr);
		assert(ret == 0);
		//UART_printf("ioctl SIOCGIFADDR %d MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n",ret,pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]);
		close(s);
		return ret == 0;
	case NETCFG_DHCPENABLE:	//!< BOOL, TRUE if DHCP is enabled
		if(*pBOOL) {
			if(dhcppid[nIfIndex] != -1)
				return TRUE;
			dhcppid[nIfIndex] = fork();
			if(dhcppid[nIfIndex]==0)
			{
				// system("udhcpc -f -V GKTV");
				char pszCommand[100];
				sprintf(pszCommand, "udhcpc -f -i %s -V GKTV &> /dev/null", ifr.ifr_name);
				//sprintf(pszCommand, "udhcpc -f -i %s -V GKTV &", ifr.ifr_name);
				system(pszCommand);
				exit(0);
			} else {
				//UART_printf("DHCP pid %d\r\n",dhcppid);
			}
		} else {
			//UART_printf("DHCP off - pid %d\r\n",dhcppid);
			if(dhcppid[nIfIndex] != -1) {
				ret = kill(dhcppid[nIfIndex], SIGTERM);
				//UART_printf("kill return %d\r\n",ret);
				ret = kill(dhcppid[nIfIndex], SIGKILL);
				//UART_printf("kill return %d\r\n",ret);
				dhcppid[nIfIndex] = -1;
			}
			return TRUE;
		}
		break;
	case NETCFG_IPADDR:		//!< DWORD, IP Address
		s = socket(AF_INET,SOCK_DGRAM,0);
		ifr.ifr_addr.sa_family = AF_INET;
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr = htonl(*pdwAddr);
		ret = ioctl(s, SIOCSIFADDR, &ifr);
		assert(ret == 0);
		close(s);
		return ret == 0;
	case NETCFG_SUBNET:		//!< DWORD, Subnet mask
		s = socket(AF_INET,SOCK_DGRAM,0);
		ifr.ifr_netmask.sa_family = AF_INET;
		((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr.s_addr = htonl(*pdwAddr);
		ret = ioctl(s, SIOCSIFNETMASK, &ifr);
		assert(ret == 0);
		close(s);
		return TRUE;
		break;
	case NETCFG_GATEWAY:	//!< DWORD, Default Gateway
		set_default_gw(ifr.ifr_name, *pdwAddr);
		return TRUE;
	case NETCFG_NS1:		//!< DWORD, DNS server 1
		SetDNS(*pdwAddr,GetDNS(1));
		return TRUE;
	case NETCFG_NS2:		//!< DWORD, DNS server 2
		SetDNS(GetDNS(0),*pdwAddr);
		return TRUE;
	case NETCFG_LINKSTATE:	//!< #NETAPI_LINK_STATE, read-only link state, if supported by hardware driver
		return FALSE; // read-only
	case NETCFG_CONNECTTO:	//!< DWORD global approximate connect timeout in seconds, if supported by network stack
		break;
	}
	return FALSE;
}


BOOL NETAPI_GetConfiguration(int nIfIndex, NETAPI_CONFIG eCfgVal, void *pData)
{
	struct ifreq ifr;
	int ret,s;
	DWORD *pdwAddr = (DWORD *)pData;
	BYTE *pAddr = (BYTE *)pData;
	NETAPI_LINK_STATE *pLinkState = (NETAPI_LINK_STATE *)pData;
	BOOL *pBOOL = (BOOL*)pData;

	if(nIfIndex>0)
		return FALSE;
	
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth0");
		
	switch(eCfgVal)
	{
	case NETCFG_MACADDRESS: //!< BYTE[6]
		s = socket(AF_INET,SOCK_DGRAM,0);
		ret = ioctl(s, SIOCGIFHWADDR, &ifr);
		pAddr[0] = ifr.ifr_hwaddr.sa_data[0];
		pAddr[1] = ifr.ifr_hwaddr.sa_data[1];
		pAddr[2] = ifr.ifr_hwaddr.sa_data[2];
		pAddr[3] = ifr.ifr_hwaddr.sa_data[3];
		pAddr[4] = ifr.ifr_hwaddr.sa_data[4];
		pAddr[5] = ifr.ifr_hwaddr.sa_data[5];
		//UART_printf("ioctl SIOCGIFADDR %d MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n",ret,pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]);
		close(s);
		return TRUE;
	case NETCFG_DHCPENABLE:	//!< BOOL, TRUE if DHCP is enabled
		if(dhcppid[nIfIndex] == -1)
			*pBOOL = FALSE;
		else
			*pBOOL = TRUE;
		return TRUE;
	case NETCFG_IPADDR:		//!< DWORD, IP Address
		s = socket(AF_INET,SOCK_DGRAM,0);
		ret = ioctl(s, SIOCGIFADDR, &ifr);
		*pdwAddr = htonl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
		close(s);
		return TRUE;
	case NETCFG_SUBNET:		//!< DWORD, Subnet mask
		s = socket(AF_INET,SOCK_DGRAM,0);
		ret = ioctl(s, SIOCGIFNETMASK, &ifr);
		*pdwAddr = htonl(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr.s_addr);
		close(s);
		return TRUE;
	case NETCFG_GATEWAY:	//!< DWORD, Default Gateway
		*pdwAddr = GetDefaultGW(nIfIndex);
		return TRUE;
	case NETCFG_NS1:		//!< DWORD, DNS server 1
		*pdwAddr = GetDNS(0);
		return TRUE;
	case NETCFG_NS2:		//!< DWORD, DNS server 2
		*pdwAddr = GetDNS(1);
		return TRUE;
	case NETCFG_LINKSTATE:	//!< #NETAPI_LINK_STATE, read-only link state, if supported by hardware driver		
		s = socket(AF_INET,SOCK_DGRAM,0);
		ret = ioctl(s, SIOCGIFFLAGS, &ifr);
		close(s);
		//UART_printf("ioctl SIOCGIFFLAGS %d %08X\r\n",ret,ifr.ifr_flags);
		if(ifr.ifr_flags & IFF_RUNNING)
			*pLinkState = NETAPI_LS_LINK_UP;
		else
			*pLinkState = NETAPI_LS_LINK_DOWN;
		return TRUE;
	case NETCFG_CONNECTTO:	//!< DWORD global approximate connect timeout in seconds, if supported by network stack
		break;
	}
	return FALSE;
}

BOOL NETAPI_GetHostByName(const char *pszHostName, NETAPI_sockaddr_in *pAddrOut, short family)
{
	//UART_printf("NETAPI_GetHostByName %s\r\n", pszHostName);
	assert(pszHostName);
	if(!pszHostName)
		return FALSE;
	struct hostent *he;
	if(pszHostName[0]=='[') {
		pszHostName++;
		char *pCopy = strdup(pszHostName);
		assert(pCopy);
		if(!pCopy)
			return FALSE;
		if(strchr(pCopy,']'))
			*strchr(pCopy,']') = 0;
		he = gethostbyname2(pCopy, AF_INET6);
		free(pCopy);
	} else {
		//he = gethostbyname(pszHostName);
		he = gethostbyname2(pszHostName, AF_INET);
		if(!he)
			he = gethostbyname2(pszHostName, AF_INET6);
	}
	//printf("Returned %p\r\n",he);
	if(!he)
		return FALSE;
	if(!he->h_addr_list)
		return FALSE;


	DWORD *pAddr = (DWORD*)*he->h_addr_list;

	pAddrOut->sin_port = 0;
	pAddrOut->sin_family = he->h_addrtype;
	switch(he->h_addrtype)
	{
	case AF_INET:
		{
		pAddrOut->sin_addr.S_un.S_addr = *pAddr;//addr4->s_addr;
		//UART_printf("INET 4 %08X\r\n",pAddrOut->sin_addr.S_un.S_addr);
		}
		break;
#ifdef ENABLE_NETAPI_IPV6
	case AF_INET6:
		{
		//UART_printf("INET 6\r\n");
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)he->h_addr_list[0];
		memcpy(pAddrOut->sin_addr.S_un.bIPv6Data, he->h_addr_list[0], sizeof(pAddrOut->sin_addr.S_un.bIPv6Data));
		}
		break;
#endif
	default:
		assert(0);
		break;
	}
	return TRUE;
}

int NETAPI_setsockopt(NETAPI_SOCKET s, int level, int optname, void *optval, int optlen)
{
	if((level==IPPROTO_IP) && (optname==IP_ADD_MEMBERSHIP))
	{
		NETAPI_ip_mreq *pnmreq = optval;
		struct ip_mreq mreq;
		assert(pnmreq);
		assert(optlen >= sizeof(NETAPI_ip_mreq));
		if((!pnmreq) || (optlen<sizeof(NETAPI_ip_mreq)))
			return -1;
		
		mreq.imr_interface.s_addr = pnmreq->imr_interface;
		mreq.imr_multiaddr.s_addr = pnmreq->imr_multiaddr;
		return setsockopt(s, level, optname, &mreq, sizeof(mreq));
	} else if((level==IPPROTO_IP) && (optname==IP_DROP_MEMBERSHIP)) {
		NETAPI_ip_mreq *pnmreq = optval;
		struct ip_mreq mreq;
		assert(pnmreq);
		assert(optlen >= sizeof(NETAPI_ip_mreq));
		if((!pnmreq) || (optlen<sizeof(NETAPI_ip_mreq)))
			return -1;
		
		mreq.imr_interface.s_addr = pnmreq->imr_interface;
		mreq.imr_multiaddr.s_addr = pnmreq->imr_multiaddr;
		return setsockopt(s, level, optname, &mreq, sizeof(mreq));
	} else if((level==IPPROTO_IP) && (optname==IP_MULTICAST_TTL))
	{
		return setsockopt(s, level, optname, optval, optlen);
	} else if((level==IPPROTO_IPV6) && (optname==IP_MULTICAST_TTL))
	{
		return setsockopt(s, level, optname, optval, optlen);
	} else if((level==SOL_SOCKET) && (optname==SO_REUSEADDR))
	{
		return setsockopt(s, level, optname, optval, optlen);
	} else if((level==SOL_SOCKET) && (optname==SO_RCVBUF))
	{
		return setsockopt(s, level, optname, optval, optlen);
	} else if((level==SOL_SOCKET) && (optname==SO_SNDBUF))
	{
		return setsockopt(s, level, optname, optval, optlen);
	} else if((level==SOL_SOCKET) && (optname==SO_BROADCAST))
	{
		return setsockopt(s, level, optname, optval, optlen);
	} else {
		//UART_debug("Unsupported NETAPI_setsockopt call level %d opt %d\r\n",level,optname);
		assert(0);
		return -1;
	}
	// should be unreachable
}


static const char *NETAPI_Config2Text(NETAPI_CONFIG eCfgVal)
{
    switch(eCfgVal)
    {
    case NETCFG_INVALID:
        return "NETCFG_INVALID";
	case NETCFG_MACADDRESS:
        return "NETCFG_MACADDRESS";
	case NETCFG_DHCPENABLE:
        return "NETCFG_DHCPENABLE";
	case NETCFG_IPADDR:
        return "NETCFG_IPADDR";
	case NETCFG_SUBNET:
        return "NETCFG_SUBNET";
	case NETCFG_GATEWAY:
        return "NETCFG_GATEWAY";
	case NETCFG_NS1:
        return "NETCFG_NS1";
	case NETCFG_NS2:
        return "NETCFG_NS2";
	case NETCFG_LINKSTATE:
        return "NETCFG_LINKSTATE";
	case NETCFG_CONNECTTO:
        return "NETCFG_CONNECTTO";
	case NETCFG_WIFI_SCAN:
        return "NETCFG_WIFI_SCAN";
	case NETCFG_WIFI_SSID:
        return "NETCFG_WIFI_SSID";
	case NETCFG_WIFI_PSK:
        return "NETCFG_WIFI_PSK";
	case NETCFG_WIFI_STATE:
        return "NETCFG_WIFI_STATE";
    default:
        return "[UNKNOWN]";
    }
}

#endif // ENABLE_NETWORK
