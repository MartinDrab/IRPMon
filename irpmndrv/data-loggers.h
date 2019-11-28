
#ifndef __DATA_LOGGERS_H__
#define __DATA_LOGGERS_H__


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
	BOOLEAN Admin;
	BOOLEAN Impersonated;
	BOOLEAN ImpersonatedAdmin;
} DATA_LOGGER_RESULT, *PDATA_LOGGER_RESULT;



void IRPDataLogger(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN Completion, PDATA_LOGGER_RESULT Result);
void IRPDataLoggerSetRequestFlags(PREQUEST_HEADER Request, const DATA_LOGGER_RESULT *Data);
void DataLoggerResultRelease(PDATA_LOGGER_RESULT Result);



#endif
