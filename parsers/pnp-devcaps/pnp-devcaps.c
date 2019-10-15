
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "parser-base.h"
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



static BOOLEAN _hideZeroValues = TRUE;


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
			ret = PBaseAddNameValue(Pair, Name, v);
		else ret = PBaseAddNameFormat(Pair, Name, L"Unknown (%u)", Value);
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
			ret = PBaseAddNameValue(Pair, Name, v);
		else ret = PBaseAddNameFormat(Pair, Name, L"Unknown (%u)", Value);
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
		ret = PBaseAddNameFormat(&p, L"Version", L"%u", devCaps->Version);
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"Size", L"%u", devCaps->Size);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"DeviceD1", devCaps->DeviceD1 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"DeviceD2", devCaps->DeviceD2 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"LockSupported", devCaps->LockSupported != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"EjectSupported", devCaps->EjectSupported != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"Removable", devCaps->Removable != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"DockDevice", devCaps->DockDevice != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"UniqueID", devCaps->UniqueID != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"SilentInstall", devCaps->SilentInstall != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"RawDeviceOK", devCaps->RawDeviceOK != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"SurpriseRemovalOK", devCaps->SurpriseRemovalOK != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"WakeFromD0", devCaps->WakeFromD0 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"WakeFromD1", devCaps->WakeFromD1 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"WakeFromD2", devCaps->WakeFromD2 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"WakeFromD3", devCaps->WakeFromD3 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"HardwareDisabled", devCaps->HardwareDisabled != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"NonDynamic", devCaps->NonDynamic != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"WarmEjectSupported", devCaps->WarmEjectSupported != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"NoDisplayInUI", devCaps->NoDisplayInUI != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"Reserved1", devCaps->Reserved1 != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"WakeFromInterrupt", devCaps->WakeFromInterrupt != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"SecureDevice", devCaps->SecureDevice != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"ChildOfVgaEnabledBridge", devCaps->ChildOfVgaEnabledBridge != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddBooleanValue(&p, L"DecodeIoOnBoot", devCaps->DecodeIoOnBoot != 0, _hideZeroValues);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"Reserved", L"%u", devCaps->Reserved);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"Address", L"%u", devCaps->Address);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"UI Number", L"0x%x", devCaps->UINumber);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"D1 Latency", L"%u ms", devCaps->D1Latency);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"D2 Latency", L"%u ms", devCaps->D2Latency);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(&p, L"D3 Latency", L"%u ms", devCaps->D3Latency);
		
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
	PBaseFreeNameValue(Names, Values, Count);

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
