
#include <ntifs.h>
#include <ntddvol.h>
#include "preprocessor.h"
#include "allocator.h"
#include "kernel-shared.h"
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



typedef struct _OBJECT_DIRECTORY_INFORMATION {
   UNICODE_STRING Name;
   UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

__declspec(dllimport) ZWQUERYDIRECTORYOBJECT ZwQueryDirectoryObject;
__declspec(dllimport) OBREFERENCEOBJECTBYNAME ObReferenceObjectByName;
__declspec(dllimport) POBJECT_TYPE *IoDriverObjectType;


/************************************************************************/
/*                             GLOBAL VARIABLES                         */
/************************************************************************/


/************************************************************************/
/* HELPER ROUTINES                                                      */
/************************************************************************/


NTSTATUS _GetDeviceGUIDProperty(PDEVICE_OBJECT DeviceObject, DEVICE_REGISTRY_PROPERTY Property, PGUID Value)
{
   ULONG ReturnLength = 0;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Property=%u; Value=0x%p", DeviceObject, Property, Value);

   RtlZeroMemory(Value, sizeof(GUID));
   Status = IoGetDeviceProperty(DeviceObject, Property, sizeof(GUID), Value, &ReturnLength);
   if (Status == STATUS_INVALID_DEVICE_REQUEST ||
      Status == STATUS_OBJECT_NAME_NOT_FOUND)
      Status = STATUS_SUCCESS;

   DEBUG_EXIT_FUNCTION("0x%x", Status);
   return Status;
}

NTSTATUS _GetWCharDeviceProperty(PDEVICE_OBJECT DeviceObject, DEVICE_REGISTRY_PROPERTY Property, PWCHAR *Buffer, PULONG BufferLength)
{
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   PVOID Tmp = NULL;
   ULONG TmpSize = 64;
   DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Property=%u; Buffer=0x%p; BufferLength=0x%p", DeviceObject, Property, Buffer, BufferLength);

   do {
      if (Tmp != NULL) {
         HeapMemoryFree(Tmp);
         Tmp = NULL;
      }

      Tmp = HeapMemoryAlloc(PagedPool, TmpSize);
      if (Tmp != NULL) {
         Status = IoGetDeviceProperty(DeviceObject, Property, TmpSize, Tmp, &TmpSize);
         if (NT_SUCCESS(Status)) {
            *BufferLength = TmpSize;
            *Buffer = (PWCHAR)Tmp;
         }
      } else Status = STATUS_INSUFFICIENT_RESOURCES;
   } while (Status == STATUS_BUFFER_TOO_SMALL);

   if (!NT_SUCCESS(Status)) {
      if (Tmp != NULL)
         HeapMemoryFree(Tmp);

      if (Status == STATUS_INVALID_DEVICE_REQUEST || 
         Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            *BufferLength = 0;
            Status = STATUS_SUCCESS;
      } else {
         *Buffer = NULL;
         *BufferLength = 0;
      }
   }

   DEBUG_EXIT_FUNCTION("0x%x, *Buffer=0x%p, *BufferLength=%u", Status, *Buffer, *BufferLength);
   return Status;
}



VOID _ReleaseDriverArray(PDRIVER_OBJECT *DriverArray, SIZE_T DriverCount)
{
   ULONG i = 0;
   DEBUG_ENTER_FUNCTION("DriverArray=0x%p; DriverCount=%u", DriverArray, DriverCount);

   if (DriverCount > 0) {
      for (i = 0; i < DriverCount; ++i)
         ObDereferenceObject(DriverArray[i]);

      HeapMemoryFree(DriverArray);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

VOID _ReleaseDeviceArray(PDEVICE_OBJECT *DeviceArray, SIZE_T ArrayLength)
{
   ULONG i = 0;
   DEBUG_ENTER_FUNCTION("DeviceArray=0x%p; ArrayLength=%u", DeviceArray, ArrayLength);

   if (ArrayLength > 0) {
      for (i = 0; i < ArrayLength; ++i)
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
		oni = (POBJECT_NAME_INFORMATION)HeapMemoryAllocNonPaged(oniLen);
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
		Name->Buffer = (PWCH)HeapMemoryAllocNonPaged(sizeof(WCHAR));
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


NTSTATUS _GetDriversInDirectory(PUNICODE_STRING Directory, PDRIVER_OBJECT **DriverArray, PSIZE_T DriverCount)
{
   SIZE_T TmpDriverCount = 0;
   PDRIVER_OBJECT *TmpDriverArray = NULL;
   HANDLE DirectoryHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   UNICODE_STRING DriverTypeStr;
   DEBUG_ENTER_FUNCTION("Directory=%S; DriverArray=0x%p; DriverCount=0x%p", Directory->Buffer, DriverArray, DriverCount);

   *DriverCount = 0;
   *DriverArray = NULL;
   RtlInitUnicodeString(&DriverTypeStr, L"Driver");
   InitializeObjectAttributes(&ObjectAttributes, Directory, OBJ_CASE_INSENSITIVE, NULL, NULL);
   Status = ZwOpenDirectoryObject(&DirectoryHandle, DIRECTORY_QUERY, &ObjectAttributes);
   if (NT_SUCCESS(Status)) {
      ULONG QueryContext = 0;
      UCHAR Buffer [1024];
      POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)&Buffer;

      do {
         RtlZeroMemory(&Buffer, sizeof(Buffer));
         Status = ZwQueryDirectoryObject(DirectoryHandle, DirInfo, sizeof(Buffer), TRUE, FALSE, &QueryContext, NULL);
         if (NT_SUCCESS(Status)) {
            if (RtlCompareUnicodeString(&DirInfo->TypeName, &DriverTypeStr, TRUE) == 0) {
               UNICODE_STRING FullDriverName;

               Status = _AppendDriverNameToDirectory(&FullDriverName, Directory, &DirInfo->Name);
               if (NT_SUCCESS(Status)) {
                  PDRIVER_OBJECT DriverPtr = NULL;

                  Status = ObReferenceObjectByName(&FullDriverName, OBJ_CASE_INSENSITIVE, NULL, GENERIC_READ, *IoDriverObjectType, KernelMode, NULL, (PVOID *)&DriverPtr);
                  if (NT_SUCCESS(Status)) {
                     PDRIVER_OBJECT *Tmp = NULL;

                     Tmp = (PDRIVER_OBJECT *)HeapMemoryAlloc(PagedPool, (TmpDriverCount + 1) * sizeof(PDRIVER_OBJECT));
                     if (Tmp != NULL) {
                        RtlCopyMemory(Tmp, TmpDriverArray, TmpDriverCount * sizeof(PDRIVER_OBJECT));
                        Tmp[TmpDriverCount] = DriverPtr;
                        if (TmpDriverArray != NULL)
                           HeapMemoryFree(TmpDriverArray);

                        TmpDriverArray = Tmp;
                        ++TmpDriverCount;
                     } else Status = STATUS_INSUFFICIENT_RESOURCES;

                     if (Tmp == NULL)
                        ObDereferenceObject(DriverPtr);
                  }

                  HeapMemoryFree(FullDriverName.Buffer);
               }
            }
         }
      } while (NT_SUCCESS(Status));

      if (Status == STATUS_NO_MORE_ENTRIES) {
         *DriverCount = TmpDriverCount;
         *DriverArray = TmpDriverArray;
         Status = STATUS_SUCCESS;
      } else _ReleaseDriverArray(TmpDriverArray, TmpDriverCount);

      ZwClose(DirectoryHandle);
   } else {
      DEBUG_ERROR("ERROR: ZwOpenDirectoryObject: 0x%x", Status);
   }

   DEBUG_EXIT_FUNCTION("0x%x, *DriverArray=0x%p, *DriverCount=%u", Status, *DriverArray, *DriverCount);
   return Status;
}

NTSTATUS _GetLowerUpperDevices(PDEVICE_OBJECT DeviceObject, BOOLEAN Upper, PDEVICE_OBJECT **DeviceArray, PSIZE_T ArrayLength)
{
   PDEVICE_OBJECT *TmpDeviceArray = NULL;
   SIZE_T TmpArrayLength = 0;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   PDEVICE_OBJECT TmpDeviceObject = NULL;
   PDEVICE_OBJECT OldTmpDeviceObject = NULL;
   DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Upper=%u; DeviceArray=0x%p; ArrayLength=0x%p", DeviceObject, Upper, DeviceArray, ArrayLength);

   *DeviceArray = NULL;
   *ArrayLength = 0;
   TmpDeviceObject = Upper ? 
      IoGetAttachedDeviceReference(DeviceObject) :
      IoGetLowerDeviceObject(DeviceObject);

   Status = STATUS_SUCCESS;
   while (((Upper && TmpDeviceObject != DeviceObject) || (!Upper && TmpDeviceObject != NULL))) {
      PDEVICE_OBJECT *Tmp = NULL;

      OldTmpDeviceObject = TmpDeviceObject;
      Tmp = (PDEVICE_OBJECT *)HeapMemoryAlloc(PagedPool, (TmpArrayLength + 1) * sizeof(PDEVICE_OBJECT));
      if (Tmp != NULL) {
         RtlCopyMemory(Tmp, TmpDeviceArray, TmpArrayLength * sizeof(PDEVICE_OBJECT));
         Tmp[TmpArrayLength] = OldTmpDeviceObject;
         TmpArrayLength++;
         if (TmpDeviceArray != NULL)
            HeapMemoryFree(TmpDeviceArray);

         TmpDeviceArray = Tmp;
      } else {
         ObDereferenceObject(OldTmpDeviceObject);
         Status = STATUS_INSUFFICIENT_RESOURCES;
         break;
      }

      TmpDeviceObject = IoGetLowerDeviceObject(TmpDeviceObject);
   }

   if (NT_SUCCESS(Status)) {
      *DeviceArray = TmpDeviceArray;
      *ArrayLength = TmpArrayLength;
      if (TmpDeviceObject == DeviceObject)
         ObDereferenceObject(TmpDeviceObject);
   }

   if (!NT_SUCCESS(Status))
      _ReleaseDeviceArray(TmpDeviceArray, TmpArrayLength);

   DEBUG_EXIT_FUNCTION("0x%x, *DeviceArray=0x%p, *ArrayLength=%u", Status, *DeviceArray, *ArrayLength);
   return Status;
}

NTSTATUS _EnumDriverDevices(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT **DeviceArray, PULONG DeviceArrayLength)
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

               Status = _EnumDriverDevices(TotalArray[i], &DeviceArray, &DeviceArrayLength);
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

static NTSTATUS _FileSystemDeviceForVolume(PDEVICE_OBJECT FileSystemDevice, PVOID Context, PVOID ReturnBuffer, ULONG ReturnBufferLength)
{
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   PDEVICE_OBJECT DiskDevice = NULL;
   DEBUG_ENTER_FUNCTION("FileSystemDevice=0x%p; Context=0x%p; ReturnBuffer=0x%p; ReturnBufferLength=%u", FileSystemDevice, Context, ReturnBuffer, ReturnBufferLength);

   UNREFERENCED_PARAMETER(ReturnBufferLength);
   UNREFERENCED_PARAMETER(ReturnBuffer);

   Status = IoGetDiskDeviceObject(FileSystemDevice, &DiskDevice);
   if (NT_SUCCESS(Status)) {
      DEBUG_PRINT_LOCATION("Disk device object: 0x%p", DiskDevice);
      if (DiskDevice == Context) {
         Status = STATUS_SUCCESS;
      } else Status = STATUS_NOT_FOUND;

      ObDereferenceObject(DiskDevice);
   } else {
     DEBUG_PRINT_LOCATION("IoGetDiskDeviceObject: 0x%x", Status);
      Status = STATUS_NOT_FOUND;
   }

   DEBUG_EXIT_FUNCTION("0x%x", Status);
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


NTSTATUS _GetFileSystemDeviceForVolume(PDEVICE_OBJECT VolumeDevice, PDEVICE_OBJECT *FileSystemDevice)
{
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("VolumeDevice=0x%p; FileSystemDevice=0x%p", VolumeDevice, FileSystemDevice);

   Status = _GetDeviceAddressByCondition(_FileSystemDeviceForVolume, FALSE, TRUE, VolumeDevice, FileSystemDevice, NULL, 0);

   DEBUG_EXIT_FUNCTION("0x%x, *FileSystemDevice=0x%p", Status, *FileSystemDevice);
   return Status;
}


VOID LogError(PDRIVER_OBJECT DriverObject, NTSTATUS Status)
{
   PIO_ERROR_LOG_PACKET packet = NULL;

   packet = IoAllocateErrorLogEntry(DriverObject, sizeof(IO_ERROR_LOG_MESSAGE));
   packet->ErrorCode = Status;
   packet->FinalStatus = Status;
   packet->DumpDataSize = 0;
   packet->NumberOfStrings = 0;
   IoWriteErrorLogEntry(packet);

   return;
}

NTSTATUS QueryDeviceRelations(PDEVICE_OBJECT DeviceObject, DEVICE_RELATION_TYPE RelationType, PDEVICE_RELATIONS *Relations)
{
   KEVENT event;
   PIRP irp = NULL;
   IO_STATUS_BLOCK statusBlock;
   PIO_STACK_LOCATION irpStack = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; RelationType=%u; Relations=0x%p", DeviceObject, RelationType, Relations);

   KeInitializeEvent(&event, NotificationEvent, FALSE);
   irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, DeviceObject, NULL, 0, NULL, &event, &statusBlock);
   if (irp != NULL) {
      statusBlock.Information = 0;
      statusBlock.Status = STATUS_NOT_SUPPORTED;
      irpStack = IoGetNextIrpStackLocation(irp);
      irpStack->MajorFunction = IRP_MJ_PNP;
      irpStack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
      irpStack->Parameters.QueryDeviceRelations.Type = RelationType;
      status = IoCallDriver(DeviceObject, irp);
      if (status == STATUS_PENDING) {
         (VOID) KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
         status = statusBlock.Status;
      }

      if (status == STATUS_SUCCESS) 
         *Relations = (PDEVICE_RELATIONS)statusBlock.Information;
   } else status = STATUS_INSUFFICIENT_RESOURCES;
   

   DEBUG_EXIT_FUNCTION("0x%x, *Relations=0x%p", status, *Relations);
   return status;
}

NTSTATUS GetDriverObjectByName(PUNICODE_STRING Name, PDRIVER_OBJECT *DriverObject)
{
   PDRIVER_OBJECT tmpDriverObject = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Name=0x%p; DriverObject=0x%p", Name, DriverObject);

   status = ObReferenceObjectByName(Name, 0, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &tmpDriverObject);
   if (NT_SUCCESS(status)) {
      *DriverObject = tmpDriverObject;
   }

   DEBUG_EXIT_FUNCTION("0x%x, *DriverObject=0x%p", status, *DriverObject);
   return status;
}
