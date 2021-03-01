
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winternl.h>
#include "guid-api.h"



typedef void (NTAPI RTLINITUNICODESTRING)(PUNICODE_STRING String, const wchar_t *WS);
typedef NTSTATUS (WINAPI RTLSTRINGFROMGUID)(const GUID *Guid, PUNICODE_STRING GuidString);
typedef NTSTATUS (NTAPI RTLGUIDFROMSTRING)(PCUNICODE_STRING GuidString, GUID *Guid);

static RTLINITUNICODESTRING *_RtlInitUnicodeString = NULL;
static RTLSTRINGFROMGUID *_RtlStringFromGUID = NULL;
static RTLGUIDFROMSTRING *_RtlGUIDFromString = NULL;





int GAGUIDToStringW(const GUID *G, wchar_t *Buffer, size_t MaxCount)
{
	int ret = 0;
	UNICODE_STRING us;
	wchar_t buf[100];

	us.MaximumLength = sizeof(buf);
	us.Length = 0;
	us.Buffer = buf;
	ret = _RtlStringFromGUID(G, &us);
	if (NT_SUCCESS(ret)) {
		if (MaxCount - 1 >= us.Length / sizeof(wchar_t)) {
			memcpy(Buffer, us.Buffer, us.Length);
			Buffer[us.Length / sizeof(wchar_t)] = L'\0';
			ret = 0;
		} else ret = ERROR_INSUFFICIENT_BUFFER;
	}

	return ret;
}


int GAGUIDToStringA(const GUID *G, char *Buffer, size_t MaxCount)
{
	int ret = 0;
	wchar_t buf[100];

	ret = GAGUIDToStringW(G, buf, sizeof(buf) / sizeof(buf[0]));
	if (ret == 0) {
		ret = snprintf(Buffer, MaxCount, "%ls", buf);
		if (ret > 0 && ret < MaxCount)
			ret = 0;
		else ret = ERROR_INSUFFICIENT_BUFFER;
	}

	return ret;
}


int GAStringToGUIDW(const wchar_t *S, GUID *G)
{
	int ret = 0;
	UNICODE_STRING us;

	_RtlInitUnicodeString(&us, S);
	ret = _RtlGUIDFromString(&us, G);

	return ret;
}


int GAStringToGUIDA(const char *S, GUID *G)
{
	int ret = 0;
	wchar_t buf[100];

	swprintf(buf, sizeof(buf) / sizeof(buf[0]) - 1, L"%hs", S);
	ret = GAStringToGUIDW(buf, G);

	return ret;
}


int GUIDApiInit(void)
{
	int ret = 0;
	HMODULE hLib = NULL;

	hLib = GetModuleHandleW(L"ntdll.dll");
	if (hLib != NULL) {
		_RtlInitUnicodeString = (RTLINITUNICODESTRING *)GetProcAddress(hLib, "RtlInitUnicodeString");
		_RtlStringFromGUID = (RTLSTRINGFROMGUID *)GetProcAddress(hLib, "RtlStringFromGUID");
		_RtlGUIDFromString = (RTLGUIDFROMSTRING *)GetProcAddress(hLib, "RtlGUIDFromString");
		if (_RtlInitUnicodeString == NULL || _RtlStringFromGUID == NULL ||
			_RtlGUIDFromString == NULL)
			ret = GetLastError();
	} else ret = GetLastError();

	return ret;
}
