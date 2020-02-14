
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "request.h"
#include "utils.h"
#include "req-queue.h"
#include "image-load.h"


/************************************************************************/
/*               HELPER FUNCTIONS                                       */
/************************************************************************/


static void _ImageNotify(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	PREQUEST_IMAGE_LOAD ilr = NULL;
	UNICODE_STRING uImageName;
	BASIC_CLIENT_INFO clientInfo;
	const IMAGE_INFO_EX *infoEx = NULL;
	DEBUG_ENTER_FUNCTION("FullImageName=\"%wZ\"; ProcessId=0x%p; ImageInfo=0x%p", FullImageName, ProcessId, ImageInfo);

	memset(&uImageName, 0, sizeof(uImageName));
	if (FullImageName != NULL)
		uImageName = *FullImageName;

	QueryClientBasicInformation(&clientInfo);
	ilr = (PREQUEST_IMAGE_LOAD)RequestMemoryAlloc(sizeof(REQUEST_IMAGE_LOAD) + uImageName.Length);
	if (ilr != NULL) {
		RequestHeaderInit(&ilr->Header, NULL, NULL, ertImageLoad);
		ilr->ImageBase = ImageInfo->ImageBase;
		ilr->ImageSize = ImageInfo->ImageSize;
		ilr->MappedToAllPids = (ImageInfo->ImageMappedToAllPids != 0);
		ilr->KernelDriver = (ImageInfo->SystemModeImage != 0);
		ilr->PartialMap = (ImageInfo->ImagePartialMap != 0);
		ilr->ExtraInfo = (ImageInfo->ExtendedInfoPresent != 0);
		if (ilr->ExtraInfo) {
			infoEx = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);
			ilr->FileObject = infoEx->FileObject;
		}

		ilr->SignatureType = ImageInfo->ImageSignatureType;
		ilr->SignatureLevel = ImageInfo->ImageSignatureLevel;
		ilr->DataSize = uImageName.Length;
		memcpy(ilr + 1, uImageName.Buffer, uImageName.Length);
		_SetRequestFlags(&ilr->Header, &clientInfo);
		RequestQueueInsert(&ilr->Header);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*               PUBLIC FUNCTIONS                                       */
/************************************************************************/


NTSTATUS ImageLoadModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	status = PsSetLoadImageNotifyRoutine(_ImageNotify);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

void ImageLoadModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	PsRemoveLoadImageNotifyRoutine(_ImageNotify);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
