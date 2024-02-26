
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
#include "wpd-data.h"
#include "wpd-parser.h"



#define IRP_MJ_DEVICE_CONTROL					0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL			0x0f


static IWpdSerializer *_serializer = NULL;

BEGIN_WPD_COMMAND_ACCESS_MAP(_wpdAccessMap)
DECLARE_WPD_STANDARD_COMMAND_ACCESS_ENTRIES
END_WPD_COMMAND_ACCESS_MAP
DECLARE_VERIFY_WPD_COMMAND_ACCESS


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

template<>
struct std::hash<GUID>
{
	std::size_t operator()(const GUID& aKey) const
	{
		std::size_t ret = 0;
		const unsigned char* b = (const unsigned char*)&aKey;

		for (size_t i = 0; i < sizeof(GUID); ++i)
			ret ^= std::hash<unsigned char>()(b[i]);

		return ret;
	}
};


static std::unordered_map<PROPERTYKEY, std::wstring> _props;
static std::unordered_map<GUID, std::wstring> _gMap;


static DWORD _AddProperty(uint32_t Level, PNV_PAIR Pair, const PROPERTYKEY *Key, const wchar_t *Value)
{
	DWORD ret = ERROR_GEN_FAILURE;

	auto it = _props.find(*Key);
	if (it != _props.end()) {
		ret = PBaseAddNameValue(Pair, it->second.data(), Value);
	} else {
		wchar_t guidString[255];
		wchar_t fullName[255];

		memset(guidString, 0, sizeof(guidString));
		ret = StringFromGUID2(Key->fmtid, guidString, sizeof(guidString) / sizeof(guidString[0]));
		if (ret > 0) {
			memset(fullName, 0, sizeof(fullName));
			_snwprintf(fullName, sizeof(fullName) / sizeof(fullName[0]), L"%ls#%u", guidString, Key->pid);
			ret = PBaseAddNameValue(Pair, fullName, Value);
		}
	}

	return ret;
}


typedef struct _WPD_STREAM {
	const uint8_t *Buffer;
	size_t Length;
} WPD_STREAM, *PWPD_STREAM;


static BOOLEAN _StreamRead(PWPD_STREAM Stream, void *Value, size_t Length)
{
	BOOLEAN ret = FALSE;

	ret = (Stream->Length >= Length);
	if (ret) {
		memcpy(Value, Stream->Buffer, Length);
		Stream->Buffer += Length;
		Stream->Length -= Length;
	}

	return ret;
}


template <size_t Size, bool Signed, bool Error = false>
static DWORD _ProcessIntNumber(PNV_PAIR Pair, PWPD_STREAM Stream)
{
	DWORD ret = ERROR_GEN_FAILURE;
	uint64_t value = 0;
	const wchar_t *n = L"Int";

	if (!Signed)
		n = L"UInt";

	if (Error)
		n = L"Error";

	if (_StreamRead(Stream, &value, Size)) {
		if (Signed)
			ret = PBaseAddNameFormat(Pair, n, L"%lld (%zu)", value, Size*8);
		if (Error)
			ret = PBaseAddNameFormat(Pair, n, L"%llx (%zu)", value, Size * 8);
		else  ret = PBaseAddNameFormat(Pair, n, L"%llu (%zu)", value, Size*8);
	}

	return ret;
}

template <size_t Size>
static DWORD _ProcessFloat(PNV_PAIR Pair, PWPD_STREAM Stream)
{
	DWORD ret = ERROR_GEN_FAILURE;
	float f = 0.0;
	double d = 0.0;

	switch (Size) {
		case 2:
		case 4:
			if (_StreamRead(Stream, &f, Size))
				ret = PBaseAddNameFormat(Pair, L"Float", L"%f", f);
			break;
		case 8:
			if (_StreamRead(Stream, &d, sizeof(d)))
				ret = PBaseAddNameFormat(Pair, L"Double", L"%lf", d);
			break;
	}

	return ret;
}


static DWORD _ProcessDate(PNV_PAIR Pair, PWPD_STREAM Stream)
{
	DWORD ret = ERROR_GEN_FAILURE;
	FILETIME fileTime;
	SYSTEMTIME st;

	if (_StreamRead(Stream, &fileTime, sizeof(fileTime))) {
		memset(&st, 0, sizeof(st));
		FileTimeToSystemTime(&fileTime, &st);
		ret = PBaseAddNameFormat(Pair, L"Date", L"%.4u-%.2u-%.2u %.2u:%.2u:%.2u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}

	return ret;
}


static DWORD _ProcessBool(PNV_PAIR Pair, PWPD_STREAM Stream)
{
	uint16_t b = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	if (_StreamRead(Stream, &b, sizeof(b)))
		ret = PBaseAddNameFormat(Pair, L"Bool", L"%u", b);

	return ret;
}


static DWORD _ProcessValues(uint32_t Level, PNV_PAIR Pairs, IPortableDeviceValues* Values)
{
	DWORD pvsCount = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	ret = Values->GetCount(&pvsCount);
	if (ret == S_OK) {
		for (DWORD i = 0; i < pvsCount; ++i) {
			PROPERTYKEY key;
			PROPVARIANT value;
			wchar_t valueStr[MAX_PATH];

			memset(&value, 0, sizeof(value));
			ret = Values->GetAt(i, &key, &value);
			if (ret != S_OK) {
				ret = S_OK;
				continue;
			}

			memset(valueStr, 0, sizeof(valueStr));
			switch (value.vt) {
				case VT_ERROR:
				case VT_HRESULT: {

					swprintf(valueStr, L"0x%x", value.uintVal);
				} break;
				case VT_UNKNOWN: {
					IPortableDeviceValues *pvs = NULL;

					ret = Values->GetIPortableDeviceValuesValue(key, &pvs);
					if (ret == S_OK) {
						ret = _ProcessValues(Level + 1, Pairs, pvs);
						pvs->Release();
						break;
					}

					swprintf(valueStr, L"<IUnknown error 0x%x>", ret);
					ret = S_OK;
				} break;
				case VT_CLSID: {
					auto it = _gMap.find(*(GUID*)&value.cauuid);
					if (it != _gMap.end()) {
						wcsncpy(valueStr, it->second.data(), sizeof(valueStr) / sizeof(valueStr[0]) - 1);
						break;
					}
				} default:
					ret = PropVariantToString(value, valueStr, sizeof(valueStr) / sizeof(valueStr[0]));
					if (ret != S_OK)
						swprintf(valueStr, L"<conversion error for type %u>", value.vt);
					break;
			}

			if (valueStr[0] != L'\0')
				_AddProperty(Level, Pairs, &key, valueStr);
			
			PropVariantClear(&value);
		}
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
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	IPortableDeviceValues *pvs = NULL;

	ret = ERROR_SUCCESS;
	if (Request->Type == ertIRP) {
		irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
		if ((irp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
			irp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
			irp->DataSize > 0) {
			ioControlCode = PtrToUlong(irp->Arg3);
			if (IS_WPD_IOCTL(ioControlCode)) {
				buffer = (irp + 1);
				bufferSize = (DWORD)irp->DataSize;
			}
		}
	} else if (Request->Type == ertIRPCompletion) {
		irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
		if ((irpComp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
			irpComp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
			irpComp->DataSize > 0) {
			ioControlCode = PtrToUlong(irpComp->Arguments[2]);
			if (IS_WPD_IOCTL(ioControlCode)) {
				buffer = (irpComp + 1);
				bufferSize = (DWORD)irpComp->DataSize;
			}
		}
	}

	if (buffer != NULL) {
		memset(&p, 0, sizeof(p));
		ret = _serializer->GetIPortableDeviceValuesFromBuffer((PBYTE)buffer, bufferSize, &pvs);		
		if (ret == ERROR_SUCCESS) {
			_ProcessValues(0, &p, pvs);
			ret = VerifyWpdCommandAccessFromMap(IOCTL_WPD_MESSAGE_READ_ACCESS, pvs, _wpdAccessMap);
			ret = PBaseAddNameFormat(&p, L"ReadOnly", L"0x%x", ret);
			pvs->Release();
		}

		if (ret == S_OK) {
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


extern "C" DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER * Parser)
{
	HRESULT ret = ERROR_GEN_FAILURE;
	PIRPMON_DATA_PARSER tmpParser = NULL;

	for (size_t i = 0; i < sizeof(_keyStrings) / sizeof(_keyStrings[0]); ++i)
		_props.insert(std::make_pair(_keyStrings[i].Key, _keyStrings[i].Name));

	for (size_t i = 0; i < sizeof(_guidStrings) / sizeof(_guidStrings[0]); ++i)
		_gMap.insert(std::make_pair(_guidStrings[i].Guid, _guidStrings[i].Name));

	ret = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (ret == RPC_E_CHANGED_MODE)
		ret = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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
