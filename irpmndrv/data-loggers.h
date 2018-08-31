
#ifndef __DATA_LOGGERS_H__
#define __DATA_LOGGERS_H__


typedef enum _EDataLoggerResultType {
	dlrtNoBuffers,
	dlrtKernelMode,
	dlrtUserMode,
} EDataLoggerResultType, *PEDataLoggerResultType;


typedef struct _DATA_LOGGER_RESULT {
	EDataLoggerResultType Type;
	void *Buffer;
	ULONG BufferSize;
	PMDL BufferMdl;
} DATA_LOGGER_RESULT, *PDATA_LOGGER_RESULT;



void IRPDataLogger(PIRP Irp, BOOLEAN Completion, PDATA_LOGGER_RESULT Result);



#endif
