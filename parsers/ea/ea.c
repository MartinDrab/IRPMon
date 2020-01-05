
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "parser-base.h"
#include "ea.h"



#define IRP_MJ_QUERY_EA					0x7
#define IRP_MJ_SET_EA					0x8



typedef struct _FILE_FULL_EA_INFORMATION {
	ULONG  NextEntryOffset;
	UCHAR  Flags;
	UCHAR  EaNameLength;
	USHORT EaValueLength;
	CHAR   EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct _FILE_GET_EA_INFORMATION {
	ULONG NextEntryOffset;
	UCHAR EaNameLength;
	CHAR  EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;


static BOOLEAN _hideZeroValues = TRUE;


static DWORD _ProcessEABuffer(PNV_PAIR Pair, const FILE_FULL_EA_INFORMATION *EABuffer)
{
	ULONG eaIndex = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	do {
		ret = PBaseAddNameFormat(Pair, L"EA Index", L"%u", eaIndex);
		if (ret == ERROR_SUCCESS && (!_hideZeroValues != EABuffer->Flags != 0))
			ret = PBaseAddNameFormat(Pair, L"  Flags", L"0x%x", EABuffer->Flags);

		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(Pair, L"  Name length", L"%u", EABuffer->EaNameLength);

		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(Pair, L"  Name", L"%S", EABuffer->EaName);

		if (ret == ERROR_SUCCESS && (!_hideZeroValues || EABuffer->EaValueLength > 0))
			ret = PBaseAddNameFormat(Pair, L"  Value Size", L"%u", EABuffer->EaValueLength);

		if (EABuffer->NextEntryOffset == 0)
			break;

		EABuffer = (FILE_FULL_EA_INFORMATION *)((unsigned char *)EABuffer + EABuffer->NextEntryOffset);
		++eaIndex;
	} while (ret == ERROR_SUCCESS);

	return ret;
}


static DWORD _ProcessEAList(PNV_PAIR Pair, const FILE_GET_EA_INFORMATION *EAList)
{
	ULONG eaIndex = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	do {
		ret = PBaseAddNameFormat(Pair, L"EA Index", L"%u", eaIndex);
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(Pair, L"  Name length", L"%u", EAList->EaNameLength);

		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(Pair, L"  Name", L"%S", EAList->EaName);

		if (EAList->NextEntryOffset == 0)
			break;

		EAList = (FILE_GET_EA_INFORMATION *)((unsigned char *)EAList + EAList->NextEntryOffset);
		++eaIndex;
	} while (ret == ERROR_SUCCESS);

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	BOOLEAN parsed = FALSE;
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	const FILE_FULL_EA_INFORMATION *eaBuffer = NULL;
	const FILE_GET_EA_INFORMATION *eaList = NULL;

	memset(&p, 0, sizeof(p));
	ret = ERROR_SUCCESS;
	switch (Request->Type) {
		case ertIRP:
			irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
			if (irp->DataSize > 0) {
				switch (irp->MajorFunction) {
					case IRP_MJ_SET_EA:
						eaBuffer = (FILE_FULL_EA_INFORMATION *)(irp + 1);
						ret = _ProcessEABuffer(&p, eaBuffer);
						parsed = TRUE;
						break;
					case IRP_MJ_QUERY_EA:
						eaList = (FILE_GET_EA_INFORMATION *)(irp + 1);
						ret = _ProcessEAList(&p, eaList);
						parsed = TRUE;
						break;
				}
			}
			break;
		case ertIRPCompletion:
			irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
			if (irpComp->MajorFunction == IRP_MJ_QUERY_EA &&
				irpComp->DataSize > 0) {
				eaBuffer = (FILE_FULL_EA_INFORMATION *)(irpComp + 1);
				ret = _ProcessEABuffer(&p, eaBuffer);
				parsed = TRUE;
			}
			break;
	}

	*RowCount = 0;
	*Names = NULL;
	*Values = NULL;
	if (ret == ERROR_SUCCESS) {
		*Handled = parsed;
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


static void cdecl _FreeRoutine(wchar_t **Names, wchar_t **Values, size_t Count)
{
	PBaseFreeNameValue(Names, Values, Count);

	return;
}


DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser)
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
			tmpParser->Name = L"EAParser";
			tmpParser->Description = L"Parses extended attributes (EAs)";
			tmpParser->Priority = 1;
			tmpParser->ParseRoutine = _ParseRoutine;
			tmpParser->FreeRoutine = _FreeRoutine;
			*Parser = tmpParser;
		}
	}
	else ret = ERROR_NOT_SUPPORTED;

	return ret;
}
