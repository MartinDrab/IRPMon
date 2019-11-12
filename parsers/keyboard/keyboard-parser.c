
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "parser-base.h"
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



static BOOLEAN _hideZeroValues = TRUE;



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
			ret = PBaseAddNameFormat(&p, L"Record ", L"%zu", i);
			if (ret == ERROR_SUCCESS)
				ret = PBaseAddNameFormat(&p, L"  Device", L"%u", kbdInput->UnitId);

			if (ret == ERROR_SUCCESS)
				ret = PBaseAddNameFormat(&p, L"  Scan code", L"%u", kbdInput->MakeCode);

			if (ret == ERROR_SUCCESS && (!_hideZeroValues || kbdInput->Flags))
				ret = PBaseAddNameFormat(&p, L"  Flags", L"0x%x", kbdInput->Flags);

			if (ret == ERROR_SUCCESS)
				ret = PBaseAddBooleanValue(&p, L"  Pressed", (kbdInput->Flags & KEY_MAKE) != 0, _hideZeroValues);

			if (ret == ERROR_SUCCESS)
				ret = PBaseAddBooleanValue(&p, L"  Release", (kbdInput->Flags & KEY_BREAK) != 0, _hideZeroValues);

			if (ret == ERROR_SUCCESS)
				ret = PBaseAddBooleanValue(&p, L"  Extended #0", (kbdInput->Flags & KEY_E0) != 0, _hideZeroValues);

			if (ret == ERROR_SUCCESS)
				ret = PBaseAddBooleanValue(&p, L"  Extended #1", (kbdInput->Flags & KEY_E1) != 0, _hideZeroValues);

			if (ret == ERROR_SUCCESS && (!_hideZeroValues || kbdInput->ExtraInformation != 0))
				ret = PBaseAddNameFormat(&p, L"  Extra", L"0x%x (%u)", kbdInput->ExtraInformation, kbdInput->ExtraInformation);

			if (ret == ERROR_SUCCESS && (!_hideZeroValues || kbdInput->Reserved != 0))
				ret = PBaseAddNameFormat(&p, L"  Reserved", L"0x%x (%u)", kbdInput->Reserved, kbdInput->Reserved);

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
			tmpParser->Name = L"KbdClass";
			tmpParser->Description = L"KEYBOARD_INPUT_DATA structure parsing";
			tmpParser->Priority = 1;
			tmpParser->ParseRoutine = _ParseRoutine;
			tmpParser->FreeRoutine = _FreeRoutine;
			*Parser = tmpParser;
		}
	} else ret = ERROR_NOT_SUPPORTED;

	return ret;
}
