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
#ifndef __HAL_NETWORK_H
#define __HAL_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define ENABLE_NETAPI_IPV6
#endif

/////////////////////////////////////////////
// Networking
//! @file HAL_Network.h
//! @brief This file contains the API for a BSD-socket like networking API

#ifdef __amd64__
typedef long NETAPI_SOCKET;		// this needs to have the size of a pointer for some TLS-related type casts
#else
typedef int NETAPI_SOCKET;		// this is the "original" BSD type
#endif
#define INVALID_SOCKET_HANDLE -1


#ifndef AF_INET
#define AF_INET 2
#endif

#ifndef AF_INET6
#ifdef ENABLE_LWIP
#ifdef LWIP_IPV6
#define AF_INET6 10		// Warning: LWIP uses a different value
#endif
#else
#define AF_INET6 23		// Warning: LWIP uses a different value
#endif
#endif

#ifndef SOCK_STREAM
#define SOCK_STREAM		1
#endif

#ifndef SOCK_DGRAM
#define SOCK_DGRAM		2
#endif


#ifndef IPPROTO_IP
#define IPPROTO_IP		0
#endif

#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6	41
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP		6
#endif

#ifndef IPPROTO_UDP
#define IPPROTO_UDP		17
#endif

#ifndef INADDR_ANY
#define INADDR_ANY		0
#endif

#ifndef INADDR_NONE
#define INADDR_NONE		 0xFFFFFFFF
#endif

#ifndef INADDR_BROADCAST
#define INADDR_BROADCAST 0xFFFFFFFF
#endif

#ifndef SOL_SOCKET
#define SOL_SOCKET      0XFFFF
#endif

#ifndef SO_REUSEADDR
#define SO_REUSEADDR		0x0004
#define SO_BROADCAST		0x0020
#define SO_SNDBUF			0x1001
#define SO_RCVBUF			0x1002
#endif

#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL    10 // was 3 in old winsock header
#define IP_ADD_MEMBERSHIP   12 // was 5 in old winsock header
#define IP_DROP_MEMBERSHIP  13 // was 6 in old winsock header
#endif

#define MAX_UDP_PACKET_SIZE 65507		// 0xffff - (sizeof(IP Header) + sizeof(UDP Header)) = 65535-(20+8) = 65507

#define MAX_WLANPSKLEN        128 // seems to be max 256bit (32 Byte) (relation to sha1)
#define MAX_SSIDLEN            64 // http://standards.ieee.org/getieee802/download/802.11-2007.pdf --> 0-32 octets

typedef struct NETAPI_in_addr {
	union {
		struct { BYTE s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { WORD s_w1,s_w2; } S_un_w;
		DWORD S_addr;
#ifdef ENABLE_NETAPI_IPV6
		BYTE bIPv6Data[16];
#endif
	} S_un;
} NETAPI_in_addr;

typedef struct NETAPI_sockaddr_in {
	short			sin_family;
	WORD			sin_port;
	NETAPI_in_addr	sin_addr;
} NETAPI_sockaddr_in;

typedef struct NETAPI_ip_mreq {
    DWORD imr_multiaddr; // IP multicast address of group
    DWORD imr_interface; // local IP address of interface
} NETAPI_ip_mreq;

typedef struct NETAPI_ipv6_mreq {
	BYTE			ipv6mr_multiaddr[16];	// IPv6 multicast address
    //struct in6_addr ipv6mr_multiaddr;  
    unsigned int    ipv6mr_interface;  // Interface index
} NETAPI_ipv6_mreq;

#ifndef HTONL_DEFINED
unsigned short htons(unsigned short s);
unsigned short ntohs(unsigned short s);
unsigned long  htonl(unsigned long l);
unsigned long  ntohl(unsigned long l);
#endif


//! NETAPI_Init should be called before any other network API function.
//! It is supposed to initialize the network hardware and software stacks.
//! @param bMAC The MAC-address that should be used for the first interface.
//! @return TRUE if the initialization was successful, FALSE otherwise.
BOOL NETAPI_Init(const BYTE *bMAC);

//! NETAPI_IsLinkConnected can be used to find out if at least one
//! network interface is available and connected.
//! @return TRUE If a network link is available and connected, FALSE otherwise
BOOL NETAPI_IsLinkConnected(void);

//! NETAPI_socket allocates a new socket handle.
//! It is the equivalent of the bsd-socket function "socket()".
//! @param af Adress family, should be AF_INET
//! @param type socket type, should be SOCK_STREAM or SOCK_DGRAM
//! @param protocol The used protocol, this is currently unused
//! @return A valid socket handle or INVALID_SOCKET_HANDLE(-1) in case of an error
NETAPI_SOCKET NETAPI_socket(int af, int type, int protocol);

//! NETAPI_bind binds a socket to a specific local adress
//! @param s A valid socket handle
//! @param pAddr Pointer to the structure describing the requested local address.
//! @return Zero in case of success or a negative value in case of an error
int NETAPI_bind(NETAPI_SOCKET s, const NETAPI_sockaddr_in *pAddr);

//! For connection oriented sockets NETAPI_connect tries to establish a connection to a remote address.
//! For datagram sockets NETAPI_connect configures the default destination address that is used for calls to NETAPI_send instead of NETAPI_sendto.
//! @param s A valid socket handle
//! @param name The address to connect to
//! @return Zero in case of success or a negative value in case of an error
int NETAPI_connect(NETAPI_SOCKET s, const NETAPI_sockaddr_in *name);

//! NETAPI_listen prepares a socket for incoming connections
//! @param s A valid socket handle
//! @param backlog The number of pending incoming connections that may queue up internally before being passed out through NETAPI_accept.
//! @return Zero in case of success or a negative value in case of an error
int NETAPI_listen(NETAPI_SOCKET s, int backlog);


//! NETAPI_accept waits for an incoming connection on a socket that has been set into listening mode before.
//! @param s A valid socket handle
//! @param addr Output pointer for the remote address. This might be NULL if the caller is not interested in the remote address.
//! @return A new socket handle that has been allocated for the incoming connection or INVALID_SOCKET_HANDLE(-1) in case of an error or closure of the listening socket.
NETAPI_SOCKET NETAPI_accept(NETAPI_SOCKET s, NETAPI_sockaddr_in *addr);


//! NETAPI_send is used to send data over an established connection.
//! @param s A valid socket handle with an established connection
//! @param buf Pointer to the data to send
//! @param len Size of the data to send
//! @return The number of bytes that have been sent or a negative value in case of an error.
int NETAPI_send(NETAPI_SOCKET s, const void *buf, int len);

//! NETAPI_sendto is used to send data to a specific remote address.
//! This function should only be used on datagram oriented sockets.
//! @param s A valid socket handle with an established connection
//! @param buf Pointer to the data to send
//! @param len Size of the data to send
//! @param to The address that the data should be sent to.
//! @return The number of bytes that have been sent or a negative value in case of an error.
int NETAPI_sendto(NETAPI_SOCKET s, const void *buf, int len, const NETAPI_sockaddr_in *to);


//! NETAPI_recv is used to receive data on an established connection.
//! @param s A valid socket handle with an established connection.
//! @param buf Pointer to a buffer that received data will be written into
//! @param len The number of bytes to read.
//! @return The number of bytes that have been received. This might be less than len bytes ! A negative value will be returned in case of an error.
int NETAPI_recv(NETAPI_SOCKET s, void *buf, int len);

//! NETAPI_recvfrom is used to receive data and read the remote address at the same time. 
//! For datagram based sockets this is the only function that provides information about the source of the datagram.
//! @param s A valid socket handle.
//! @param buf Pointer to a buffer that received data will be written into
//! @param len The number of bytes to read.
//! @param from Output pointer for the remote address.
//! @return The number of bytes that have been received. This might be less than len bytes ! A negative value will be returned in case of an error.
int NETAPI_recvfrom(NETAPI_SOCKET s, void* buf, int len, NETAPI_sockaddr_in *from);

//! NETAPI_shutdown disables send and/or receive functions on a socket.
//! This function should be called before closing a socket to make sure that all remaining data is sent and not discarded.
//! @param s A valid socket handle
//! @return Zero in case of success or a negative value in case of an error.
int NETAPI_shutdown(NETAPI_SOCKET s);

//! NETAPI_closesocket closes a socket and releases all resources that have been allocated for it.
//! If the socket is connected when this function is called, the connection is closed.
//! @param s A valid socket handle
//! @return Zero for success or a negative value in case of an error.
int NETAPI_closesocket(NETAPI_SOCKET s);

//! NETAPI_GetErrorDescription translates a negative return value as it is returned by some network functions
//! into a human-readable error message. This function is only supposed to be used for debug-purposes
//! and might be implemented in a non-reentrant way (e.g. static format string buffer) on some platforms.
//! @param nErrCode The error code to translate
//! @return A text string describing the error
const char *NETAPI_GetErrorDescription(int nErrCode);

//! NETAPI_CONFIG contains one entry for each item that can be configured for a network interface using #NETAPI_SetConfiguration or #NETAPI_GetConfiguration.
typedef enum {
	NETCFG_INVALID = 0,
	NETCFG_MACADDRESS = 1,	//!< BYTE[6]
	NETCFG_DHCPENABLE = 2,	//!< BOOL, TRUE if DHCP is enabled
	NETCFG_IPADDR = 3,		//!< DWORD, IP Adress
	NETCFG_SUBNET = 4,		//!< DWORD, Subnet mask
	NETCFG_GATEWAY = 5,		//!< DWORD, Default Gateway
	NETCFG_NS1 = 6,			//!< DWORD, DNS server 1
	NETCFG_NS2 = 7,			//!< DWORD, DNS server 2
	NETCFG_LINKSTATE = 8,	//!< #NETAPI_LINK_STATE, read-only link state, if supported by hardware driver
	NETCFG_CONNECTTO = 9,	//!< DWORD global approximate connect timeout in seconds, if supported by network stack
	NETCFG_WIFI_SCAN = 10,	//!< linux wpa-supplicant-like tab-delimited and zero-terminated buffer (size 2048)
	NETCFG_WIFI_SSID = 11,	//!< char*, string containing the new selected SSID, NULL to clear
	NETCFG_WIFI_PSK = 12,	//!< char*, string containing the new PSK password, NULL to clear
	NETCFG_WIFI_STATE = 13  //!< #NETAPI_WIFI_STATE, read-only wifi state
} NETAPI_CONFIG;

typedef enum {
	NETAPI_LS_INVALID = 0,	//!< should not be used
	NETAPI_LS_UNSUPPORTED=1,//!< the hardware or driver has no media sense support
	NETAPI_LS_LINK_DOWN=2,	//!< feature supported & link down
	NETAPI_LS_LINK_NEG=3,	//!< optional transitional state, autonegotiation in progress
	NETAPI_LS_LINK_UP=4		//!< feature supported & link up
	// some more states might be required for WLAN later
} NETAPI_LINK_STATE;

typedef enum {
	NETAPI_WS_INVALID = 0,	//!< unsupported or error occurred
	NETAPI_WS_INACTIVE = 1,
 	NETAPI_WS_SCANNING = 2,
	NETAPI_WS_CONNECTING = 3,	//!< ASSOCIATING / 4WAY_HANDSHAKE
	NETAPI_WS_CONNECTED = 4
} NETAPI_WIFI_STATE;

//! NETAPI_SetConfiguration can be used to configure network stack parameters
//! @param nIfIndex Zero based index of the network interface to configure
//! @param eCfgVal Parameter to configure
//! @param pData Pointer to the configuration data, see #NETAPI_CONFIG for details.
//! @return TRUE if the configuration was set successfully, FALSE otherwise
BOOL NETAPI_SetConfiguration(int nIfIndex, NETAPI_CONFIG eCfgVal, void *pData);

//! NETAPI_GetConfiguration can be read configured network stack parameters
//! @param nIfIndex Zero based index of the network interface
//! @param eCfgVal Parameter to read
//! @param pData Pointer that the configuration data will be written to, see #NETAPI_CONFIG for details.
//! @return TRUE if the configuration was read successfully, FALSE otherwise
BOOL NETAPI_GetConfiguration(int nIfIndex, NETAPI_CONFIG eCfgVal, void *pData);

//! NetAPI_GetHostByName performs a DNS lookup for the A record (IPv4 host address) or AAAA record (IPv6 host address) of the specified host name.
//! This function may block for a platform-dependent amount of seconds if the request can not
//! be answered from the local cache already.
//! @param pszHostName The host name
//! @param pAddrOut output pointer for the lookup result. The port number of the output will remain unchanged.
//!                 The address family and address fields will be overwritten/filled if the lookup was successful.
//! @param family If non-zero, forces a lookup specifically for the indicated address family (typically AF_INET or AF_INET6)
//! @return TRUE if the lookup was successful and the output data is now valid.
BOOL NETAPI_GetHostByName(const char *pszHostName, NETAPI_sockaddr_in *pAddrOut, short family);

//! NETAPI_setsockopt sets a socket option.
//! @param s A valid socket handle
//! @param level The level at which the option is defined (e.g. SOL_SOCKET or IPPROTO_IP)
//! @param optname The socket option to be set (e.g. SO_SNDBUF or IP_MULTICAST_TTL)
//! @param optval Pointer to a buffer containing the new configuration value.
//! @param optlen The size of the optval buffer (in bytes)
//! @return On success, the function returns zero. A negative value is returned in case of an error.
int NETAPI_setsockopt(NETAPI_SOCKET s, int level, int optname, void *optval, int optlen);

//! NETAPI_getsockname gets the local address of a socket
//! @param s A valid socket handle
//! @param pAddrOut Output pointer for the address.
//! @return On success, the function returns zero. A negative value is returned in case of an error.
int NETAPI_getsockname(NETAPI_SOCKET s, NETAPI_sockaddr_in *pAddrOut);

// ******************************************
//   Platform-Independent HAL_Media Utils
BOOL NETAPI_HALUtil_IsMulticastAddress(const NETAPI_sockaddr_in *pAddr);

#ifdef __cplusplus

bool NETAPI_HALUTIL_InetNToA(const NETAPI_sockaddr_in &addr, char *pszOut, bool fIncludePort = false);
bool NETAPI_HALUTIL_ParseIPArg(const char *pszArg, const char *pszPrefix, NETAPI_sockaddr_in &addr);

}
#endif

#endif // __HAL_NETWORK_H
