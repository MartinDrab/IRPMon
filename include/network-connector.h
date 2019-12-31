
#ifndef __NETWORK_CONNECTOR_H__
#define __NETWORK_CONNECTOR_H__



#include <stdint.h>
#include <windows.h>
#include "irpmondll-types.h"

#define IOCTL_IRPMON_SERVER_ARCH_32BIT				1
#define IOCTL_IRPMON_SERVER_ARCH_64BIT				2

typedef struct _NETWORK_MSG_IOCTL {
	uint32_t Result;
	uint32_t ControlCode;
	uint32_t InputBufferSize;
	uint32_t OutputBufferSize;
	// Input buffer
} NETWORK_MSG_IOCTL, *PNETWORK_MSG_IOCTL;




DWORD NetConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

DWORD NetConn_Connect(const IRPMON_INIT_INFO *Info);
void NetConn_Disconnect(void);
BOOL NetConn_Active(VOID);



#endif
