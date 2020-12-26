
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils.h"
#include "hook.h"
#include "hook-handlers.h"
#include "kernel-shared.h"
#include "ioctls.h"
#include "modules.h"
#include "req-queue.h"
#include "um-services.h"
#include "pnp-driver-watch.h"
#include "process-events.h"
#include "req-queue.h"
#include "regman.h"
#include "data-loggers.h"
#include "driver-settings.h"
#include "devext-hooks.h"
#include "image-load.h"
#include "boot-log.h"
#include "driver.h"


/************************************************************************/
/*                             GLOBAL VARIABLES                         */
/************************************************************************/


static ERESOURCE _createCloseLock;
static volatile LONG _openHandles = 0;
static PDEVICE_OBJECT _controlDeviceObject = NULL;

/************************************************************************/
/*                            HELPER FUNCTIONS                          */
/************************************************************************/

#if defined(_AMD64_) || defined(_IA64_)

#define PROCESS_QUERY_INFORMATION			0x0400

typedef NTSTATUS (NTAPI ZWQUERYINFORMATIONPROCESS)(
	_In_      HANDLE           ProcessHandle,
	_In_      PROCESSINFOCLASS ProcessInformationClass,
	_Out_     PVOID            ProcessInformation,
	_In_      ULONG            ProcessInformationLength,
	_Out_opt_ PULONG           ReturnLength
);

__declspec(dllimport) ZWQUERYINFORMATIONPROCESS ZwQueryInformationProcess;


static NTSTATUS _Wow64Check(void)
{
	HANDLE hProcess = NULL;
	ULONG_PTR isWow64 = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	status = ObOpenObjectByPointer(PsGetCurrentProcess(), OBJ_KERNEL_HANDLE, NULL, PROCESS_QUERY_INFORMATION, *PsProcessType, KernelMode, &hProcess);
	if (NT_SUCCESS(status)) {
		status = ZwQueryInformationProcess(hProcess, ProcessWow64Information, &isWow64, sizeof(isWow64), NULL);
		if (NT_SUCCESS(status) && isWow64)
			status = STATUS_NOT_SUPPORTED;

		ZwClose(hProcess);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

#endif


NTSTATUS DriverCreateCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpStack = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (DeviceObject == _controlDeviceObject) {
		DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&_createCloseLock, TRUE);
		irpStack = IoGetCurrentIrpStackLocation(Irp);
		if (irpStack->MajorFunction == IRP_MJ_CLEANUP) {
			UMRequestQueueDisconnect();
			UMDeleteHandlesForProcess(PsGetCurrentProcess());
		}

		ExReleaseResourceLite(&_createCloseLock);
		KeLeaveCriticalRegion();
		status = STATUS_SUCCESS;
		if (irpStack->MajorFunction == IRP_MJ_CREATE) {
#if defined(_AMD64_) || defined(_IA64_)
			status = _Wow64Check();
#endif
			if (NT_SUCCESS(status))
				Irp->IoStatus.Information = FILE_OPENED;
		}

		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		DEBUG_EXIT_FUNCTION("0x%x", status);
	} else status = HookHandlerIRPDisptach(DeviceObject, Irp);

	return status;
}

static NTSTATUS _HandleCDORequest(ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ControlCode=0x%x; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u; IoStatus=0x%p",
		ControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus);

	switch (ControlCode) {
		case IOCTL_IRPMNDRV_CONNECT:
			status = UMRequestQueueConnect();
			break;
		case IOCTL_IRPMNDRV_DISCONNECT:
			UMRequestQueueDisconnect();
			status = STATUS_SUCCESS;
			break;
		case IOCTL_IRPMNDRV_QUEUE_CLEAR:
			UMRequestQueueClear();
			status = STATUS_SUCCESS;
			break;
		case IOCTL_IRPMNDRV_GET_RECORD:
			status = UMGetRequestRecord(OutputBuffer, OutputBufferLength, &IoStatus->Information);
			break;
		case IOCTL_IRPMNDRV_HOOK_DRIVER:
			status = UMHookDriver((PIOCTL_IRPMNDRV_HOOK_DRIVER_INPUT)InputBuffer, InputBufferLength, (PIOCTL_IRPMNDRV_HOOK_DRIVER_OUTPUT)OutputBuffer, OutputBufferLength);
			if (NT_SUCCESS(status))
				IoStatus->Information = OutputBufferLength;
			break;
		case IOCTL_IRPMNDRV_UNHOOK_DRIVER:
			status = UMUnhookDriver((PIOCTL_IRPMNDRV_UNHOOK_DRIVER_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_HOOK_ADD_DEVICE:
			status = UMHookAddDevice((PIOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT)InputBuffer, InputBufferLength, (PIOCTL_IRPMNDRV_HOOK_ADD_DEVICE_OUTPUT)OutputBuffer, OutputBufferLength);
			if (NT_SUCCESS(status))
				IoStatus->Information = OutputBufferLength;
			break;
		case IOCTL_IRPMNDRV_HOOK_REMOVE_DEVICE:
			status = UMHookDeleteDevice((PIOCTL_IRPMNDRV_HOOK_REMOVE_DEVICE_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_GET_DRIVER_DEVICE_INFO:
			status = UMEnumDriversDevices(OutputBuffer, OutputBufferLength, &OutputBufferLength);
			if (NT_SUCCESS(status))
				IoStatus->Information = OutputBufferLength;
			break;
		case IOCTL_IRPMNDRV_HOOK_DRIVER_SET_INFO:
			status = UMHookedDriverSetInfo((PIOCTL_IRPMNDRV_HOOK_DRIVER_SET_INFO_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO:
			status = UMHookedDriverGetInfo((PIOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO_INPUT)InputBuffer, InputBufferLength, (PIOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO_OUTPUT)OutputBuffer, OutputBufferLength);
			break;
		case IOCTL_IRPMNDRV_HOOK_DEVICE_SET_INFO:
			status = UMHookedDeviceSetInfo((PIOCTL_IRPMNDRV_HOOK_DEVICE_SET_INFO_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO:
			status = UMHookedDeviceGetInfo((PIOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO_INPUT)InputBuffer, InputBufferLength, (PIOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO_OUTPUT)OutputBuffer, OutputBufferLength);
			break;
		case IOCTL_IRPMNDRV_HOOK_DRIVER_MONITORING_CHANGE:
			status = UMHookedDriverMonitoringEnable((PIOCTL_IRPMNDRV_HOOK_DRIVER_MONITORING_CHANGE_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMONDRV_HOOK_GET_INFO:
			status = UMHookedObjectsEnumerate((PIOCTL_IRPMONDRV_HOOK_GET_INFO_OUTPUT)OutputBuffer, OutputBufferLength);
			if (NT_SUCCESS(status))
				IoStatus->Information = OutputBufferLength;
			break;
		case IOCTL_IRPMONDRV_HOOK_OPEN:
			status = UMOpenObjectById((PIOCTL_IRPMONDRV_HOOK_OPEN_INPUT)InputBuffer, InputBufferLength, (PIOCTL_IRPMONDRV_HOOK_OPEN_OUTPUT)OutputBuffer, OutputBufferLength);
			if (NT_SUCCESS(status))
				IoStatus->Information = sizeof(IOCTL_IRPMONDRV_HOOK_OPEN_OUTPUT);
			break;
		case IOCTL_IRPMONDRV_HOOK_CLOSE:
			status = UMCloseHandle((PIOCTL_IRPMONDRV_HOOK_CLOSE_INPUT)InputBuffer, InputBufferLength);
			break;

		case IOCTL_IRPMNDRV_CLASS_WATCH_REGISTER:
			status = UMClassWatchRegister((PIOCTL_IRPMNDRV_CLASS_WATCH_REGISTER_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_CLASS_WATCH_UNREGISTER:
			status = UMClassWatchUnregister((PIOCTL_IRPMNDRV_CLASS_WATCH_UNREGISTER_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_CLASS_WATCH_ENUM:
			status = PDWClassEnumerate((PIOCTL_IRPMNDRV_CLASS_WATCH_OUTPUT)OutputBuffer, OutputBufferLength, &IoStatus->Information, ExGetPreviousMode());
			break;

		case IOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER:
			status = UMDriverNameWatchRegister((PIOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER:
			status = UMDriverNamehUnregister((PIOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_DRIVER_WATCH_ENUM:
			status = PWDDriverNameEnumerate((PIOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT)OutputBuffer, OutputBufferLength, &IoStatus->Information, ExGetPreviousMode());
			break;
		case IOCTL_IRPMNDRV_EMULATE_DRVDEV:
			status = UMListDriversDevicesByEvents();
			break;
		case IOCTL_IRPMNDRV_EMULATE_PROCESS:
			status = UMListProcessesByEvents();
			break;

		case IOCTL_IRPMNDRV_SETTINGS_QUERY:
			status = UMDriverSettingsQuery(OutputBuffer, OutputBufferLength, &IoStatus->Information);
			break;
		case IOCTL_IRPMNDRV_SETTINGS_SET:
			status = UMDriverSettingsSet(InputBuffer, InputBufferLength);
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	IoStatus->Status = status;

	DEBUG_EXIT_FUNCTION("0x%x, IoStatus->Status=0x%x, IoStatus->Information=%u", status, IoStatus->Status, IoStatus->Information);
	return status;
}

NTSTATUS DriverDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PVOID inputBuffer = NULL;
	PVOID outputBuffer = NULL;
	ULONG controlCode = 0;
	ULONG inputBufferLength = 0;
	ULONG outputBufferLength = 0;
	PIO_STACK_LOCATION irpSp = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	if (DeviceObject == _controlDeviceObject) {
		DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		controlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
		inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
		outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
		inputBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
		outputBuffer = Irp->UserBuffer;
		status = _HandleCDORequest(controlCode, inputBuffer, inputBufferLength, outputBuffer, outputBufferLength, &Irp->IoStatus);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		DEBUG_EXIT_FUNCTION("0x%x", status);
	} else status = HookHandlerIRPDisptach(DeviceObject, Irp);

	return status;
}

BOOLEAN DriverFastIoDeviceControl(PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG ControlCode, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;

	if (DeviceObject == _controlDeviceObject) {
		DEBUG_ENTER_FUNCTION("FileObject=0x%p; Wait=%u; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u; ControlCode=0x%x; IoStatusBlock=0x%p; DeviceObject=0x%p",
			FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ControlCode, IoStatusBlock, DeviceObject);

		_HandleCDORequest(ControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatusBlock);
		ret = TRUE;

		DEBUG_EXIT_FUNCTION("%u", ret);
	} else ret = HookHandlerFastIoDeviceControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ControlCode, IoStatusBlock, DeviceObject);

	return ret;
}


VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	UNREFERENCED_PARAMETER(DriverObject);

	ModuleFrameworkFinalizeModules();
	ModuleFrameworkFinit();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS DriverShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (DeviceObject == _controlDeviceObject) {
		DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

		PDWClassWatchesUnregister();
		status = STATUS_SUCCESS;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		DEBUG_EXIT_FUNCTION("0x%x", status);
	} else status = HookHandlerIRPDisptach(DeviceObject, Irp);

	return status;
}

/************************************************************************/
/*                COMMUNICATION DEVICE INIT AND FINIT                   */
/************************************************************************/

NTSTATUS DriverInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	UNICODE_STRING uLinkName;
	UNICODE_STRING uDeviceName;
	PDEVICE_OBJECT cdo = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(Context);

	fastIoDispatch = HeapMemoryAllocPaged(sizeof(FAST_IO_DISPATCH));
	if (fastIoDispatch != NULL) {
		memset(fastIoDispatch, 0, sizeof(FAST_IO_DISPATCH));
		fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
		DriverObject->FastIoDispatch = fastIoDispatch;
		status = ExInitializeResourceLite(&_createCloseLock);
		if (NT_SUCCESS(status)) {
			RtlInitUnicodeString(&uDeviceName, IRPMNDRV_DEVICE_NAME);
			status = IoCreateDevice(DriverObject, sizeof(EDeviceExtensionType), &uDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &cdo);
			if (NT_SUCCESS(status)) {
				*(PEDeviceExtensionType)(cdo->DeviceExtension) = detCDO;
				RtlInitUnicodeString(&uLinkName, IRPMNDRV_SYMBOLIC_LINK);
				status = IoCreateSymbolicLink(&uLinkName, &uDeviceName);
				if (NT_SUCCESS(status)) {
					DriverObject->DriverUnload = DriverUnload;
					for (size_t i = 0; i < sizeof(DriverObject->MajorFunction) / sizeof(DriverObject->MajorFunction[0]); ++i)
						DriverObject->MajorFunction[i] = HookHandlerIRPDisptach;

					fastIoDispatch->AcquireForCcFlush = HookHandlerFastIoAcquireForCcFlush;
					fastIoDispatch->AcquireForModWrite = HookHandlerFastIoAcquireForModWrite;
					fastIoDispatch->FastIoCheckIfPossible = HookHandlerFastIoCheckIfPossible;
					fastIoDispatch->FastIoDetachDevice = HookHandlerFastIoDetachDevice;
					fastIoDispatch->FastIoDeviceControl = HookHandlerFastIoDeviceControl;
					fastIoDispatch->FastIoLock = HookHandlerFastIoLock;
					fastIoDispatch->FastIoQueryBasicInfo = HookHandlerFastIoQueryBasicInfo;
					fastIoDispatch->FastIoQueryNetworkOpenInfo = HookHandlerFastIoQueryNetworkOpenInfo;
					fastIoDispatch->FastIoQueryOpen = HookHandlerFastIoQueryOpenInfo;
					fastIoDispatch->FastIoQueryStandardInfo = HookHandlerFastIoQueryStandardInfo;
					fastIoDispatch->FastIoRead = HookHandlerFastIoRead;
					fastIoDispatch->FastIoReadCompressed = HookHandlerFastIoReadCompressed;
					fastIoDispatch->FastIoUnlockAll = HookHandlerFastIoUnlockAll;
					fastIoDispatch->FastIoUnlockAllByKey = HookHandlerFastIoUnlockByKey;
					fastIoDispatch->FastIoUnlockSingle = HookHandlerFastIoUnlockSingle;
					fastIoDispatch->FastIoWrite = HookHandlerFastIoWrite;
					fastIoDispatch->FastIoWriteCompressed = HookHandlerFastIoWriteCompressed;
					fastIoDispatch->MdlRead = HookHandlerFastIoMdlRead;
					fastIoDispatch->MdlReadComplete = HookHandlerFastIoMdlReadComplete;
					fastIoDispatch->MdlReadCompleteCompressed = HookHandlerFastIoMdlReadCompleteCompressed;
					fastIoDispatch->MdlWriteComplete = HookHandlerFastIoMdlWriteComplete;
					fastIoDispatch->MdlWriteCompleteCompressed = HookHandlerFastIoMdlWriteCompleteCompressed;
					fastIoDispatch->PrepareMdlWrite = HookHandlerFastIoMdlWrite;
					fastIoDispatch->ReleaseForCcFlush = HookHandlerFastIoReleaseForCcFlush;
					fastIoDispatch->ReleaseForModWrite = HookHandlerFastIoReleaseForModWrite;
					
					DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateCleanup;
					DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DriverCreateCleanup;
					DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateCleanup;
					DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
					DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DriverShutdown;
					fastIoDispatch->FastIoDeviceControl = DriverFastIoDeviceControl;
					status = IoRegisterShutdownNotification(cdo);
					if (NT_SUCCESS(status))
						_controlDeviceObject = cdo;
					
					if (!NT_SUCCESS(status))
						IoDeleteSymbolicLink(&uLinkName);
				}
				
				if (!NT_SUCCESS(status))
					IoDeleteDevice(DriverObject->DeviceObject);
			}

			if (!NT_SUCCESS(status))
				ExDeleteResourceLite(&_createCloseLock);
		}

		if (!NT_SUCCESS(status)) {
			DriverObject->FastIoDispatch = NULL;
			HeapMemoryFree(fastIoDispatch);
		}
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID DriverFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	UNICODE_STRING uLinkName;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(Context);
	RtlInitUnicodeString(&uLinkName, IRPMNDRV_SYMBOLIC_LINK);
	IoDeleteSymbolicLink(&uLinkName);
	IoDeleteDevice(DriverObject->DeviceObject);
	ExDeleteResourceLite(&_createCloseLock);
	HeapMemoryFree(DriverObject->FastIoDispatch);
	DriverObject->FastIoDispatch = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                      INITIALIZATION                                  */
/************************************************************************/

static DRIVER_MODULE_ENTRY_PARAMETERS _moduleEntries[] = {
	{UtilsModuleInit, UtilsModuleFinit, NULL},
	{DriverSettingsInit, DriverSettingsFinit, NULL},
	{DataLoggerModuleInit, DataLoggerModuleFinit, NULL},
	{RequestQueueModuleInit, RequestQueueModuleFinit, NULL},
	{HookModuleInit, HookModuleFinit, NULL},
	{HookHandlerModuleInit, HookHandlerModuleFinit, NULL},
	{DevExtHooksModuleInit, DevExtHooksModuleFinit, NULL},
	{PWDModuleInit, PWDModuleFinit, NULL},
	{BLModuleInit, BLModuleFinit, NULL},
	{ProcessEventsModuleInit, ProcessEventsModuleFinit, NULL},
	{ImageLoadModuleInit, ImageLoadModuleFinit, NULL},
	{UMServicesModuleInit, UMServicesModuleFinit, NULL},
	{DriverInit, DriverFinit, NULL},
};

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"", DriverObject, RegistryPath);

	_moduleEntries[3].Context = RegistryPath;
	status= ModuleFrameworkInit(DriverObject);
	if (NT_SUCCESS(status)) {
		status = ModuleFrameworkAddModules(_moduleEntries, sizeof(_moduleEntries) / sizeof(DRIVER_MODULE_ENTRY_PARAMETERS));
		if (NT_SUCCESS(status))
			status = ModuleFrameworkInitializeModules(RegistryPath);

		if (!NT_SUCCESS(status))
			ModuleFrameworkFinit();
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}
