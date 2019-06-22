
#include <windows.h>
#include "general-types.h"
#include "data-parser-types.h"
#include "pnp-ids.h"


#define IRP_MJ_PNP					0x1b
#define IRP_MN_QUERY_ID				0x13

typedef enum {
	BusQueryDeviceID = 0,
	BusQueryHardwareIDs = 1,
	BusQueryCompatibleIDs = 2,
	BusQueryInstanceID = 3,
	BusQueryDeviceSerialNumber = 4,
	BusQueryContainerID = 5
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;



static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	size_t idCount = 0;
	size_t tmpLen = 0;
	size_t index = 0;
	wchar_t **tmpOutput = NULL;
	const wchar_t *tmp = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	const wchar_t *ids = NULL;
	const wchar_t *idName = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;

	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRPCompletion) {
		irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
		if (irpComp->MajorFunction == IRP_MJ_PNP &&
			irpComp->MinorFunction == IRP_MN_QUERY_ID) {
			ids = (wchar_t *)(irpComp + 1);
			switch ((BUS_QUERY_ID_TYPE)irpComp->Arguments[0]) {
				case BusQueryDeviceID:
					idName = L"Device ID";
					break;
				case BusQueryInstanceID:
					idName = L"Instance ID";
					break;
				case BusQueryHardwareIDs:
					idName = L"Hardware ID";
					break;
				case BusQueryCompatibleIDs:
					idName = L"Compatible ID";
					break;
				case BusQueryContainerID:
					idName = L"Container ID";
					break;
				case BusQueryDeviceSerialNumber:
					idName = L"Serial number";
					break;
				default:
					idName = L"Unknown ID";
					break;
			}
		}
	}

	if (ids != NULL) {
		if (irpComp->DataSize > 0) {
			tmp = ids;
			while (*tmp != L'\0') {
				tmpLen = wcslen(tmp);
				++idCount;
				tmp += (tmpLen + 1);
			}

			tmpOutput = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, idCount*2*sizeof(wchar_t *));
			if (tmpOutput != NULL) {
				tmp = ids;
				while (ret == ERROR_SUCCESS && *tmp != L'\0') {
					tmpLen = wcslen(tmp);
					tmpOutput[index] = (wchar_t *)idName;
					tmpOutput[index + idCount] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (tmpLen + 1)*sizeof(wchar_t));
					if (tmpOutput[index + idCount] != NULL) {
						memcpy(tmpOutput[index + idCount], tmp, tmpLen*sizeof(wchar_t));
						++index;
						tmp += (tmpLen + 1);
					} else ret = GetLastError();
				}

				if (ret == ERROR_SUCCESS) {
					*Handled = TRUE;
					*Names = tmpOutput;
					*Values = tmpOutput + idCount;
					*RowCount = idCount;
				}

				if (ret != ERROR_SUCCESS) {
					while (index > 0) {
						HeapFree(GetProcessHeap(), 0, tmpOutput[idCount + index - 1]);
						--index;
					}

					HeapFree(GetProcessHeap(), 0, tmpOutput);
				}
			} else ret = GetLastError();
		} else {
			*Handled = TRUE;
			*RowCount = 0;
			*Names = NULL;
			*Values = NULL;
		}
	} else *Handled = FALSE;

	return ret;
}


static void cdecl _FreeRoutine(wchar_t **Names, wchar_t **Values, size_t Count)
{
	if (Count > 0) {
		for (size_t i = 0; i < Count; ++i)
			HeapFree(GetProcessHeap(), 0, Values[i]);

		HeapFree(GetProcessHeap(), 0, Names);
	}

	return;
}



DWORD cdecl DP_INIT_ROUTINE_NAME(PIRPMON_DATA_PARSER Parser)
{
	RtlSecureZeroMemory(Parser, sizeof(IRPMON_DATA_PARSER));
	Parser->MajorVersion = 1;
	Parser->MinorVersion = 0;
	Parser->BuildVersion = 0;
	Parser->Name = L"PnP ID Parser";
	Parser->Priority = 1;
	Parser->ParseRoutine = _ParseRoutine;
	Parser->FreeRoutine = _FreeRoutine;

	return ERROR_SUCCESS;
}
