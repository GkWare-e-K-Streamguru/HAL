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
#ifdef AF_INET6
	if((pAddr->sin_family == AF_INET6)) {		
		assert(0); // **TODO**
        return FALSE;
    }
#endif
	return FALSE;
}

#endif // ENABLE_NETWORK
