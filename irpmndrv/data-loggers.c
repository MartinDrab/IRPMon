
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


void IRPDataLogger(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN Completion, PDATA_LOGGER_RESULT Result)
{
	KPROCESSOR_MODE mode;
	KPROCESSOR_MODE reqMode;
	DEBUG_ENTER_FUNCTION("Irp=0x%p; IrpStack=0x%p; Completion=%u; Result=0x%p", Irp, IrpStack, Completion, Result);

	reqMode = Irp->RequestorMode;
	mode = ExGetPreviousMode();
	memset(Result, 0, sizeof(DATA_LOGGER_RESULT));
	switch (IrpStack->MajorFunction) {
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
					Result->BufferSize = IrpStack->Parameters.Write.Length;
			}
			break;
		case IRP_MJ_DEVICE_CONTROL:
		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		case IRP_MJ_FILE_SYSTEM_CONTROL: {
			if (IrpStack->MajorFunction != IRP_MJ_FILE_SYSTEM_CONTROL ||
				IrpStack->MinorFunction == IRP_MN_KERNEL_CALL ||
				IrpStack->MinorFunction == IRP_MN_USER_FS_REQUEST) {
				ULONG method = IrpStack->Parameters.DeviceIoControl.IoControlCode & 3;

				switch (method) {
					case METHOD_NEITHER:
						if (!Completion) {
							Result->Buffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
							Result->BufferSize = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
							if (IrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
								(IrpStack->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL && IrpStack->MinorFunction == IRP_MN_USER_FS_REQUEST)) {
								if (reqMode == UserMode &&
									Result->BufferSize > 0) {
									__try {
										ProbeForRead(Result->Buffer, Result->BufferSize, 1);
									} __except (EXCEPTION_EXECUTE_HANDLER) {
										DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "DANGEROUS BUFFER: Irp=0x%p; IrpStack=0x%p; Buffer=0x%p; Size=%zu\n", Irp, IrpStack, Result->Buffer, Result->BufferSize);
										__debugbreak();
										Result->Buffer = NULL;
										Result->BufferSize = 0;
									}
								}

								if (reqMode == UserMode &&
									IrpStack->Parameters.DeviceIoControl.OutputBufferLength > 0) {
									__try {
										ProbeForRead(Irp->UserBuffer, IrpStack->Parameters.DeviceIoControl.OutputBufferLength, 1);
									} __except (EXCEPTION_EXECUTE_HANDLER) {
										DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "DANGEROUS BUFFER: Irp=0x%p; IrpStack=0x%p; Buffer=0x%p; Size=%zu\n", Irp, IrpStack, Irp->UserBuffer, IrpStack->Parameters.DeviceIoControl.OutputBufferLength);
										__debugbreak();
										Result->Buffer = NULL;
										Result->BufferSize = 0;
									}
								}
							}
						} else {
							Result->Buffer = Irp->UserBuffer;
							Result->BufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
						}
						break;
					case METHOD_IN_DIRECT:
						if (!Completion) {
							Result->BufferMdl = Irp->MdlAddress;
							if (Irp->MdlAddress != NULL)
								Result->Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
							else Result->Buffer = Irp->AssociatedIrp.SystemBuffer;

							Result->BufferSize = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
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
							Result->BufferSize = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
						}
						break;
					case METHOD_BUFFERED:
						Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
						if (!Completion)
							Result->BufferSize = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
						else Result->BufferSize = Irp->IoStatus.Information;
						break;
				}
			} else {
				if (!Completion) {
					switch (IrpStack->MinorFunction) {
						case IRP_MN_MOUNT_VOLUME:
							Result->Buffer = IrpStack->Parameters.MountVolume.Vpb;
							Result->BufferSize = sizeof(VPB);
							break;
						case IRP_MN_VERIFY_VOLUME:
							Result->Buffer = IrpStack->Parameters.MountVolume.Vpb;
							Result->BufferSize = sizeof(VPB);
							break;
						case IRP_MN_LOAD_FILE_SYSTEM:
							break;
					}

					if (Result->Buffer == NULL)
						Result->BufferSize = 0;
				}
			}
			} break;
		case IRP_MJ_QUERY_VOLUME_INFORMATION:
		case IRP_MJ_QUERY_INFORMATION:
			if (Completion) {
				Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
				Result->BufferSize = Irp->IoStatus.Information;
			}
			break;
		case IRP_MJ_SET_INFORMATION:
			if (!Completion) {
				Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
				Result->BufferSize = IrpStack->Parameters.SetFile.Length;
			}
			break;
		case IRP_MJ_SET_VOLUME_INFORMATION:
			if (!Completion) {
				Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
				Result->BufferSize = IrpStack->Parameters.SetVolume.Length;
			}
			break;
		case IRP_MJ_DIRECTORY_CONTROL:
			switch (IrpStack->MinorFunction) {
				case IRP_MN_QUERY_DIRECTORY:
					Result->Buffer = Irp->UserBuffer;
					Result->BufferSize = Irp->IoStatus.Information;
					if (!Completion) {
						if (reqMode == UserMode &&
							Result->BufferSize > 0) {
							__try {
								ProbeForRead(Result->Buffer, Result->BufferSize, 1);
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "DANGEROUS BUFFER: Irp=0x%p; IrpStack=0x%p; Buffer=0x%p; Size=%zu\n", Irp, IrpStack, Result->Buffer, Result->BufferSize);
								__debugbreak();
							}
						}

						Result->Buffer = NULL;
						Result->BufferSize = 0;
					}
					break;
				case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
					// Already probed by the kernel
					if (!Completion) {
						Result->Buffer = Irp->UserBuffer;
						Result->BufferSize = Irp->IoStatus.Information;
					}
					break;
			}
			break;
		case IRP_MJ_QUERY_EA:
			if (Completion) {
				Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
				Result->BufferSize = Irp->IoStatus.Information;
			}
			break;
		case IRP_MJ_SET_EA:
			if (!Completion) {
				Result->Buffer = Irp->AssociatedIrp.SystemBuffer;
				Result->BufferSize = IrpStack->Parameters.SetEa.Length;
			}
			break;
		case IRP_MJ_PNP: {
			if (Completion && NT_SUCCESS(Irp->IoStatus.Status)) {
				switch (IrpStack->MinorFunction) {
					case IRP_MN_QUERY_ID:
						Result->Buffer = (void *)Irp->IoStatus.Information;
						switch (IrpStack->Parameters.QueryId.IdType) {
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
						Result->Buffer = (void *)Irp->IoStatus.Information;
						Result->BufferSize = _StringSize(Result->Buffer);
						break;
					case IRP_MN_QUERY_CAPABILITIES:
						Result->Buffer = IrpStack->Parameters.DeviceCapabilities.Capabilities;
						Result->BufferSize = sizeof(DEVICE_CAPABILITIES);
						break;
					case IRP_MN_QUERY_DEVICE_RELATIONS:
						Result->Buffer = (void *)Irp->IoStatus.Information;
						Result->BufferSize = _DeviceRelationSize(Result->Buffer);
						break;
					case IRP_MN_QUERY_BUS_INFORMATION:
						Result->Buffer = (void *)Irp->IoStatus.Information;
						Result->BufferSize = sizeof(PNP_BUS_INFORMATION);
						break;
					case IRP_MN_QUERY_INTERFACE:
						Result->Buffer = IrpStack->Parameters.QueryInterface.Interface;
						Result->BufferSize = IrpStack->Parameters.QueryInterface.Size;
						break;
				}
			} else if (!Completion) {
				switch (IrpStack->MinorFunction) {
					case IRP_MN_QUERY_INTERFACE:
						Result->Buffer = (void *)IrpStack->Parameters.QueryInterface.InterfaceType;
						Result->BufferSize = sizeof(GUID);
						break;
				}
			}
		} break;
		case IRP_MJ_QUERY_SECURITY: {
			// Already probed by the kernel
			if (Completion && NT_SUCCESS(Irp->IoStatus.Status)) {
				Result->Buffer = Irp->UserBuffer;
				Result->BufferSize = IrpStack->Parameters.QuerySecurity.Length;
			}
		} break;
		case IRP_MJ_SET_SECURITY: {
			if (!Completion) {
				Result->Buffer = IrpStack->Parameters.SetSecurity.SecurityDescriptor;
				Result->BufferSize = RtlLengthSecurityDescriptor(IrpStack->Parameters.SetSecurity.SecurityDescriptor);
			}
		} break;
		default:
			break;
	}

	if (_stripLargeData && Result->BufferSize > _threshold) {
		Result->BufferSize = _threshold;
		Result->Stripped = TRUE;
	}

	DEBUG_EXIT_FUNCTION("void, *Buffer=0x%p, *BufferSize=%Iu, *BfferMdl=0x%p", Result->Buffer, Result->BufferSize, Result->BufferMdl);
	return;
}


void IRPDataLoggerSetRequestFlags(PREQUEST_HEADER Request, const DATA_LOGGER_RESULT *Data)
{
	DEBUG_ENTER_FUNCTION("Request=0x%p; Data=0x%p", Request, Data);

	if (Data->Stripped)
		Request->Flags |= REQUEST_FLAG_DATA_STRIPPED;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void DataLoggerResultRelease(PDATA_LOGGER_RESULT Result)
{
	DEBUG_ENTER_FUNCTION("Result=0x%p", Result);

	if (Result->BufferAllocated)
		HeapMemoryFree(Result->Buffer);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}