
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "keyboard-parser.h"



#define IRP_MJ_READ					0x03

#define KEY_MAKE					0x0
#define KEY_BREAK					0x1
#define KEY_E0						0x2
#define KEY_E1						0x4


typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;


typedef struct _NV_PAIR {
	wchar_t **Names;
	wchar_t **Values;
	size_t Count;
} NV_PAIR, *PNV_PAIR;



static BOOLEAN _hideZeroValues = TRUE;


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
			tmpName = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (totalLen + 1) * sizeof(wchar_t));
			if (tmpName != NULL) {
				tmpValue = tmpName + nameLen + 1;
				CopyMemory(tmpName, Name, nameLen * sizeof(wchar_t));
				CopyMemory(tmpValue, Value, valueLen * sizeof(wchar_t));
				tmp = (wchar_t **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * (Pair->Count + 1) * sizeof(wchar_t *));
				if (tmp != NULL) {
					CopyMemory(tmp, Pair->Names, Pair->Count * sizeof(wchar_t *));
					tmp[Pair->Count] = tmpName;
					CopyMemory(tmp + Pair->Count + 1, Pair->Values, Pair->Count * sizeof(wchar_t *));
					tmp[Pair->Count * 2 + 1] = tmpValue;
					if (Pair->Count > 0)
						HeapFree(GetProcessHeap(), 0, Pair->Names);

					Pair->Names = tmp;
					Pair->Values = tmp + Pair->Count + 1;
					++Pair->Count;
				}
				else ret = GetLastError();

				if (ret != ERROR_SUCCESS)
					HeapFree(GetProcessHeap(), 0, tmpName);
			}
			else ret = GetLastError();
		}
	}

	return ret;
}


static DWORD _AddNameFormat(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Format, ...)
{
	wchar_t buf[1024];
	DWORD ret = ERROR_GEN_FAILURE;
	va_list args;

	va_start(args, Format);
	RtlSecureZeroMemory(buf, sizeof(buf));
	ret = StringCbVPrintf(buf, sizeof(buf) / sizeof(buf[0]), Format, args);
	if (ret == S_OK)
		ret = _AddNameValue(Pair, Name, buf);

	va_end(args);

	return ret;
}


static DWORD _AddBooleanValue(PNV_PAIR Pair, const wchar_t *Name, BOOLEAN Value)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const wchar_t *boolValues[] = { L"false", L"true" };

	ret = ERROR_SUCCESS;
	if (Value)
		Value = TRUE;

	if (!_hideZeroValues || Value)
		ret = _AddNameValue(Pair, Name, boolValues[Value]);

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	const wchar_t *driverName = L"\\Driver\\kbdclass";
	const wchar_t *deviceName = L"\\Device\\KeyboardClass0";
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	const KEYBOARD_INPUT_DATA *kbdInput = NULL;
	size_t inputCount = 0;

	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRPCompletion) {
		irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
		if (irpComp->MajorFunction == IRP_MJ_READ &&
			irpComp->DataSize > 0 &&
			ExtraInfo->DriverName != NULL && _wcsicmp(ExtraInfo->DriverName, driverName) == 0 &&
			ExtraInfo->DeviceName != NULL && wcslen(ExtraInfo->DeviceName) > wcslen(deviceName) &&
			memcmp(ExtraInfo->DeviceName, deviceName, wcslen(deviceName) * sizeof(wchar_t)) == 0) {
			kbdInput = (KEYBOARD_INPUT_DATA *)(irpComp + 1);
			inputCount = irpComp->DataSize / sizeof(KEYBOARD_INPUT_DATA);
		}
	}

	if (kbdInput != NULL) {
		memset(&p, 0, sizeof(p));
		for (size_t i = 0; i < inputCount; ++i) {
			ret = _AddNameFormat(&p, L"Record ", L"%zu", i);
			if (ret == ERROR_SUCCESS)
				ret = _AddNameFormat(&p, L"  Device", L"%u", kbdInput->UnitId);

			if (ret == ERROR_SUCCESS)
				ret = _AddNameFormat(&p, L"  Scan code", L"%u", kbdInput->MakeCode);

			if (ret == ERROR_SUCCESS)
				ret = _AddBooleanValue(&p, L"  Pressed", (kbdInput->Flags & KEY_MAKE) != 0);

			if (ret == ERROR_SUCCESS)
				ret = _AddBooleanValue(&p, L"  Release", (kbdInput->Flags & KEY_BREAK) != 0);

			if (ret == ERROR_SUCCESS)
				ret = _AddBooleanValue(&p, L"  Extended #0", (kbdInput->Flags & KEY_E0) != 0);

			if (ret == ERROR_SUCCESS)
				ret = _AddBooleanValue(&p, L"  Extended #1", (kbdInput->Flags & KEY_E1) != 0);

			if (ret == ERROR_SUCCESS && (!_hideZeroValues || kbdInput->ExtraInformation != 0))
				ret = _AddNameFormat(&p, L"  Extra", L"0x%x (%u)", kbdInput->ExtraInformation, kbdInput->ExtraInformation);

			if (ret == ERROR_SUCCESS && (!_hideZeroValues || kbdInput->Reserved != 0))
				ret = _AddNameFormat(&p, L"  Reserved", L"0x%x (%u)", kbdInput->Reserved, kbdInput->Reserved);

			if (ret != ERROR_SUCCESS)
				break;

			++kbdInput;
		}

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
	Parser->Name = L"KbdClass";
	Parser->Priority = 1;
	Parser->ParseRoutine = _ParseRoutine;
	Parser->FreeRoutine = _FreeRoutine;

	return ERROR_SUCCESS;
}
