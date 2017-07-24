
#ifndef __LIBFILTERS_H__
#define __LIBFILTERS_H__


#include <ntifs.h>



typedef enum _EFilterDriverType {
	fdtLower,
	fdtUpper,
} EFilterDriverType, *PEFilterDriverType;



NTSTATUS LibFiltersFilterInstall(_In_ PUNICODE_STRING ClassGuid, _In_ PUNICODE_STRING ServiceName, _In_ EFilterDriverType Type, _In_ BOOLEAN First);
VOID LibFiltersFilterUninstall(_In_ PUNICODE_STRING ClassGuid, EFilterDriverType Type);

NTSTATUS LibFiltersModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context);
VOID LibFiltersModuleFinit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context);




#endif
