
#ifndef __LIBVSOCK_H__
#define __LIBVSOCK_H__

#include <winsock2.h>
#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif

unsigned short LibVSockGetAddressFamily(void);
unsigned int LibVSockGetVersion(void);
unsigned int LibVSockGetLocalId(void);
int LibVSockAddressAlloc(unsigned int CID, unsigned int Port, struct sockaddr** Address, int* Len);
void LibVSockAddressFree(struct sockaddr* Address);

#ifdef __cplusplus
}
#endif



#endif
