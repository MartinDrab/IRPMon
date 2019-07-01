
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "pnp-devcaps.h"



#define IRP_MJ_PNP					0x1b
#define IRP_MN_QUERY_CAPABILITIES   0x09

typedef struct _DEVICE_CAPABILITIES {
	USHORT Size;
	USHORT Version;
	ULONG DeviceD1 : 1;
	ULONG DeviceD2 : 1;
	ULONG LockSupported : 1;
	ULONG EjectSupported : 1;
	ULONG Removable : 1;
	ULONG DockDevice : 1;
	ULONG UniqueID : 1;
	ULONG SilentInstall : 1;
	ULONG RawDeviceOK : 1;
	ULONG SurpriseRemovalOK : 1;
	ULONG WakeFromD0 : 1;
	ULONG WakeFromD1 : 1;
	ULONG WakeFromD2 : 1;
	ULONG WakeFromD3 : 1;
	ULONG HardwareDisabled : 1;
	ULONG NonDynamic : 1;
	ULONG WarmEjectSupported : 1;
	ULONG NoDisplayInUI : 1;
	ULONG Reserved1 : 1;
	ULONG WakeFromInterrupt : 1;
	ULONG SecureDevice : 1;
	ULONG ChildOfVgaEnabledBridge : 1;
	ULONG DecodeIoOnBoot : 1;
	ULONG Reserved : 9;
	ULONG Address;
	ULONG UINumber;
	DEVICE_POWER_STATE DeviceState[POWER_SYSTEM_MAXIMUM];
	SYSTEM_POWER_STATE SystemWake;
	DEVICE_POWER_STATE DeviceWake;
	ULONG D1Latency;
	ULONG D2Latency;
	ULONG D3Latency;
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;


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
	const wchar_t *boolValues[] = {L"false", L"true"};

	ret = ERROR_SUCCESS;
	if (Value)
		Value = TRUE;

	if (!_hideZeroValues || Value)
	ret = _AddNameValue(Pair, Name, boolValues[Value]);

	return ret;
}


static DWORD _AddDevicePowerStateValue(PNV_PAIR Pair, const wchar_t *Name, DEVICE_POWER_STATE Value)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const wchar_t *values[] = {
		L"Unspecified",
		L"DeviceD0",
		L"DeviceD1",
		L"DeviceD2",
		L"DeviceD3",
		L"DeviceMaximum",
	};
	const wchar_t *v = NULL;

	ret = ERROR_SUCCESS;
	if (Value < sizeof(values) / sizeof(values[0]))
		v = values[Value];

	if (!_hideZeroValues || Value) {
		if (v != NULL)
			ret = _AddNameValue(Pair, Name, v);
		else ret = _AddNameFormat(Pair, Name, L"Unknown (%u)", Value);
	}

	return ret;
}


static DWORD _AddSystemPowerStateValue(PNV_PAIR Pair, const wchar_t *Name, SYSTEM_POWER_STATE Value)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const wchar_t *values[] = {
		L"Unspecified",
		L"SystemWorking",
		L"SystemSleeping1",
		L"SystemSleeping2",
		L"SystemSleeping3",
		L"SystemHibernate",
		L"SystemShutdown",
		L"SystemMaximum"
	};
	const wchar_t *v = NULL;

	ret = ERROR_SUCCESS;
	if (Value < sizeof(values) / sizeof(values[0]))
		v = values[Value];

	if (!_hideZeroValues || Value) {
		if (v != NULL)
			ret = _AddNameValue(Pair, Name, v);
		else ret = _AddNameFormat(Pair, Name, L"Unknown (%u)", Value);
	}

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	const DEVICE_CAPABILITIES *devCaps = NULL;

	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRPCompletion) {
		irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
		if (irpComp->MajorFunction == IRP_MJ_PNP &&
			irpComp->MinorFunction == IRP_MN_QUERY_CAPABILITIES &&
			irpComp->DataSize > 0)
			devCaps = (DEVICE_CAPABILITIES *)(irpComp + 1);
	}

	if (devCaps != NULL) {
		memset(&p, 0, sizeof(p));
		ret = _AddNameFormat(&p, L"Version", L"%u", devCaps->Version);
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"Size", L"%u", devCaps->Size);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"DeviceD1", devCaps->DeviceD1 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"DeviceD2", devCaps->DeviceD2 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"LockSupported", devCaps->LockSupported != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"EjectSupported", devCaps->EjectSupported != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"Removable", devCaps->Removable != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"DockDevice", devCaps->DockDevice != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"UniqueID", devCaps->UniqueID != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"SilentInstall", devCaps->SilentInstall != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"RawDeviceOK", devCaps->RawDeviceOK != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"SurpriseRemovalOK", devCaps->SurpriseRemovalOK != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"WakeFromD0", devCaps->WakeFromD0 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"WakeFromD1", devCaps->WakeFromD1 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"WakeFromD2", devCaps->WakeFromD2 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"WakeFromD3", devCaps->WakeFromD3 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"HardwareDisabled", devCaps->HardwareDisabled != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"NonDynamic", devCaps->NonDynamic != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"WarmEjectSupported", devCaps->WarmEjectSupported != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"NoDisplayInUI", devCaps->NoDisplayInUI != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"Reserved1", devCaps->Reserved1 != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"WakeFromInterrupt", devCaps->WakeFromInterrupt != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"SecureDevice", devCaps->SecureDevice != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"ChildOfVgaEnabledBridge", devCaps->ChildOfVgaEnabledBridge != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddBooleanValue(&p, L"DecodeIoOnBoot", devCaps->DecodeIoOnBoot != 0);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"Reserved", L"%u", devCaps->Reserved);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"Address", L"%u", devCaps->Address);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"UI Number", L"0x%x", devCaps->UINumber);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"D1 Latency", L"%u ms", devCaps->D1Latency);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"D2 Latency", L"%u ms", devCaps->D2Latency);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddNameFormat(&p, L"D3 Latency", L"%u ms", devCaps->D3Latency);
		
		if (ret == ERROR_SUCCESS) {
			for (size_t i = 0; i < sizeof(devCaps->DeviceState) / sizeof(devCaps->DeviceState[0]); ++i) {
				ret = _AddDevicePowerStateValue(&p, L"  Device state", devCaps->DeviceState[i]);
				if (ret != ERROR_SUCCESS)
					break;
			}
		}

		if (ret == ERROR_SUCCESS)
			ret = _AddDevicePowerStateValue(&p, L"Device wake", devCaps->DeviceWake);
		
		if (ret == ERROR_SUCCESS)
			ret = _AddSystemPowerStateValue(&p, L"System wake", devCaps->SystemWake);
		
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
	Parser->Name = L"PnPDevCaps";
	Parser->Priority = 1;
	Parser->ParseRoutine = _ParseRoutine;
	Parser->FreeRoutine = _FreeRoutine;

	return ERROR_SUCCESS;
}
