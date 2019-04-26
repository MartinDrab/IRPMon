
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "secdesc.h"



#define IRP_MJ_QUERY_SECURITY		0x14
#define IRP_MJ_SET_SECURITY			0x15

typedef struct _NV_PAIR {
	wchar_t **Names;
	wchar_t **Values;
	size_t Count;
} NV_PAIR, *PNV_PAIR;


static DWORD _AddNameValue(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Value)
{
	DWORD ret = ERROR_GEN_FAILURE;
	wchar_t *tmpName = NULL;
	wchar_t *tmpValue = NULL;
	size_t nameLen = 0;
	size_t valueLen = 0;
	size_t totalLen = 0;
	wchar_t **tmp = NULL;

	ret = StringCbLengthW(Name, STRSAFE_MAX_CCH, &nameLen);
	if (ret == S_OK) {
		ret = StringCbLengthW(Value, STRSAFE_MAX_CCH, &valueLen);
		if (ret == S_OK) {
			totalLen = nameLen + 1 + valueLen;
			tmpName = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (totalLen + 1)*sizeof(wchar_t));
			if (tmpName != NULL) {
				tmpValue = tmpName + nameLen + 1;
				CopyMemory(tmpName, Name, nameLen*sizeof(wchar_t));
				CopyMemory(tmpValue, Value, valueLen * sizeof(wchar_t));
				tmp = (wchar_t **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2*(Pair->Count + 1)*sizeof(wchar_t *));
				if (tmp != NULL) {
					CopyMemory(tmp, Pair->Names, Pair->Count*sizeof(wchar_t *));
					tmp[Pair->Count] = tmpName;
					CopyMemory(tmp + Pair->Count + 1, Pair->Values, Pair->Count*sizeof(wchar_t *));
					tmp[Pair->Count * 2 + 1] = tmpValue;
					if (Pair->Count > 0)
						HeapFree(GetProcessHeap(), 0, Pair->Names);

					Pair->Names = tmp;
					Pair->Values = tmp + Pair->Count + 1;
					++Pair->Count;
				} else ret = GetLastError();
			
				if (ret != ERROR_SUCCESS)
					HeapFree(GetProcessHeap(), 0, tmpName);
			} else ret = GetLastError();
		}
	}

	return ret;
}




static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	DWORD ret = ERROR_GEN_FAILURE;
	size_t length = 0;
	const SECURITY_DESCRIPTOR *data = NULL;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;

	ret = ERROR_NOT_SUPPORTED;
	RtlSecureZeroMemory(&p, sizeof(p));
	switch (Request->Type) {
		case ertIRP:
			irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
			if (irp->MajorFunction == IRP_MJ_SET_SECURITY) {
				data = (SECURITY_DESCRIPTOR *)(irp + 1);
				length = irp->DataSize;
				ret = ERROR_SUCCESS;
			}
			break;
		case ertIRPCompletion:
			irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
			data = (SECURITY_DESCRIPTOR *)(irpComp + 1);
			length = irpComp->DataSize;
			ret = ERROR_SUCCESS;
			break;
		default:
			break;
	}

	if (ret == ERROR_SUCCESS) {

	} else if (ret == ERROR_NOT_SUPPORTED) {
		*Handled = FALSE;
		ret = ERROR_SUCCESS;
	}


	return ret;
}


static void cdecl _FreeRoutine(wchar_t **Names, wchar_t **Values, size_t Count)
{
	for (size_t i = 0; i < Count; ++i)
		HeapFree(GetProcessHeap(), 0, Names[i]);

	HeapFree(GetProcessHeap(), 0, Names);

	return;
}



DWORD cdecl DP_INIT_ROUTINE_NAME(PIRPMON_DATA_PARSER Parser)
{
	RtlSecureZeroMemory(Parser, sizeof(IRPMON_DATA_PARSER));
	Parser->MajorVersion = 1;
	Parser->MinorVersion = 0;
	Parser->BuildVersion = 0;
	Parser->Name = L"SecDesc";
	Parser->Priority = 1;
	Parser->ParseRoutine = _ParseRoutine;
	Parser->FreeRoutine = _FreeRoutine;

	return ERROR_SUCCESS;
}
