
#ifndef __VRTULETREE_UTILS_H_
#define __VRTULETREE_UTILS_H_

#include <ntifs.h>
#include "kernel-shared.h"


typedef NTSTATUS (DEVICE_CONDITION_CALLBACK)(PDEVICE_OBJECT DeviceObject, PVOID Context, PVOID ReturnBuffer, ULONG ReturnBufferLength);



typedef struct _CLIENT_TOKEN_INFORMATION {
	UNICODE_STRING UserSid;
	UNICODE_STRING UserName;
	UNICODE_STRING UserDomain;
	ULONG SessionId;
	ULONG IntegrityLevel;
	SID_NAME_USE SidType;
	BOOLEAN Admin;
} CLIENT_TOKEN_INFORMATION, *PCLIENT_TOKEN_INFORMATION;

typedef struct _CLIENT_INFORMATION {
	BOOLEAN Impersonated;
	BOOLEAN CopyOnOpen;
	BOOLEAN EffectiveOnly;
	SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
	CLIENT_TOKEN_INFORMATION PrimaryToken;
	CLIENT_TOKEN_INFORMATION ImpersonationToken;
} CLIENT_INFORMATION, *PCLIENT_INFORMATION;

typedef struct _BASIC_CLIENT_INFO {
	BOOLEAN Admin;
	BOOLEAN Impersonated;
	BOOLEAN ImpersonatedAdmin;
	SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
	BOOLEAN CopyOnOpen;
	BOOLEAN EffectiveOnly;
} BASIC_CLIENT_INFO, *PBASIC_CLIENT_INFO;

#define SystemProcessInformation 5

typedef struct _SYSTEM_PROCESS_INFORMATION_REAL {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER SpareLi1;
	LARGE_INTEGER SpareLi2;
	LARGE_INTEGER SpareLi3;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	LONG BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
} SYSTEM_PROCESS_INFORMATION_REAL, *PSYSTEM_PROCESS_INFORMATION_REAL;


VOID _ReleaseDriverArray(PDRIVER_OBJECT *DriverArray, SIZE_T DriverCount);
VOID _ReleaseDeviceArray(PDEVICE_OBJECT *DeviceArray, SIZE_T ArrayLength);
NTSTATUS _GetObjectName(PVOID Object, PUNICODE_STRING Name);
NTSTATUS _GetDriversInDirectory(PUNICODE_STRING Directory, PDRIVER_OBJECT **DriverArray, PSIZE_T DriverCount);
NTSTATUS UtilsEnumDriverDevices(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT **DeviceArray, PULONG DeviceArrayLength);
NTSTATUS _GetDeviceAddress(PUNICODE_STRING DeviceName, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object);
NTSTATUS GetDriverObjectByName(PUNICODE_STRING Name, PDRIVER_OBJECT *DriverObject);
NTSTATUS VerifyDeviceByAddress(PVOID Address, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object);
NTSTATUS QueryTokenInfo(PACCESS_TOKEN Token, PCLIENT_TOKEN_INFORMATION Info);
NTSTATUS QueryClientInformation(PCLIENT_INFORMATION Client);
void QueryClientBasicInformation(PBASIC_CLIENT_INFO Info);
NTSTATUS UtilsCopyUnicodeString(POOL_TYPE PoolType, PUNICODE_STRING Target, const UNICODE_STRING *Source);

NTSTATUS ProcessEnumerate(PSYSTEM_PROCESS_INFORMATION_REAL *Processes);
VOID ProcessEnumerationFree(PSYSTEM_PROCESS_INFORMATION_REAL Processes);
NTSTATUS ProcessQueryFullImageName(HANDLE ProcessHandle, POOL_TYPE PoolType, PUNICODE_STRING Name);
NTSTATUS ProcessQueryCommandLine(HANDLE ProcessHandle, POOL_TYPE PoolType, PUNICODE_STRING CommandLine);

NTSTATUS FileNameFromFileObject(PFILE_OBJECT FileObject, PUNICODE_STRING Name);

NTSTATUS UtilsGetCurrentControlSetNumber(PULONG Number);

size_t UtilsCaptureStackTrace(size_t FrameCount, void **Frames);

NTSTATUS UtilsModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void UtilsModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);


#endif
