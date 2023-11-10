
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include <PortableDeviceTypes.h>
#include <PortableDevice.h>
#include <initguid.h>
#include "general-types.h"
#include "parser-base.h"
#include "wpd-parser.h"



#define IRP_MJ_DEVICE_CONTROL					0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL			0x0f


static IWpdSerializer *_serializer = NULL;


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	size_t bufferSize = 0;
	const void *buffer = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP *irp = NULL;

	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRP) {
		irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
		if ((irp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
			irp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
			irp->DataSize > 0) {
			buffer = (irp + 1);
			bufferSize = irp->DataSize;
		}
	}

	if (buffer != NULL) {
		memset(&p, 0, sizeof(p));
		if (ret == ERROR_SUCCESS) {
			*Handled = TRUE;
			*RowCount = p.Count;
			*Names = p.Names;
			*Values = p.Values;
		}

		if (ret != ERROR_SUCCESS) {
			for (size_t i = 0; i < p.Count; ++i)
				HeapFree(GetProcessHeap(), 0, p.Names[i]);

			HeapFree(GetProcessHeap(), 0, p.Names);
		}
	} else {
		*Names = NULL;
		*Values = NULL;
		*RowCount = 0;
		*Handled = FALSE;
	}

	return ret;
}


static void cdecl _FreeRoutine(wchar_t **Names, wchar_t **Values, size_t Count)
{
	PBaseFreeNameValue(Names, Values, Count);

	return;
}


DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PIRPMON_DATA_PARSER tmpParser = NULL;

	ret = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (ret == S_OK || ret == S_FALSE) {
		ret = CoCreateInstance(CLSID_WpdSerializer, NULL, CLSCTX_INPROC_SERVER, IID_IWpdSerializer, (VOID**)&_serializer);
		if (ret == S_OK) {
			if (RequestedVersion >= IRPMON_DATA_PARSER_VERSION_1) {
				ret = PBaseDataParserAlloc(IRPMON_DATA_PARSER_VERSION_1, &tmpParser);
				if (ret == ERROR_SUCCESS) {
					tmpParser->MajorVersion = 1;
					tmpParser->MinorVersion = 0;
					tmpParser->BuildVersion = 0;
					tmpParser->Name = L"WPD";
					tmpParser->Description = L"WPD IOCTL parsing";
					tmpParser->Priority = 1;
					tmpParser->ParseRoutine = _ParseRoutine;
					tmpParser->FreeRoutine = _FreeRoutine;
					*Parser = tmpParser;
					_serializer->AddRef();
					ret = S_OK;
				}
			} else ret = ERROR_NOT_SUPPORTED;
	
			_serializer->Release();
		}

		if (ret != S_OK)
			CoUninitialize();
	}

	return ret;
}
