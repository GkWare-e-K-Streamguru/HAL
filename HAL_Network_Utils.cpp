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

BOOL NETAPI_HALUtil_IsMulticastAddress(const NETAPI_sockaddr_in *pAddr)
{
	assert(pAddr);
	if(!pAddr)
		return FALSE;
	
	if((pAddr->sin_family == AF_INET) && (pAddr->sin_addr.S_un.S_un_b.s_b1>=224) && (pAddr->sin_addr.S_un.S_un_b.s_b1<=239)) {
        return true;
    }
#ifdef ENABLE_NETAPI_IPV6
	// see https://en.wikipedia.org/wiki/Multicast_address#Notable_IPv6_multicast_addresses
	// "Multicast addresses in IPv6 use the prefix ff00::/8."
	if((pAddr->sin_family == AF_INET6)) {
		if(pAddr->sin_addr.S_un.bIPv6Data[0] == 0xFF)
			return TRUE;
    }
#endif
	return FALSE;
}

#endif // ENABLE_NETWORK
