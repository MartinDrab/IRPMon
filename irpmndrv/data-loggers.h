
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
} DATA_LOGGER_RESULT, *PDATA_LOGGER_RESULT;



void IRPDataLogger(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN Completion, PDATA_LOGGER_RESULT Result);



#endif
