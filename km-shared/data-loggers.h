
#ifndef __DATA_LOGGERS_H__
#define __DATA_LOGGERS_H__


#include "general-types.h"


typedef enum _EDataLoggerResultType {
	dlrtNoBuffers,
	dlrtKernelMode,
	dlrtUserMode,
} EDataLoggerResultType, *PEDataLoggerResultType;


typedef struct _DATA_LOGGER_RESULT {
	void *Buffer;
	SIZE_T BufferSize;
	PMDL BufferMdl;
	BOOLEAN Stripped;
	BOOLEAN BufferAllocated;
} DATA_LOGGER_RESULT, *PDATA_LOGGER_RESULT;



void IRPDataLogger(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN Completion, PDATA_LOGGER_RESULT Result);
void IRPDataLoggerSetRequestFlags(PREQUEST_HEADER Request, const DATA_LOGGER_RESULT *Data);
void DataLoggerResultRelease(PDATA_LOGGER_RESULT Result);

NTSTATUS DataLoggerModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void DataLoggerModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
