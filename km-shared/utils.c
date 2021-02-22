
#include <ntifs.h>
#include <fltKernel.h>
#include <ntddvol.h>
#include "preprocessor.h"
#include "allocator.h"
#include "kernel-shared.h"
#include "utils-dym-array.h"
#include "thread-context.h"
#include "utils.h"

#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0

typedef NTSTATUS (NTAPI ZWQUERYDIRECTORYOBJECT)(
   HANDLE DirectoryHandle,
   PVOID Buffer,
   ULONG Length,
   BOOLEAN ReturnSingleEntry,
   BOOLEAN RestartScan,
   PULONG Context,
   PULONG ReturnLength);

typedef NTSTATUS (NTAPI OBREFERENCEOBJECTBYNAME) (
   PUNICODE_STRING ObjectPath,
   ULONG Attributes,
   PACCESS_STATE PassedAccessState OPTIONAL,
   ACCESS_MASK DesiredAccess OPTIONAL,
   POBJECT_TYPE ObjectType,
   KPROCESSOR_MODE AccessMode,
   PVOID ParseContext OPTIONAL,
   PVOID *ObjectPtr); 

typedef NTSTATUS(NTAPI ZWQUERYINFORMATIONPROCESS)(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength);

typedef NTSTATUS (NTAPI ZWQUERYSYSTEMINFORMATION)(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength);

typedef struct _OBJECT_DIRECTORY_INFORMATION {
   UNICODE_STRING Name;
   UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef POBJECT_TYPE (OBGETOBJECTTYPE)(PVOID Object);

__declspec(dllimport) ZWQUERYDIRECTORYOBJECT ZwQueryDirectoryObject;
__declspec(dllimport) OBREFERENCEOBJECTBYNAME ObReferenceObjectByName;
__declspec(dllimport) ZWQUERYSYSTEMINFORMATION ZwQuerySystemInformation;
__declspec(dllimport) ZWQUERYINFORMATIONPROCESS ZwQueryInformationProcess;

static POBJECT_TYPE *_IoDriverObjectType = NULL;


/************************************************************************/
/*                             GLOBAL VARIABLES                         */
/************************************************************************/


/************************************************************************/
/* HELPER ROUTINES                                                      */
/************************************************************************/






VOID _ReleaseDriverArray(PDRIVER_OBJECT *DriverArray, SIZE_T DriverCount)
{
	DEBUG_ENTER_FUNCTION("DriverArray=0x%p; DriverCount=%u", DriverArray, DriverCount);

	if (DriverCount > 0) {
		for (ULONG i = 0; i < DriverCount; ++i)
			ObDereferenceObject(DriverArray[i]);

		HeapMemoryFree(DriverArray);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID _ReleaseDeviceArray(PDEVICE_OBJECT *DeviceArray, SIZE_T ArrayLength)
{
	DEBUG_ENTER_FUNCTION("DeviceArray=0x%p; ArrayLength=%u", DeviceArray, ArrayLength);

	if (ArrayLength > 0) {
		for (ULONG i = 0; i < ArrayLength; ++i)
			ObDereferenceObject(DeviceArray[i]);

		HeapMemoryFree(DeviceArray);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS _GetObjectName(PVOID Object, PUNICODE_STRING Name)
{
	ULONG oniLen = 0;
	POBJECT_NAME_INFORMATION oni = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Object=0x%p; Name=0x%p", Object, Name);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	status = ObQueryNameString(Object, NULL, 0, &oniLen);
	if (status == STATUS_INFO_LENGTH_MISMATCH) {
		oniLen += sizeof(OBJECT_NAME_INFORMATION) + sizeof(WCHAR);
		oni = HeapMemoryAllocNonPaged(oniLen);
		if (oni != NULL) {
			status = ObQueryNameString(Object, oni, oniLen, &oniLen);
			if (NT_SUCCESS(status)) {
				Name->Length = oni->Name.Length;
				Name->MaximumLength = Name->Length + sizeof(WCHAR);
				Name->Buffer = (PWCH)HeapMemoryAllocNonPaged(oni->Name.Length + sizeof(WCHAR));
				if (Name->Buffer != NULL) {
					memcpy(Name->Buffer, oni->Name.Buffer, oni->Name.Length);
					Name->Buffer[oni->Name.Length / sizeof(WCHAR)] = L'\0';
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			}

			HeapMemoryFree(oni);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else if (NT_SUCCESS(status)) {
		Name->Length = 0;
		Name->MaximumLength = sizeof(WCHAR);
		Name->Buffer = HeapMemoryAllocNonPaged(sizeof(WCHAR));
		if (Name->Buffer != NULL) {
			Name->Buffer[0] = L'\0';
			status = STATUS_SUCCESS;
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _AppendDriverNameToDirectory(PUNICODE_STRING Dest, PUNICODE_STRING Src1, PUNICODE_STRING Src2)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Dest=0x%p; Src1=%S; Src2=%S", Dest, Src1->Buffer, Src2->Buffer);

	Dest->Length = Src1->Length + sizeof(WCHAR) + Src2->Length;
	Dest->MaximumLength = Dest->Length;
	Dest->Buffer = (PWSTR)HeapMemoryAlloc(PagedPool, Dest->Length + sizeof(WCHAR));
	if (Dest->Buffer != NULL) {
		RtlZeroMemory(Dest->Buffer, Dest->Length + sizeof(WCHAR));
		RtlCopyMemory(Dest->Buffer, Src1->Buffer, Src1->Length);
		Dest->Buffer[Src1->Length / sizeof(WCHAR)] = L'\\';
		RtlCopyMemory(&Dest->Buffer[(Src1->Length / sizeof(WCHAR)) + 1], Src2->Buffer, Src2->Length);
		Status = STATUS_SUCCESS;
	} else Status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Dest=%S", Status, Dest->Buffer);
	return Status;
}


typedef struct _OBJECT_DIRECTORY_ENTRY {
	struct _OBJECT_DIRECTORY_ENTRY *Next;
	PVOID Object;
	ULONG Hash;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

typedef struct _OBJECT_DIRECTORY {
	POBJECT_DIRECTORY_ENTRY Buckets[37];
	EX_PUSH_LOCK Lock;
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

NTSTATUS _GetDriversInDirectory(PUNICODE_STRING Directory, PDRIVER_OBJECT **DriverArray, PSIZE_T DriverCount)
{
	SIZE_T tmpDriverCount = 0;
	PDRIVER_OBJECT *tmpDriverArray = NULL;
	HANDLE hDirectory = NULL;
	PUTILS_DYM_ARRAY driverArray = NULL;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING uDriverTypeString;
	DEBUG_ENTER_FUNCTION("Directory=%S; DriverArray=0x%p; DriverCount=0x%p", Directory->Buffer, DriverArray, DriverCount);

	*DriverCount = 0;
	*DriverArray = NULL;
	status = DymArrayCreate(PagedPool, &driverArray);
	if (NT_SUCCESS(status)) {
		RtlInitUnicodeString(&uDriverTypeString, L"Driver");
		InitializeObjectAttributes(&oa, Directory, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		status = ZwOpenDirectoryObject(&hDirectory, DIRECTORY_QUERY, &oa);
		if (NT_SUCCESS(status)) {
			ULONG QueryContext = 0;
			UCHAR Buffer[1024];
			POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)&Buffer;

			do {
				RtlZeroMemory(&Buffer, sizeof(Buffer));
				status = ZwQueryDirectoryObject(hDirectory, DirInfo, sizeof(Buffer), TRUE, FALSE, &QueryContext, NULL);
				if (NT_SUCCESS(status)) {
					if (RtlEqualUnicodeString(&DirInfo->TypeName, &uDriverTypeString, TRUE)) {
						UNICODE_STRING FullDriverName;

						status = _AppendDriverNameToDirectory(&FullDriverName, Directory, &DirInfo->Name);
						if (NT_SUCCESS(status)) {
							PDRIVER_OBJECT DriverPtr = NULL;

							status = ObReferenceObjectByName(&FullDriverName, OBJ_CASE_INSENSITIVE, NULL, GENERIC_READ, *_IoDriverObjectType, KernelMode, NULL, (PVOID *)&DriverPtr);
							if (NT_SUCCESS(status)) {
								status = DymArrayPushBack(driverArray, DriverPtr);
								if (!NT_SUCCESS(status))
									ObDereferenceObject(DriverPtr);
							}

							HeapMemoryFree(FullDriverName.Buffer);
						}
					}
				}
			} while (NT_SUCCESS(status));

			if (status == STATUS_NO_MORE_ENTRIES) {
				tmpDriverCount = DymArrayLength(driverArray);
				tmpDriverArray = HeapMemoryAllocPaged(tmpDriverCount*sizeof(PDRIVER_OBJECT));
				if (tmpDriverArray != NULL) {
					for (SIZE_T i = 0; i < DymArrayLength(driverArray); ++i)
						tmpDriverArray[i] = (PDRIVER_OBJECT)DymArrayItem(driverArray, i);

					*DriverCount = tmpDriverCount;
					*DriverArray = tmpDriverArray;
					status = STATUS_SUCCESS;
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			}

			ZwClose(hDirectory);
		}

		if (!NT_SUCCESS(status)) {
			for (SIZE_T i = 0; i < DymArrayLength(driverArray); ++i)
				ObDereferenceObject(DymArrayItem(driverArray, i));
		}

		DymArrayDestroy(driverArray);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *DriverArray=0x%p, *DriverCount=%zu", status, *DriverArray, *DriverCount);
	return status;
}


NTSTATUS UtilsEnumDriverDevices(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT **DeviceArray, PULONG DeviceArrayLength)
{
	ULONG TmpArrayLength = 0;
	PDEVICE_OBJECT *TmpDeviceArray = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; DeviceArray=0x%p; DeviceArrayLength=0x%p", DriverObject, DeviceArray, DeviceArrayLength);

	do {
		Status = IoEnumerateDeviceObjectList(DriverObject, TmpDeviceArray, TmpArrayLength * sizeof(PDEVICE_OBJECT), &TmpArrayLength);
		if (Status == STATUS_BUFFER_TOO_SMALL) {
			if (TmpDeviceArray != NULL)
				HeapMemoryFree(TmpDeviceArray);

			TmpDeviceArray = (PDEVICE_OBJECT *)HeapMemoryAlloc(NonPagedPool, TmpArrayLength * sizeof(PDEVICE_OBJECT));
			if (TmpDeviceArray == NULL)
				Status = STATUS_INSUFFICIENT_RESOURCES;
		}
	} while (Status == STATUS_BUFFER_TOO_SMALL);

	if (NT_SUCCESS(Status)) {
		*DeviceArrayLength = TmpArrayLength;
		*DeviceArray = TmpDeviceArray;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *DeviceArray=0x%p, *DeviceArrayLength=%u", Status, *DeviceArray, *DeviceArrayLength);
	return Status;
}


NTSTATUS _GetDeviceAddressByCondition(DEVICE_CONDITION_CALLBACK *Callback, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PVOID Context, PDEVICE_OBJECT *DeviceAddress, PVOID ReturnBuffer, ULONG ReturnBufferLength)
{
   PDRIVER_OBJECT *DriverArray = NULL;
   SIZE_T DriverArrayLength = 0;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Callback=0x%p; SearchDrivers=%d; SearchFileSystems=%d; Context=0x%p; DeviceAddress=0x%p; ReturnBuffer=0x%p; ReturnBufferLength=%d", Callback, SearchDrivers, SearchFileSystems, Context, DeviceAddress, ReturnBuffer, ReturnBufferLength);

   Status = STATUS_NOT_FOUND;
   *DeviceAddress = NULL;
   if (SearchDrivers) {
      UNICODE_STRING Directory;

      RtlInitUnicodeString(&Directory, L"\\Driver");
      Status = _GetDriversInDirectory(&Directory, &DriverArray, &DriverArrayLength);
   } else {
      Status = STATUS_SUCCESS;
   }

   if (NT_SUCCESS(Status)) {
      PDRIVER_OBJECT *FileSystemArray = NULL;
      SIZE_T FileSystemArrayLength = 0;

      if (SearchFileSystems) {
         UNICODE_STRING Directory;

         RtlInitUnicodeString(&Directory, L"\\FileSystem");
         Status = _GetDriversInDirectory(&Directory, &FileSystemArray, &FileSystemArrayLength);
      }

      if (NT_SUCCESS(Status)) {
         PDRIVER_OBJECT *TotalArray = NULL;
         SIZE_T TotalArrayLength = 0;

         TotalArrayLength = DriverArrayLength + FileSystemArrayLength;
         TotalArray = (PDRIVER_OBJECT *)HeapMemoryAlloc(NonPagedPool, TotalArrayLength * sizeof(PDRIVER_OBJECT));
         if (TotalArray != NULL) {
            ULONG i = 0;

            RtlCopyMemory(TotalArray, DriverArray, DriverArrayLength * sizeof(PDRIVER_OBJECT));
            RtlCopyMemory(&TotalArray[DriverArrayLength], FileSystemArray, FileSystemArrayLength * sizeof(PDRIVER_OBJECT));
            for (i = 0; i < TotalArrayLength; ++i) {
               BOOLEAN Finish = FALSE;
               PDEVICE_OBJECT *DeviceArray = NULL;
               ULONG DeviceArrayLength = 0;

               Status = UtilsEnumDriverDevices(TotalArray[i], &DeviceArray, &DeviceArrayLength);
               if (NT_SUCCESS(Status)) {
                  ULONG j = 0;

                  for (j = 0; j < DeviceArrayLength; ++j) {
                     Status = Callback(DeviceArray[j], Context, ReturnBuffer, ReturnBufferLength);
                     if (NT_SUCCESS(Status)) {
                        Finish = TRUE;
                        *DeviceAddress = DeviceArray[j];
                        ObReferenceObject(*DeviceAddress);
                     } else {
                        Finish = Status != STATUS_NOT_FOUND;
                     }
                     
                     if (Finish) {
                        break;
                     }
                  }

                  _ReleaseDeviceArray(DeviceArray, DeviceArrayLength);
               } else {
                  Finish = TRUE;
               }

               if (Finish)
                  break;
            }

            if (*DeviceAddress == NULL)
               Status = STATUS_NOT_FOUND;

            HeapMemoryFree(TotalArray);
         }

         _ReleaseDriverArray(FileSystemArray, FileSystemArrayLength);
      }

      _ReleaseDriverArray(DriverArray, DriverArrayLength);
   }

   DEBUG_EXIT_FUNCTION("0x%x, *DeviceAddress=0x%p", Status, *DeviceAddress);
   return Status;
}

static NTSTATUS _DeviceByNameCondition(PDEVICE_OBJECT DeviceObject, PVOID Context, PVOID ReturnBuffer, ULONG ReturnBufferLength)
{                     
   UNICODE_STRING DeviceName;
   PUNICODE_STRING TargetDeviceName = (PUNICODE_STRING)Context;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;

   UNREFERENCED_PARAMETER(ReturnBufferLength);
   UNREFERENCED_PARAMETER(ReturnBuffer);

   Status = _GetObjectName(DeviceObject, &DeviceName);
   if (NT_SUCCESS(Status)) {
      if (RtlCompareUnicodeString(&DeviceName, TargetDeviceName, TRUE) == 0) {
         Status = STATUS_SUCCESS;
      } else {
         Status = STATUS_NOT_FOUND;
      }

      HeapMemoryFree(DeviceName.Buffer);
   }

   return Status;
}


static NTSTATUS _DeviceByAddressCondition(PDEVICE_OBJECT DeviceObject, PVOID Context, PVOID ReturnBuffer, ULONG ReturnBufferLength)
{                     
	PDEVICE_OBJECT targetDeviceAddress = (PDEVICE_OBJECT)Context;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	UNREFERENCED_PARAMETER(ReturnBufferLength);
	UNREFERENCED_PARAMETER(ReturnBuffer);
	Status = STATUS_NOT_FOUND;
	if (targetDeviceAddress == DeviceObject)
		Status = STATUS_SUCCESS;

	return Status;
}


NTSTATUS _GetDeviceAddress(PUNICODE_STRING DeviceName, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object)
{
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("DeviceName=%S; SearchDrivers=%d; SearchFileSystems=%d; Object=0x%p", DeviceName->Buffer, SearchDrivers, SearchFileSystems, Object);

   if (DeviceName->Length > 0) {
	   Status = STATUS_NOT_FOUND;
      Status = _GetDeviceAddressByCondition(_DeviceByNameCondition, SearchDrivers, SearchFileSystems, DeviceName, Object, NULL, 0);
   } else Status = STATUS_NOT_FOUND;

   DEBUG_EXIT_FUNCTION("0x%x, *Object=0x%p", Status, *Object);
   return Status;
}


NTSTATUS VerifyDeviceByAddress(PVOID Address, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Address=0x%p; SearchDrivers=%d; SearchFileSystems=%d; Object=0x%p", Address, SearchDrivers, SearchFileSystems, Object);

	Status = _GetDeviceAddressByCondition(_DeviceByAddressCondition, SearchDrivers, SearchFileSystems, Address, Object, NULL, 0);

	DEBUG_EXIT_FUNCTION("0x%x, *Object=0x%p", Status, *Object);
	return Status;
}


NTSTATUS GetDriverObjectByName(PUNICODE_STRING Name, PDRIVER_OBJECT *DriverObject)
{
	PDRIVER_OBJECT tmpDriverObject = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Name=0x%p; DriverObject=0x%p", Name, DriverObject);

	status = ObReferenceObjectByName(Name, 0, NULL, 0, *_IoDriverObjectType, KernelMode, NULL, &tmpDriverObject);
	if (NT_SUCCESS(status))
		*DriverObject = tmpDriverObject;

	DEBUG_EXIT_FUNCTION("0x%x, *DriverObject=0x%p", status, *DriverObject);
	return status;
}


NTSTATUS QueryTokenInfo(PACCESS_TOKEN Token, PCLIENT_TOKEN_INFORMATION Info)
{
	ULONG userNameLen = 0;
	ULONG domainNameLen = 0;
	unsigned char userNameBuffer[300];
	unsigned char domainNameBuffer[300];
	PUNICODE_STRING userName = (PUNICODE_STRING)userNameBuffer;
	PUNICODE_STRING domainName = (PUNICODE_STRING)domainNameBuffer;
	PTOKEN_USER tokenUser = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Token=0x%p; Info=0x%p", Token, Info);

	memset(Info, 0, sizeof(CLIENT_TOKEN_INFORMATION));
	Info->Admin = SeTokenIsAdmin(Token);
	status = SeQueryInformationToken(Token, TokenSessionId, (void **)&Info->SessionId);
	if (NT_SUCCESS(status))
		status = SeQueryInformationToken(Token, TokenIntegrityLevel, (void **)&Info->IntegrityLevel);

	if (NT_SUCCESS(status)) {
		status = SeQueryInformationToken(Token, TokenUser, &tokenUser);
		if (NT_SUCCESS(status)) {
			status = RtlConvertSidToUnicodeString(&Info->UserSid, tokenUser->User.Sid, TRUE);
			if (NT_SUCCESS(status)) {
				userNameLen = sizeof(userNameBuffer);
				domainNameLen = sizeof(domainNameBuffer);
				status = SecLookupAccountSid(tokenUser->User.Sid, &userNameLen, userName, &domainNameLen, domainName, &Info->SidType);
				if (NT_SUCCESS(status)) {

				}
			}

			ExFreePool(tokenUser);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS QueryClientInformation(PCLIENT_INFORMATION Client)
{
	PACCESS_TOKEN token = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Client=0x%p", Client);

	status = STATUS_SUCCESS;
	token = PsReferenceImpersonationToken(PsGetCurrentThread(), &Client->CopyOnOpen, &Client->EffectiveOnly, &Client->ImpersonationLevel);
	Client->Impersonated = (token != NULL);
	if (token != NULL) {
		status = QueryTokenInfo(token, &Client->ImpersonationToken);
		PsDereferenceImpersonationToken(token);
	} 
		
	token = PsReferencePrimaryToken(PsGetCurrentProcess());
	if (token != NULL) {
		status = QueryTokenInfo(token, &Client->PrimaryToken);
		PsDereferencePrimaryToken(token);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS UtilsCopyUnicodeString(POOL_TYPE PoolType, PUNICODE_STRING Target, const UNICODE_STRING *Source)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("PoolType=%u; Target=0x%p; Source=\"%wZ\"", PoolType, Target, Source);

	status = STATUS_SUCCESS;
	memset(Target, 0, sizeof(UNICODE_STRING));
	Target->Length = Source->Length;
	Target->MaximumLength = Target->MaximumLength;
	if (Target->Length > 0) {
		Target->Buffer = HeapMemoryAlloc(PoolType, Target->Length);
		if (Target->Buffer != NULL)
			memcpy(Target->Buffer, Source->Buffer, Target->Length);
		else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void QueryClientBasicInformation(PBASIC_CLIENT_INFO Info)
{
	PACCESS_TOKEN token = NULL;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	memset(Info, 0, sizeof(BASIC_CLIENT_INFO));
	token = PsReferenceImpersonationToken(PsGetCurrentThread(), &Info->CopyOnOpen, &Info->EffectiveOnly, &Info->ImpersonationLevel);
	if (token != NULL) {
		Info->Impersonated = TRUE;
		Info->ImpersonatedAdmin = SeTokenIsAdmin(token);
		PsDereferenceImpersonationToken(token);
	}

	token = PsReferencePrimaryToken(PsGetCurrentProcess());
	if (token != NULL) {
		Info->Admin = SeTokenIsAdmin(token);
		PsDereferencePrimaryToken(token);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS ProcessEnumerate(PSYSTEM_PROCESS_INFORMATION_REAL *Processes)
{
	ULONG retLen = 0;
	ULONG spiSize = 64;
	PSYSTEM_PROCESS_INFORMATION_REAL spi = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Processes=0x%p", Processes);

	status = STATUS_INFO_LENGTH_MISMATCH;
	do {
		spi = HeapMemoryAllocPaged(spiSize);
		if (spi != NULL) {
			status = ZwQuerySystemInformation(SystemProcessInformation, spi, spiSize, &retLen);
			if (status == STATUS_INFO_LENGTH_MISMATCH) {
				HeapMemoryFree(spi);
				spiSize *= 2;
			}
		}
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	if (NT_SUCCESS(status))
		*Processes = spi;

	DEBUG_EXIT_FUNCTION("0x%x", status, *Processes);
	return status;
}


VOID ProcessEnumerationFree(PSYSTEM_PROCESS_INFORMATION_REAL Processes)
{
	DEBUG_ENTER_FUNCTION("Processes=0x%p", Processes);

	HeapMemoryFree(Processes);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS ProcessQueryFullImageName(HANDLE ProcessHandle, POOL_TYPE PoolType, PUNICODE_STRING Name)
{
	ULONG retLength = 0;
	ULONG pniSize = 512;
	PUNICODE_STRING pni = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ProcessHandle=0x%p; PoolType=%u; Name=0x%p", ProcessHandle, PoolType, Name);

	do {
		if (pni != NULL) {
			HeapMemoryFree(pni);
			pniSize *= 2;
		}

		pni = HeapMemoryAllocPaged(pniSize);
		if (pni != NULL)
			status = ZwQueryInformationProcess(ProcessHandle, ProcessImageFileName, pni, pniSize, &retLength);
		else status = STATUS_INSUFFICIENT_RESOURCES;
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	if (NT_SUCCESS(status))
		status = UtilsCopyUnicodeString(PoolType, Name, pni);

	if (pni != NULL)
		HeapMemoryFree(pni);

	DEBUG_EXIT_FUNCTION("0x%x, Name=\"%wZ\"", status, Name);
	return status;
}


NTSTATUS ProcessQueryCommandLine(HANDLE ProcessHandle, POOL_TYPE PoolType, PUNICODE_STRING CommandLine)
{
	ULONG retLength = 0;
	ULONG pcliSize = 512;
	PUNICODE_STRING pcli = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ProcessHandle=0x%p; PoolType=%u; CommandLine=0x%p", ProcessHandle, PoolType, CommandLine);

	do {
		if (pcli != NULL) {
			HeapMemoryFree(pcli);
			pcliSize *= 2;
		}

		pcli = HeapMemoryAllocPaged(pcliSize);
		if (pcli != NULL)
			status = ZwQueryInformationProcess(ProcessHandle, ProcessCommandLineInformation, pcli, pcliSize, &retLength);
		else status = STATUS_INSUFFICIENT_RESOURCES;
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	if (NT_SUCCESS(status))
		status = UtilsCopyUnicodeString(PoolType, CommandLine, pcli);

	if (pcli != NULL)
		HeapMemoryFree(pcli);

	DEBUG_EXIT_FUNCTION("0x%x, CommandLine=\"%wZ\"", status, CommandLine);
	return status;
}


NTSTATUS FileNameFromFileObject(PFILE_OBJECT FileObject, PUNICODE_STRING Name)
{
	BOOLEAN delimited = FALSE;
	UNICODE_STRING uName;
	UNICODE_STRING uRelatedName;
	wchar_t *tmp = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("FileObject=0x%p; Name=0x%p", FileObject, Name);

	status = STATUS_SUCCESS;
	memset(&uName, 0, sizeof(uName));
	memset(&uRelatedName, 0, sizeof(uRelatedName));
	uName = FileObject->FileName;
	if (FileObject->RelatedFileObject != NULL)
		status = _GetObjectName(FileObject->RelatedFileObject, &uRelatedName);

	if (NT_SUCCESS(status)) {
		delimited = (uRelatedName.Length > 0 && uRelatedName.Buffer[uRelatedName.Length / sizeof(wchar_t) - 1] != L'\\');
		Name->Length = uRelatedName.Length + uName.Length;
		if (delimited)
			Name->Length += sizeof(wchar_t);

		Name->MaximumLength = Name->Length;
		Name->Buffer = HeapMemoryAllocPaged(Name->Length);
		if (Name->Buffer != NULL) {
			tmp = Name->Buffer;
			memcpy(tmp, uRelatedName.Buffer, uRelatedName.Length);
			tmp += (uRelatedName.Length / sizeof(wchar_t));
			if (delimited) {
				*tmp = L'\\';
				++tmp;
			}

			memcpy(tmp, uName.Buffer, uName.Length);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	
		if (!NT_SUCCESS(status))
			memset(Name, 0, sizeof(UNICODE_STRING));

		if (uRelatedName.Buffer != NULL)
			HeapMemoryFree(uRelatedName.Buffer);
	}

	DEBUG_EXIT_FUNCTION("0x%x, Name=\"%wZ\"", status, Name);
	return status;
}


NTSTATUS UtilsGetCurrentControlSetNumber(PULONG Number)
{
	HANDLE hSelectKey = NULL;
	UNICODE_STRING uSelectKey;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Number=0x%p", Number);

	RtlInitUnicodeString(&uSelectKey, L"\\Registry\\Machine\\SYSTEM\\Select");
	InitializeObjectAttributes(&oa, &uSelectKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&hSelectKey, KEY_QUERY_VALUE, &oa);
	if (NT_SUCCESS(status)) {
		ULONG retLength = 0;
		UNICODE_STRING uValueName;
		UCHAR kvpiStorage[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
		PKEY_VALUE_PARTIAL_INFORMATION kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)kvpiStorage;

		RtlInitUnicodeString(&uValueName, L"Current");
		status = ZwQueryValueKey(hSelectKey, &uValueName, KeyValuePartialInformation, kvpi, sizeof(kvpiStorage), &retLength);
		if (NT_SUCCESS(status)) {
			if (kvpi->DataLength == sizeof(ULONG))
				*Number = *(PULONG)kvpi->Data;
			else status = STATUS_INVALID_PARAMETER;
		}

		ZwClose(hSelectKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Number=%u", status, *Number);
	return status;
}


size_t UtilsCaptureStackTrace(size_t FrameCount, void **Frames)
{
	size_t ret = 0;
	PTHREAD_CONTEXT_RECORD threadContext = NULL;
	LONG count = 0;

	RtlSecureZeroMemory(Frames, FrameCount * sizeof(void*));
	if (KeGetCurrentIrql() == PASSIVE_LEVEL &&
		ExGetPreviousMode() == UserMode) {
		threadContext = ThreadContextGet();
		if (threadContext != NULL) {
			count = InterlockedIncrement(&threadContext->StackTraceInProgress);
			if (count == 1) {
				ret = RtlWalkFrameChain(Frames, (ULONG)FrameCount, 1);
				if (ret != 0)
					ret = FrameCount;
			}

			InterlockedDecrement(&threadContext->StackTraceInProgress);
			ThreadContextDereference(threadContext);
		}
	}

	return ret;
}


NTSTATUS UtilsModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	UNICODE_STRING uRoutineName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	status = STATUS_SUCCESS;
	RtlInitUnicodeString(&uRoutineName, L"IoDriverObjectType");
	_IoDriverObjectType = (POBJECT_TYPE*)MmGetSystemRoutineAddress(&uRoutineName);
	if (_IoDriverObjectType == NULL)
		status = STATUS_NOT_FOUND;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void UtilsModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	_IoDriverObjectType = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
