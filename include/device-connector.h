
#ifndef __DEVICE_CONNECTOR_H__
#define __DEVICE_CONNECTOR_H__


#include <windows.h>
#include "general-types.h"
#include "irpmondll-types.h"





DWORD DevConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

DWORD DevConn_Connect(const IRPMON_INIT_INFO *Info);
void DevConn_Disconnect(void);
BOOL DevConn_Active(VOID);



#endif
