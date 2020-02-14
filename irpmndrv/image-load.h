
#ifndef __IMAGE_LOAD_H__
#define __IMAGE_LOAD_H__


#include <ntifs.h>





NTSTATUS ImageLoadModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void ImageLoadModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
