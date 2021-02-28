
#ifndef __HYPERV_CONNECTOR_H__
#define __HYPERV_CONNECTOR_H__



#include <stdint.h>
#include <windows.h>
#include "irpmondll-types.h"

#define IOCTL_IRPMON_SERVER_ARCH_32BIT				1
#define IOCTL_IRPMON_SERVER_ARCH_64BIT				2

typedef struct _HYPERV_MSG_IOCTL {
	uint32_t Result;
	uint32_t ControlCode;
	uint32_t InputBufferSize;
	uint32_t OutputBufferSize;
	// Input buffer
} HYPERV_MSG_IOCTL, *PHYPERV_MSG_IOCTL;



#ifdef __cplusplus
extern "C" {
#endif

DWORD HyperVConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

DWORD HyperVConn_Connect(const IRPMON_INIT_INFO *Info);
void HyperVConn_Disconnect(void);
BOOL HyperVConn_Active(void);

#ifdef __cplusplus
}
#endif


#endif
