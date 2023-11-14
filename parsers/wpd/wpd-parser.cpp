
#include <string>
#include <functional>
#include <unordered_map>
#include <cstdint>
#include <windows.h>
#include <strsafe.h>
#include <PortableDeviceTypes.h>
#include <PortableDevice.h>
#include <initguid.h>
#include <propvarutil.h>
#include "general-types.h"
#include "parser-base.h"
#include "wpd-parser.h"



#define IRP_MJ_DEVICE_CONTROL					0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL			0x0f


static IWpdSerializer *_serializer = NULL;

BEGIN_WPD_COMMAND_ACCESS_MAP(_wpdAccessMap)
DECLARE_WPD_STANDARD_COMMAND_ACCESS_ENTRIES
END_WPD_COMMAND_ACCESS_MAP
DECLARE_VERIFY_WPD_COMMAND_ACCESS

typedef struct _WPD_GUID_ENTRY {
	PROPERTYKEY Guid;
	const wchar_t *Name;
} WPD_GUID_ENTRY, *PWPD_GUID_ENTRY;

static WPD_GUID_ENTRY _propertyMap[] = {
	{WPD_PROPERTY_COMMON_COMMAND_CATEGORY, L"Category"},
	{WPD_PROPERTY_COMMON_COMMAND_ID, L"ID"},
	{WPD_PROPERTY_COMMON_HRESULT, L"HRESULT"},
	{WPD_PROPERTY_COMMON_DRIVER_ERROR_CODE, L"DriverCode"},
	{WPD_PROPERTY_COMMON_COMMAND_TARGET, L"Target"},
	{WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT, L"ClientContext"},
	{WPD_PROPERTY_COMMON_ACTIVITY_ID, L"ActivityId"},
};

static WPD_GUID_ENTRY _commandMap [] = {
	{WPD_COMMAND_COMMON_RESET_DEVICE, L"ResetDevice"},
	{WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS, L"GetObjectIDs"},
	{WPD_COMMAND_COMMON_SAVE_CLIENT_INFORMATION, L"SaveClientInformation"},
	{WPD_COMMAND_OBJECT_ENUMERATION_START_FIND, L"FindFirst"},
	{WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT, L"FindNext"},
	{WPD_COMMAND_OBJECT_ENUMERATION_END_FIND, L"FindClose"},
	{WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED, L"GetSupported"},
	{WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES, L"GetAttributes"},
	{WPD_COMMAND_OBJECT_PROPERTIES_GET, L"Get"},
	{WPD_COMMAND_OBJECT_PROPERTIES_SET, L"Set"},
	{WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL, L"GetAll"},
	{WPD_COMMAND_OBJECT_PROPERTIES_DELETE, L"Delete"},
	{WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_START, L"BulkFindFirst"},
	{WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_NEXT, L"BulkFindNext"},
	{WPD_COMMAND_OBJECT_PROPERTIES_BULK_GET_VALUES_BY_OBJECT_LIST_END, L"BulkFindClose"},
	{WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_START, L"SetFirst"},
	{WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_NEXT, L"SetNext"},
	{WPD_COMMAND_OBJECT_PROPERTIES_BULK_SET_VALUES_BY_OBJECT_LIST_END, L"SetClose"}
};


template<>
struct std::hash<PROPERTYKEY>
{
	std::size_t operator()(const PROPERTYKEY & aKey) const
	{
		std::size_t ret = 0;
		const unsigned char *b = (const unsigned char *)&aKey;
	
		for (size_t i = 0; i < sizeof(PROPERTYKEY); ++i)
			ret ^= std::hash<unsigned char>()(b[i]);

		return ret;
	}
};


static std::unordered_map<PROPERTYKEY, std::wstring> _cmds;
static std::unordered_map<PROPERTYKEY, std::wstring> _props;


static DWORD _AddProperty(PNV_PAIR Pair, const PROPERTYKEY *Key, const wchar_t *Value)
{
	DWORD ret = ERROR_GEN_FAILURE;

	auto it = _props.find(*Key);
	if (it != _props.end()) {
		ret = PBaseAddNameValue(Pair, it->second.data(), Value);
	} else {
		wchar_t guidString[255];
		wchar_t fullName[255];

		memset(guidString, 0, sizeof(guidString));
		StringFromGUID2(Key->fmtid, guidString, sizeof(guidString) / sizeof(guidString[0]));
		_snwprintf(fullName, sizeof(fullName) / sizeof(fullName[0]), L"%ls#%u", guidString, Key->pid);
		memset(fullName, 0, sizeof(fullName));
		ret = PBaseAddNameValue(Pair, fullName, Value);
	}

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	DWORD pvsCount = 0;
	DWORD bufferSize = 0;
	ULONG ioControlCode = 0;
	const void *buffer = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP *irp = NULL;
	IPortableDeviceValues *pvs = NULL;

	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRP) {
		irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
		if ((irp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
			irp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
			irp->DataSize > 0) {
			ioControlCode = (ULONG)irp->Arg3;
			if (IS_WPD_IOCTL(ioControlCode)) {
				buffer = (irp + 1);
				bufferSize = (DWORD)irp->DataSize;
			}
		}
	}

	if (buffer != NULL) {
		memset(&p, 0, sizeof(p));
		ret = _serializer->GetIPortableDeviceValuesFromBuffer((PBYTE)buffer, bufferSize, &pvs);
		if (ret == ERROR_SUCCESS) {
			ret = pvs->GetCount(&pvsCount);
			if (ret == S_OK) {
				for (DWORD i = 0; i < pvsCount; ++i) {
					PROPERTYKEY k;
					PROPVARIANT v;
					wchar_t strbuf[256];
					UINT strbufSize = sizeof(strbuf) / sizeof(strbuf[0]);

					memset(&k ,0, sizeof(k));
					memset(&v, 0, sizeof(v));
					memset(strbuf, 0, sizeof(strbuf));
					ret = pvs->GetAt(i, &k, &v);
					if (ret != S_OK)
						continue;

					ret = PropVariantToString(v, strbuf, strbufSize);
					if (ret == S_OK)
						ret = _AddProperty(&p, &k, strbuf);
					
					PropVariantClear(&v);
				}

				if (ret == S_OK) {
					ret = VerifyWpdCommandAccessFromMap(IOCTL_WPD_MESSAGE_READ_ACCESS, pvs, _wpdAccessMap);
					PBaseAddBooleanValue(&p, L"ReadOnly", (ret == S_OK), FALSE);
					ret = S_OK;
				}

				if (ret == S_OK) {
					*Handled = TRUE;
					*RowCount = p.Count;
					*Names = p.Names;
					*Values = p.Values;
				}
			}

			pvs->Release();
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

	for (size_t i = 0; i < sizeof(_propertyMap) / sizeof(_propertyMap[0]); ++i)
		_props.insert(std::make_pair(_propertyMap->Guid, _propertyMap->Name));

	for (size_t i = 0; i < sizeof(_commandMap) / sizeof(_commandMap[0]); ++i)
		_cmds.insert(std::make_pair(_commandMap->Guid, _commandMap->Name));

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
