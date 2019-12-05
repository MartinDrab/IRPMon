
#ifndef __DEVICE_CONNECTOR_H__
#define __DEVICE_CONNECTOR_H__


#include <windows.h>
#include "general-types.h"
#include "kernel-shared.h"





DWORD DevConn_SynchronousNoIOIOCTL(DWORD Code);
DWORD DevConn_SynchronousWriteIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength);
DWORD DevConn_SynchronousReadIOCTL(DWORD Code, PVOID OutputBuffer, ULONG OutputBufferLength);
DWORD DevConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

DWORD DevConn_Connect(void);
void DevConn_Disconnect(void);
BOOL DevConn_Active(VOID);



#endif
