
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "parser-base.h"
#include "mailslots.h"



#define IRP_MJ_CREATE_MAILSLOT			0x13



static BOOLEAN _hideZeroValues = TRUE;


static DWORD _ProcessCreateBuffer(PNV_PAIR Pair, const REQUEST_IRP_CREATE_MAILSLOT_DATA *Buffer)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = PBaseAddNameFormat(Pair, L"Desired access", L"0x%x", Buffer->DesiredAccess);
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pair, L"Quota", L"%u", Buffer->Parameters.MailslotQuota);

	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pair, L"Max message size", L"%u", Buffer->Parameters.MaximumMessageSize);

	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pair, L"Timeout specified", L"%u", Buffer->Parameters.TimeoutSpecified);
	
	if (ret == ERROR_SUCCESS && Buffer->Parameters.TimeoutSpecified)
		ret = PBaseAddNameFormat(Pair, L"Timeout", L"%lld ms", Buffer->Parameters.ReadTimeout.QuadPart / 10000);

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_CREATE_MAILSLOT_DATA *buffer = NULL;

	memset(&p, 0, sizeof(p));
	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRP) {
		irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
		if (irp->DataSize > 0) {
			switch (irp->MajorFunction) {
				case IRP_MJ_CREATE_MAILSLOT:
					buffer = (REQUEST_IRP_CREATE_MAILSLOT_DATA *)(irp + 1);
					ret = _ProcessCreateBuffer(&p, buffer);
					break;
			}
		}
	}

	if (ret == ERROR_SUCCESS && buffer != NULL) {
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

	return ret;
}


static void cdecl _FreeRoutine(wchar_t** Names, wchar_t** Values, size_t Count)
{
	PBaseFreeNameValue(Names, Values, Count);

	return;
}


DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER* Parser)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PIRPMON_DATA_PARSER tmpParser = NULL;

	ret = ERROR_SUCCESS;
	if (RequestedVersion >= IRPMON_DATA_PARSER_VERSION_1) {
		ret = PBaseDataParserAlloc(IRPMON_DATA_PARSER_VERSION_1, &tmpParser);
		if (ret == ERROR_SUCCESS) {
			tmpParser->MajorVersion = 1;
			tmpParser->MinorVersion = 0;
			tmpParser->BuildVersion = 0;
			tmpParser->Name = L"PipesParser";
			tmpParser->Description = L"Parses named pipe requests";
			tmpParser->Priority = 1;
			tmpParser->ParseRoutine = _ParseRoutine;
			tmpParser->FreeRoutine = _FreeRoutine;
			*Parser = tmpParser;
		}
	} else ret = ERROR_NOT_SUPPORTED;

	return ret;
}
