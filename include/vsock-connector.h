
#ifndef __VSOCK_CONNECTOR_H__
#define __VSOCK_CONNECTOR_H__



#include <stdint.h>
#include <windows.h>
#include "irpmondll-types.h"

#define IOCTL_IRPMON_SERVER_ARCH_32BIT				1
#define IOCTL_IRPMON_SERVER_ARCH_64BIT				2

typedef struct _VSOCK_MSG_IOCTL {
	uint32_t Result;
	uint32_t ControlCode;
	uint32_t InputBufferSize;
	uint32_t OutputBufferSize;
	// Input buffer
} VSOCK_MSG_IOCTL, *PVSOCK_MSG_IOCTL;



#ifdef __cplusplus
extern "C" {
#endif

DWORD VSockConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);

DWORD VSockConn_Connect(const IRPMON_INIT_INFO *Info);
void VSockConn_Disconnect(void);
BOOL VSockConn_Active(void);
unsigned int VSockConn_LocalId(void);
unsigned int VSockConn_VMCIVersion(void);

#ifdef __cplusplus
}
#endif


#endif
