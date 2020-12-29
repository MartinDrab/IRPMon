
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hook-handlers.h"
#include "devext-hooks.h"


#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0

/************************************************************************/
/*                        GLOBAL VARIABLE                               */
/************************************************************************/

static PDRIVER_OBJECT _driverObject = NULL;

/************************************************************************/
/*                        PUBLIC FUNCTIONS                              */
/************************************************************************/


NTSTATUS ProxyDeviceCreate(PDEVICE_OBJECT TargetDevice, PDEVICE_OBJECT *ProxyDevice)
{
	PDEVICE_OBJECT *updatePlace = NULL;
	PDEVICE_OBJECT tmpProxy = NULL;
	PDEVICE_OBJECT upperDevice = NULL;
	PPROXY_DEVICE_EXTENSION ext = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("TargetDevice=0x%p; ProxyDevice=0x%p", TargetDevice, ProxyDevice);

	status = STATUS_SUCCESS;
	if (_driverObject == NULL)
		status = STATUS_DEVICE_NOT_READY;

	upperDevice = TargetDevice->AttachedDevice;
	if (NT_SUCCESS(status) && upperDevice == NULL)
		status = STATUS_INVALID_PARAMETER;

	if (NT_SUCCESS(status)) {
		status = IoCreateDevice(_driverObject, sizeof(PROXY_DEVICE_EXTENSION), NULL, TargetDevice->DeviceType, TargetDevice->Characteristics, (TargetDevice->Flags & DO_EXCLUSIVE), &tmpProxy);
		if (NT_SUCCESS(status)) {
			tmpProxy->StackSize = TargetDevice->StackSize;
			tmpProxy->AlignmentRequirement = TargetDevice->AlignmentRequirement;
			tmpProxy->SectorSize = TargetDevice->SectorSize;
			tmpProxy->AttachedDevice = TargetDevice->AttachedDevice;
			ext = (PPROXY_DEVICE_EXTENSION)tmpProxy->DeviceExtension;
			memset(ext, 0, sizeof(PROXY_DEVICE_EXTENSION));
			ext->Type = detProxy;
			ObReferenceObject(TargetDevice);
			ext->TargetDevice = TargetDevice;
			ObReferenceObject(upperDevice);
			ext->UpperDevice = upperDevice;
			ext->UpperDeviceExtensionSize = upperDevice->Size - sizeof(DEVICE_OBJECT);
			updatePlace = (PDEVICE_OBJECT *)upperDevice->DeviceExtension;
			tmpProxy->Flags = TargetDevice->Flags;
			status = STATUS_NOT_FOUND;
			for (size_t i = 0; i < ext->UpperDeviceExtensionSize / sizeof(PDEVICE_OBJECT); ++i) {
				if (*updatePlace == TargetDevice) {
					ext->LowerDeviceOffset = i * sizeof(PDEVICE_OBJECT);
					++ext->LowerDevicePlaceCount;
					*updatePlace = tmpProxy;
					status = STATUS_SUCCESS;
				}

				++updatePlace;
			}


			if (NT_SUCCESS(status))
				*ProxyDevice = tmpProxy;

			if (!NT_SUCCESS(status)) {
				tmpProxy->AttachedDevice = NULL;
				IoDeleteDevice(tmpProxy);
			}
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x, *ProxyDevice=0x%p", status, *ProxyDevice);
	return status;
}


void ProxyDeviceDelete(PDEVICE_OBJECT ProxyDevice)
{
	PDEVICE_OBJECT *updatePlace = NULL;
	PPROXY_DEVICE_EXTENSION ext = NULL;
	DEBUG_ENTER_FUNCTION("ProxyDevice=0x%p", ProxyDevice);

	ProxyDevice->AttachedDevice = NULL;
	ext = (PPROXY_DEVICE_EXTENSION)ProxyDevice->DeviceExtension;
	updatePlace = (PDEVICE_OBJECT *)ext->UpperDevice->DeviceExtension;
	for (size_t i = 0; i < ext->UpperDeviceExtensionSize / sizeof(PDEVICE_OBJECT); ++i) {
		if (*updatePlace == ProxyDevice)
			*updatePlace = ext->TargetDevice;

		++updatePlace;
	}

	ObDereferenceObject(ext->UpperDevice);
	ObDereferenceObject(ext->TargetDevice);
	IoDeleteDevice(ProxyDevice);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void ProxySetDriverObject(PDRIVER_OBJECT DriverObject)
{
	PDRIVER_OBJECT localDriverObject = NULL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	ObReferenceObject(DriverObject);
	localDriverObject = InterlockedExchangePointer(&_driverObject, DriverObject);
	if (localDriverObject != NULL)
		ObDereferenceObject(localDriverObject);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void ProxyTranslate(PDEVICE_OBJECT *DeviceObject, PDRIVER_OBJECT *DriverObject)
{
	PPROXY_DEVICE_EXTENSION ext = NULL;
	PDRIVER_OBJECT tmpDriverObject = NULL;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; DriverObject=0x%p", DeviceObject, DriverObject);

	tmpDriverObject = (*DeviceObject)->DriverObject;
	if (_driverObject != NULL && tmpDriverObject == _driverObject && *DeviceObject != NULL) {
		ext = (PPROXY_DEVICE_EXTENSION)((*DeviceObject)->DeviceExtension);
		if (ext->Type == detProxy) {
			tmpDriverObject = ext->TargetDevice->DriverObject;
			*DeviceObject = ext->TargetDevice;
		}
	}

	*DriverObject = tmpDriverObject;

	DEBUG_EXIT_FUNCTION("void, *DeviceObject=0x%p, *DriverObject=0x%p", *DeviceObject, *DriverObject);
	return;
}


/************************************************************************/
/*                   INITIALIZATION AND FINALIZATION                    */
/************************************************************************/


NTSTATUS DevExtHooksModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	status = STATUS_SUCCESS;
	if (DriverObject != NULL)
		ProxySetDriverObject(DriverObject);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void DevExtHooksModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	PDRIVER_OBJECT localDriverObject = NULL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	localDriverObject = InterlockedExchangePointer(&_driverObject, NULL);
	if (localDriverObject != NULL) {
		ObDereferenceObject(localDriverObject);
		localDriverObject = NULL;
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
