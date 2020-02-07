
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

	upperDevice = TargetDevice->AttachedDevice;
	if (upperDevice != NULL) {
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
	} else status = STATUS_INVALID_PARAMETER;

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


/************************************************************************/
/*                   INITIALIZATION AND FINALIZATION                    */
/************************************************************************/


NTSTATUS DevExtHooksModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	ObReferenceObject(DriverObject);
	_driverObject = DriverObject;
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void DevExtHooksModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	ObDereferenceObject(_driverObject);
	_driverObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
