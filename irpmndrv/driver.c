
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
#include "req-queue.h"
#include "regman.h"
#include "driver.h"


/************************************************************************/
/*                             GLOBAL VARIABLES                         */
/************************************************************************/


static ERESOURCE _createCloseLock;
static volatile LONG _openHandles = 0;

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
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

	UNREFERENCED_PARAMETER(DeviceObject);

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
	return status;
}

static NTSTATUS _HandleCDORequest(ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ControlCode=0x%x; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u; IoStatus=0x%p",
		ControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus);

	switch (ControlCode) {
		case IOCTL_IRPMNDRV_CONNECT:
			status = UMRequestQueueConnect((PIOCTL_IRPMNDRV_CONNECT_INPUT)InputBuffer, InputBufferLength);
			break;
		case IOCTL_IRPMNDRV_DISCONNECT:
			UMRequestQueueDisconnect();
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
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

	UNREFERENCED_PARAMETER(DeviceObject);
	irpSp = IoGetCurrentIrpStackLocation(Irp);
	controlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	inputBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	outputBuffer = Irp->UserBuffer;
	status = _HandleCDORequest(controlCode, inputBuffer, inputBufferLength, outputBuffer, outputBufferLength, &Irp->IoStatus);	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

BOOLEAN DriverFastIoDeviceControl(PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG ControlCode, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("FileObject=0x%p; Wait=%u; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u; ControlCode=0x%x; IoStatusBlock=0x%p; DeviceObject=0x%p",
		FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ControlCode, IoStatusBlock, DeviceObject);

	UNREFERENCED_PARAMETER(FileObject);
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Wait);
	
	_HandleCDORequest(ControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatusBlock);
	ret = TRUE;

	DEBUG_EXIT_FUNCTION("%u", ret);
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
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

	UNREFERENCED_PARAMETER(DeviceObject);

	PDWClassWatchesUnregister();
	status = STATUS_SUCCESS;
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DEBUG_EXIT_FUNCTION("0x%x", status);
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
			status = IoCreateDevice(DriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &cdo);
			if (NT_SUCCESS(status)) {
				RtlInitUnicodeString(&uLinkName, IRPMNDRV_SYMBOLIC_LINK);
				status = IoCreateSymbolicLink(&uLinkName, &uDeviceName);
				if (NT_SUCCESS(status)) {
					DriverObject->DriverUnload = DriverUnload;
					DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateCleanup;
					DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DriverCreateCleanup;
					DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
					DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DriverShutdown;
					fastIoDispatch->FastIoDeviceControl = DriverFastIoDeviceControl;
					status = IoRegisterShutdownNotification(cdo);
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
	{RequestQueueModuleInit, RequestQueueModuleFinit, NULL},
	{HookModuleInit, HookModuleFinit, NULL},
	{HookHandlerModuleInit, HookHandlerModuleFinit, NULL},
	{UMServicesModuleInit, UMServicesModuleFinit, NULL},
	{RegManInit, RegManFinit, NULL },
	{PWDModuleInit, PWDModuleFinit, NULL},
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
