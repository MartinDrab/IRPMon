
#ifndef __NETWORK_CONNECTOR_H__
#define __NETWORK_CONNECTOR_H__



#include <windows.h>
#include "general-types.h"
#include "kernel-shared.h"



DWORD NetConn_SynchronousNoIOIOCTL(DWORD Code);
DWORD NetConn_SynchronousWriteIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength);
DWORD NetConn_SynchronousReadIOCTL(DWORD Code, PVOID OutputBuffer, ULONG OutputBufferLength);
DWORD NetConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

DWORD NetConn_Connect(void);
void NetConn_Disconnect(void);
BOOL NetConn_Active(VOID);



#endif
