
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils.h"
#include "data-loggers.h"



/************************************************************************/
/*               GLOBAL VARIABLES                                       */
/************************************************************************/

static ULONG _threshold = 1024;
static BOOLEAN _stripLargeData = TRUE;


/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static SIZE_T _StringSize(const wchar_t *S)
{
	SIZE_T ret = 0;
	DEBUG_ENTER_FUNCTION("S=%ls", S);

	if (S != NULL)
		ret = (wcslen(S) + 1) * sizeof(wchar_t);

	DEBUG_EXIT_FUNCTION("%Iu", ret);
	return ret;
}


static SIZE_T _MultiStringSize(const wchar_t *MS)
{
	SIZE_T len = 0;
	SIZE_T ret = 0;
	DEBUG_ENTER_FUNCTION("MS=0x%p", MS);

	if (MS != NULL) {
		ret = sizeof(wchar_t);
		while (*MS != L'\0') {
			len = wcslen(MS) + 1;
			ret += len * sizeof(wchar_t);
			MS += len;
		}
	}

	DEBUG_EXIT_FUNCTION("%Iu", ret);
	return ret;
}


static SIZE_T _DeviceRelationSize(const DEVICE_RELATIONS *R)
{
	SIZE_T ret = 0;
	DEBUG_ENTER_FUNCTION("R=0x%p", R);

	if (R != NULL)
		ret = sizeof(DEVICE_RELATIONS) - sizeof(PDEVICE_OBJECT) + sizeof(PDEVICE_OBJECT)*R->Count;

	DEBUG_EXIT_FUNCTION("%Iu", ret);
	return ret;
}


/************************************************************************/
/*            PUBLIC FUNCTIONS                                          */
/************************************************************************/


void IRPDataLogger(PIRP Irp, BOOLEAN Completion, PDATA_LOGGER_RESULT Result)
{
	KPROCESSOR_MODE mode;
	PIO_STACK_LOCATION irpStack = NULL;
	DEBUG_ENTER_FUNCTION("Irp=0x%p; Result=0x%p", Irp, Result);

	mode = ExGetPreviousMode();
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	memset(Result, 0, sizeof(Result));
	switch (irpStack->MajorFunction) {
		case IRP_MJ_READ: {
			if (Completion) {
				if (Irp->MdlAddress != NULL) {
					Result->BufferMdl = Irp->MdlAddress;
					Result->Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
				} else if (Irp->AssociatedIrp.SystemBuffer != NULL)
					Result->Buffer = Irp->AssociatedIrp.SystemBuffer;

				if (Result->Buffer != NULL)
					Result->BufferSize = Irp->IoStatus.Information;
			}
		} break;
		case IRP_MJ_WRITE:
			if (!Completion) {
				if (Irp->MdlAddress != NULL) {
					Result->BufferMdl = Irp->MdlAddress;
					Result->Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
				} else if (Irp->AssociatedIrp.SystemBuffer != NULL)
					Result->Buffer = Irp->AssociatedIrp.SystemBuffer;

				if (Result->Buffer != NULL)
					Result->BufferSize = irpStack->Parameters.Write.Length;
			}
			break;
		case IRP_MJ_DEVICE_CONTROL:
		case IRP_MJ_INTERNAL_DEVICE_CONTROL: {
			ULONG method = irpStack->Parameters.DeviceIoControl.IoControlCode & 3;

			switch (method) {
				case METHOD_NEITHER:
					if (!Completion) {
						Result->Buffer = irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
						Result->BufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
					} else {
						Result->Buffer = Irp->UserBuffer;
						Result->BufferSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
					}
					break;
				case METHOD_IN_DIRECT:
					if (!Completion) {
						Result->BufferMdl = Irp->MdlAddress;
						if (Irp->MdlAddress != NULL)
							Result->Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
						else Result->Buffer = Irp->AssociatedIrp.SystemBuffer;

						Result->BufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
					}
					break;
				case METHOD_OUT_DIRECT:
					if (Completion) {
						Result->BufferMdl = Irp->MdlAddress;
						if (Irp->MdlAddress != NULL)
							Result->Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
						else Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
					
						Result->BufferSize = Irp->IoStatus.Information;
					} else {
						Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
						Result->BufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
					}
					break;
				case METHOD_BUFFERED:
					Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
					if (!Completion)
						Result->BufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
					else Result->BufferSize = Irp->IoStatus.Information;
					break;
			}
			} break;
		case IRP_MJ_PNP: {
			if (Completion && NT_SUCCESS(Irp->IoStatus.Status)) {
				switch (irpStack->MinorFunction) {
					case IRP_MN_QUERY_ID:
						Result->Buffer = Irp->IoStatus.Pointer;
						switch (irpStack->Parameters.QueryId.IdType) {
							case BusQueryDeviceID:
							case BusQueryInstanceID:
							case BusQueryContainerID:
							case BusQueryDeviceSerialNumber:
								Result->BufferSize = _StringSize(Result->Buffer);
								break;
							case BusQueryHardwareIDs:
							case BusQueryCompatibleIDs:
								Result->BufferSize = _MultiStringSize(Result->Buffer);
								break;
						}
						break;
					case IRP_MN_QUERY_DEVICE_TEXT:
						Result->Buffer = Irp->IoStatus.Pointer;
						Result->BufferSize = _StringSize(Result->Buffer);
						break;
					case IRP_MN_QUERY_CAPABILITIES:
						Result->Buffer = irpStack->Parameters.DeviceCapabilities.Capabilities;
						Result->BufferSize = sizeof(DEVICE_CAPABILITIES);
						break;
					case IRP_MN_QUERY_DEVICE_RELATIONS:
						Result->Buffer = Irp->IoStatus.Pointer;
						Result->BufferSize = _DeviceRelationSize(Result->Buffer);
						break;
					case IRP_MN_QUERY_BUS_INFORMATION:
						Result->Buffer = Irp->IoStatus.Pointer;
						Result->BufferSize = sizeof(PNP_BUS_INFORMATION);
						break;
				}
			}
		} break;
		default:
			break;
	}

	DEBUG_EXIT_FUNCTION("void, *Buffer=0x%p, *BufferSize=%Iu, *BfferMdl=0x%p", Result->Buffer, Result->BufferSize, Result->BufferMdl);
	return;
}
