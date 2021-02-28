
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
int LibVSockAddressPrintA(const struct sockaddr *Address, char *Buffer, size_t CharCount);
int LibVSockAddressPrintW(const struct sockaddr *Address, wchar_t *Buffer, size_t CharCount);


#ifdef __cplusplus
}
#endif



#endif
