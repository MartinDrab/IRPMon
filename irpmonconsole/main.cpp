
#include <string>
#include <vector>
#include <assert.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>
#include "debug.h"
#include "irpmondll-types.h"
#include "irpmondll.h"
#include "install.h"
#include "cache.h"
#include "libtranslate.h"
#include "main.h"


/************************************************************************/
/*                   HELPER FUNCTIONS                                   */
/************************************************************************/

static BOOLEAN StringToPointer(PWCHAR String, PVOID *Pointer)
{
	BOOLEAN ret = FALSE;
	ULONG_PTR tmpProinter = 0;
	SIZE_T len = String != NULL ? wcslen(String) : 0;

	if (len > 0) {
		ret = (len > 2 && len <= 2+sizeof(PVOID)*2 && String[0] == L'0' && (String[1] == L'x' || String[1] == L'X'));
		if (ret) {
			PWCHAR aktChr = String + 2;

			while (ret && *aktChr != L'\0') {
				if (*aktChr >= L'0' && *aktChr <= L'9')
					tmpProinter = tmpProinter*16 + (*aktChr - L'0');
				else if (*aktChr >= L'a' && *aktChr <= L'f')
					tmpProinter = tmpProinter*16 + (*aktChr - L'a' + 10);
				else if (*aktChr >= L'A' && *aktChr <= L'F')
					tmpProinter = tmpProinter*16 + (*aktChr - L'A' + 10);
				else ret = FALSE;
				
				++aktChr;
			}
		}
	} else {
		tmpProinter = NULL;
		ret = TRUE;
	}

	if (ret)
		*Pointer = (PVOID)tmpProinter;

	return ret;
}

/************************************************************************/
/*                  PRINTING                                            */
/************************************************************************/

static std::wstring Ptr2Hex(PVOID Value)
{
	WCHAR buf[19];

	swprintf(buf, 19, L"0x%p", Value);

	return buf;
}

static std::wstring UInt642Hex(ULONG64 Value)
{
	WCHAR buf[19];

	swprintf(buf, 19, L"0x%x", Value);

	return buf;
}

static std::wstring _FastIoTypeToString(EFastIoOperationType Type)
{
	std::wstring res;
	PWCHAR _fastIoTypes[] = {
		L"FastIoCheckIfPossible",
		L"FastIoRead",
		L"FastIoWrite",
		L"FastIoQueryBasicInfo",
		L"FastIoQueryStandardInfo",
		L"FastIoLock",
		L"FastIoUnlockSingle",
		L"FastIoUnlockAll",
		L"FastIoUnlockAllByKey",
		L"FastIoDeviceControl",
		L"AcquireFileForNtCreateSection",
		L"ReleaseFileForNtCreateSection",
		L"FastIoDetachDevice",
		L"FastIoQueryNetworkOpenInfo",
		L"AcquireForModWrite",
		L"MdlRead",
		L"MdlReadComplete",
		L"PrepareMdlWrite",
		L"MdlWriteComplete",
		L"FastIoReadCompressed",
		L"FastIoWriteCompressed",
		L"MdlReadCompleteCompressed",
		L"MdlWriteCompleteCompressed",
		L"FastIoQueryOpen",
		L"ReleaseForModWrite",
		L"AcquireForCcFlush",
		L"ReleaseForCcFlush"};
	
	if ((ULONG)Type < FastIoMax)
		res = _fastIoTypes[Type];
	else res = L"<nknown> (" + std::to_wstring(Type) + L")";

	return res;
}

static std::wstring _AccessModeToString(ULONG AccessMode)
{
	std::wstring res;
	PWCHAR modes [] = {L"KernelMode", L"UserMode"};

	if (AccessMode < sizeof(modes) / sizeof(PWCHAR))
		res = modes[AccessMode];
	else res = L"<unknown> (" + std::to_wstring(AccessMode) + L")";

	return res;
}

std::wstring GetRequestResult(PREQUEST_HEADER h)
{
	std::wstring res;

	switch (h->ResultType) {
		case rrtNTSTATUS:
			res = LibTranslateGeneralIntegerValueToString(ltivtNTSTATUS, FALSE, h->Result.NTSTATUSValue);
			break;
		case rrtBOOLEAN:
			res = (h->Result.BOOLEANValue) ? L"TRUE" : L"FALSE";
			break;
		default:
			res = L"None";
			break;
	}

	return res;
}

static void _ParseIRPParameters(UCHAR MajorFunction, UCHAR MinorFunction, PVOID Arg1, PVOID Arg2, PVOID Arg3, PVOID Arg4, std::wstring & minor, std::vector<std::pair<std::wstring, std::wstring>> & args)
{
	PWCHAR wMinor = NULL;
	minor = std::to_wstring(MinorFunction);

	switch (MajorFunction) {
		case IRP_MJ_CREATE:
			args.push_back(std::make_pair(L"Security context", Ptr2Hex(Arg1)));
			args.push_back(std::make_pair(L"Options", Ptr2Hex(Arg2)));
			args.push_back(std::make_pair(L"File attributes", Ptr2Hex(Arg3)));
			args.push_back(std::make_pair(L"EA length", std::to_wstring((ULONG)Arg4)));
			break;
		case IRP_MJ_READ:
		case IRP_MJ_WRITE: {
			wMinor = LibTranslateBitMaskValueToString(((MajorFunction == IRP_MJ_READ) ? ltbtIRPReadMinorFunction : ltbtIRPWriteMinorFunction), FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			LibTranslateBitMaskValueStringFree(wMinor);
			args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)Arg1)));
			args.push_back(std::make_pair(L"Key", std::to_wstring((ULONG)Arg2)));
			std::wstring byteOffsetStr;
			if (sizeof(PVOID) == 8)
				byteOffsetStr = UInt642Hex((ULONG64)Arg3 + ((ULONG64)Arg4 << 32));
			else byteOffsetStr = UInt642Hex((ULONG64)Arg3);

			args.push_back(std::make_pair(L"Byte offset", byteOffsetStr));
		} break;
		case IRP_MJ_QUERY_INFORMATION:
			args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)Arg1)));
			args.push_back(std::make_pair(L"File information class", LibTranslateEnumerationValueToString(ltetFileInformationClass, FALSE, (ULONG)Arg2)));
			break;
		case IRP_MJ_SET_INFORMATION:
			args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)Arg1)));
			args.push_back(std::make_pair(L"File information class", LibTranslateEnumerationValueToString(ltetFileInformationClass, FALSE, (ULONG)Arg2)));
			break;
		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)Arg1)));
			args.push_back(std::make_pair(L"Volume information class", LibTranslateEnumerationValueToString(ltetFileVolumeInformationClass, FALSE, (ULONG)Arg2)));
			break;
		case IRP_MJ_DIRECTORY_CONTROL:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPDirectoryMinorFunction, FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			break;
		case IRP_MJ_FILE_SYSTEM_CONTROL:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPFileSystemMinorFunction, FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			if (MinorFunction == 1 || MinorFunction == 2) {
				args.push_back(std::make_pair(L"Device object", Ptr2Hex(Arg2)));
			} else if (MinorFunction == 0 || MinorFunction == 4) {
				args.push_back(std::make_pair(L"Output buffer length", std::to_wstring((ULONG)Arg1)));
				args.push_back(std::make_pair(L"Input buffer length", std::to_wstring((ULONG)Arg2)));
				args.push_back(std::make_pair(L"FSCTL", UInt642Hex((ULONG)Arg3)));
				args.push_back(std::make_pair(L"Type3InputBuffer", Ptr2Hex(Arg4)));
			}

			break;
		case IRP_MJ_PNP:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPPnPMinorFunction, FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			switch (MinorFunction) {
				case 0x7:
					args.push_back(std::make_pair(L"Device relation type", std::to_wstring((ULONG)Arg1)));
					break;
				case 0x0C:
					args.push_back(std::make_pair(L"Device text type", std::to_wstring((ULONG)Arg1)));
					break;
				case 0x12:
					args.push_back(std::make_pair(L"Lock", std::to_wstring((ULONG)Arg1)));
					break;
				case 0x13:
					args.push_back(std::make_pair(L"Device ID type", std::to_wstring((ULONG)Arg1)));
					break;
			}
			break;
		case IRP_MJ_POWER:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPPowerMinorFunction, FALSE, MinorFunction);
			minor = wMinor;
			break;
		case IRP_MJ_SYSTEM_CONTROL:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPSystemMinorFunction, FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			break;
		case IRP_MJ_LOCK_CONTROL:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPLockMinorFunction, FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			break;
		case IRP_MJ_FLUSH_BUFFERS:
			wMinor = LibTranslateEnumerationValueToString(ltetIRPFlushMinorFunction, FALSE, MinorFunction);
			minor = std::wstring(wMinor);
			break;
		case IRP_MJ_DEVICE_CONTROL:
		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			args.push_back(std::make_pair(L"Output buffer length", std::to_wstring((ULONG)Arg1)));
			args.push_back(std::make_pair(L"Input buffer length", std::to_wstring((ULONG)Arg2)));
			args.push_back(std::make_pair(L"IOCTL", LibTranslateGeneralIntegerValueToString(ltivtDeviceControl, FALSE, (ULONG)Arg3)));
			args.push_back(std::make_pair(L"Type3InputBuffer", Ptr2Hex(Arg4)));
			break;
	}

	return;
}

std::vector<std::pair<std::wstring, std::wstring>> GetRequestDetails(PREQUEST_HEADER h)
{
	std::vector<std::pair<std::wstring, std::wstring>> res;

	switch (h->Type) {
		case ertIRP: {
			PREQUEST_IRP r = CONTAINING_RECORD(h, REQUEST_IRP, Header);
			std::wstring major = LibTranslateGeneralIntegerValueToString(ltivtFileIRPMajorFunction, FALSE, r->MajorFunction);
			std::wstring minor;
			std::vector<std::pair<std::wstring, std::wstring>> args;
			
			PWCHAR wFlagsStr = LibTranslateIRPFLagsToString(r->MajorFunction, r->MinorFunction, r->IrpFlags & (~0x60000), FALSE);
			std::wstring flagsStr;
			if (wFlagsStr != NULL) {
				flagsStr = std::wstring(wFlagsStr);
				LibTranslateBitMaskValueStringFree(wFlagsStr);
			}

			_ParseIRPParameters(r->MajorFunction, r->MinorFunction, r->Arg1, r->Arg2, r->Arg3, r->Arg4, minor, args);
			res.push_back(std::make_pair(L"IRP address", Ptr2Hex(r->IRPAddress)));
			res.push_back(std::make_pair(L"File object", Ptr2Hex(r->FileObject)));
			res.push_back(std::make_pair(L"Major function", major));
			res.push_back(std::make_pair(L"Minor function", minor));
			res.push_back(std::make_pair(L"Flags", flagsStr));
			res.push_back(std::make_pair(L"Access mode", _AccessModeToString(r->PreviousMode)));
			res.push_back(std::make_pair(L"Requestor mode", _AccessModeToString(r->RequestorMode)));
			for (auto it = args.cbegin(); it != args.cend(); ++it)
				res.push_back(*it);

		} break;
		case ertIRPCompletion: {
			PREQUEST_IRP_COMPLETION r = CONTAINING_RECORD(h, REQUEST_IRP_COMPLETION, Header);

			res.push_back(std::make_pair(L"IRP address", Ptr2Hex(r->IRPAddress)));
			res.push_back(std::make_pair(L"Completion information", Ptr2Hex((PVOID)r->CompletionInformation)));
			res.push_back(std::make_pair(L"Completion status", LibTranslateGeneralIntegerValueToString(ltivtNTSTATUS, FALSE, (ULONG)r->CompletionStatus)));
		} break;
		case ertFastIo: {
			PREQUEST_FASTIO f = CONTAINING_RECORD(h, REQUEST_FASTIO, Header);
			bool iosbValid = false;
			std::vector<std::pair<std::wstring, std::wstring>> args;

			switch (f->FastIoType) {
				case FastIoCheckIfPossible: {
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)f->Arg3)));
					args.push_back(std::make_pair(L"Operation", ((ULONG)f->Arg4 != 0) ? L"Read" : L"Write"));
					args.push_back(std::make_pair(L"Wait", ((ULONG)f->Arg5) ? L"Yes" : L"No"));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg6)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoRead:  // the same as the FastIoWrite case
				case FastIoWrite: {
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)f->Arg3)));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg4)));
					args.push_back(std::make_pair(L"Wait", ((ULONG)f->Arg5) ? L"Yes" : L"No"));
					args.push_back(std::make_pair(L"Buffer", Ptr2Hex(f->Arg6)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoQueryBasicInfo: {
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
					if (iosbValid && f->IOSBStatus >= 0) {
						ULONG64 creationTime = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);
						LONG64 lastAccessTime = (ULONG64)f->Arg3 + ((ULONG64)f->Arg4 << 32);
						ULONG64 lastWriteTime = (ULONG64)f->Arg5 + ((ULONG64)f->Arg6 << 32);
						PWCHAR attrMask = LibTranslateBitMaskValueToString(ltbtFileAttributes, FALSE, (ULONG)f->Arg7);

						args.push_back(std::make_pair(L"Creation time", std::to_wstring(creationTime)));
						args.push_back(std::make_pair(L"Last access time", std::to_wstring(lastAccessTime)));
						args.push_back(std::make_pair(L"Last write time", std::to_wstring(lastWriteTime)));
						if (attrMask != NULL) {
							args.push_back(std::make_pair(L"File attributes", attrMask));
							LibTranslateBitMaskValueStringFree(attrMask);
						} else args.push_back(std::make_pair(L"File attributes", Ptr2Hex(f->Arg7)));
					} else {
						args.push_back(std::make_pair(L"Buffer", Ptr2Hex(f->Arg1)));
						args.push_back(std::make_pair(L"Wait", ((ULONG)f->Arg2) ? L"Yes" : L"No"));
					}
				} break;
				case FastIoQueryStandardInfo: {
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
					if (iosbValid && f->IOSBStatus >= 0) {
						ULONG64 allocationSize = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);
						LONG64 endOfFile = (ULONG64)f->Arg3 + ((ULONG64)f->Arg4 << 32);

						args.push_back(std::make_pair(L"Allocation size", std::to_wstring(allocationSize)));
						args.push_back(std::make_pair(L"End of file", std::to_wstring(endOfFile)));
						args.push_back(std::make_pair(L"Number of links", std::to_wstring((ULONG)f->Arg5)));
						args.push_back(std::make_pair(L"Directory", ((ULONG)f->Arg6) ? L"Yes" : L"No"));
						args.push_back(std::make_pair(L"DeletePending", ((ULONG)f->Arg7) ? L"Yes" : L"No"));
					} else {
						args.push_back(std::make_pair(L"Buffer", Ptr2Hex(f->Arg1)));
						args.push_back(std::make_pair(L"Wait", ((ULONG)f->Arg2) ? L"Yes" : L"No"));
					}
				} break;
				case FastIoLock: {
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);
					ULONG64 regionLength = (ULONG64)f->Arg3 + ((ULONG64)f->Arg4 << 32);
					ULONG flags = (ULONG)f->Arg5;

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"Length", std::to_wstring(regionLength)));
					args.push_back(std::make_pair(L"Fail immediately", (flags & 2) ? L"Yes" : L"No"));
					args.push_back(std::make_pair(L"Exclusive", (flags & 1) ? L"Yes" : L"No"));
					args.push_back(std::make_pair(L"ProcessId", Ptr2Hex(f->Arg6)));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg7)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoUnlockSingle: {
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);
					ULONG64 regionLength = (ULONG64)f->Arg3 + ((ULONG64)f->Arg4 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"Length", std::to_wstring(regionLength)));
					args.push_back(std::make_pair(L"ProcessId", Ptr2Hex(f->Arg5)));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg6)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoUnlockAll: {
					args.push_back(std::make_pair(L"ProcessId", Ptr2Hex(f->Arg1)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoUnlockAllByKey: {
					args.push_back(std::make_pair(L"ProcessId", Ptr2Hex(f->Arg1)));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg2)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoDeviceControl: {
					args.push_back(std::make_pair(L"IOCTL", LibTranslateGeneralIntegerValueToString(ltivtDeviceControl, FALSE, (ULONG)f->Arg1)));
					args.push_back(std::make_pair(L"Input buffer length", std::to_wstring((ULONG)f->Arg2)));
					args.push_back(std::make_pair(L"Output buffer length", std::to_wstring((ULONG)f->Arg3)));
					args.push_back(std::make_pair(L"Wait", ((ULONG)f->Arg5) ? L"Yes" : L"No"));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoDetachDevice: {
					args.push_back(std::make_pair(L"Source device", Ptr2Hex(f->Arg1)));
					args.push_back(std::make_pair(L"Target device", Ptr2Hex(f->Arg2)));
				} break;
				case FastIoQueryNetworkOpenInfo: {
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
					if (iosbValid && f->IOSBStatus >= 0) {
						ULONG64 creationTime = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);
						LONG64 lastAccessTime = (ULONG64)f->Arg3 + ((ULONG64)f->Arg4 << 32);
						ULONG64 lastWriteTime = (ULONG64)f->Arg5 + ((ULONG64)f->Arg6 << 32);
						PWCHAR attrMask = LibTranslateBitMaskValueToString(ltbtFileAttributes, FALSE, (ULONG)f->Arg7);

						args.push_back(std::make_pair(L"Creation time", std::to_wstring(creationTime)));
						args.push_back(std::make_pair(L"Last access time", std::to_wstring(lastAccessTime)));
						args.push_back(std::make_pair(L"Last write time", std::to_wstring(lastWriteTime)));
						if (attrMask != NULL) {
							args.push_back(std::make_pair(L"File attributes", attrMask));
							LibTranslateBitMaskValueStringFree(attrMask);
						} else args.push_back(std::make_pair(L"File attributes", Ptr2Hex(f->Arg7)));
					} else {
						args.push_back(std::make_pair(L"Buffer", Ptr2Hex(f->Arg1)));
						args.push_back(std::make_pair(L"Wait", ((ULONG)f->Arg2) ? L"Yes" : L"No"));
					}
				} break;
				case AcquireForModWrite: {
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					if (f->Header.Result.NTSTATUSValue >= 0)
						args.push_back(std::make_pair(L"Lock", Ptr2Hex(f->Arg3)));
				} break;
				case PrepareMdlWrite:
				case MdlRead: {  // the same as the PrepareMdlWrite case
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)f->Arg3)));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg4)));
					args.push_back(std::make_pair(L"MDL", Ptr2Hex(f->Arg5)));
					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case MdlReadComplete: 
				case MdlReadCompleteCompressed: { // the same as the MdlReadComplete case
					args.push_back(std::make_pair(L"MDL", Ptr2Hex(f->Arg1)));
				} break;
				case MdlWriteComplete: 
				case MdlWriteCompleteCompressed: { // the same as the MdlWriteComplete case
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"MDL", Ptr2Hex(f->Arg3)));
				} break;
				case FastIoWriteCompressed:
				case FastIoReadCompressed: { // The same as the FastIoWriteCompressed case
					ULONG64 fileOffset = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);

					args.push_back(std::make_pair(L"File offset", std::to_wstring(fileOffset)));
					args.push_back(std::make_pair(L"Length", std::to_wstring((ULONG)f->Arg3)));
					args.push_back(std::make_pair(L"Lock key", std::to_wstring((ULONG)f->Arg4)));
					args.push_back(std::make_pair(L"Buffer", Ptr2Hex(f->Arg5)));
					args.push_back(std::make_pair(L"Compressed info length", std::to_wstring((ULONG)f->Arg6)));
					if (f->IOSBStatus >= 0) 
						args.push_back(std::make_pair(L"MDL", Ptr2Hex(f->Arg7)));

					iosbValid = (f->Header.Result.BOOLEANValue != FALSE);
				} break;
				case FastIoQueryOpen: {
					if (f->Header.Result.BOOLEANValue) {
						ULONG64 creationTime = (ULONG64)f->Arg1 + ((ULONG64)f->Arg2 << 32);
						LONG64 lastAccessTime = (ULONG64)f->Arg3 + ((ULONG64)f->Arg4 << 32);
						ULONG64 lastWriteTime = (ULONG64)f->Arg5 + ((ULONG64)f->Arg6 << 32);
						PWCHAR attrMask = LibTranslateBitMaskValueToString(ltbtFileAttributes, FALSE, (ULONG)f->Arg7);

						args.push_back(std::make_pair(L"Creation time", std::to_wstring(creationTime)));
						args.push_back(std::make_pair(L"Last access time", std::to_wstring(lastAccessTime)));
						args.push_back(std::make_pair(L"Last write time", std::to_wstring(lastWriteTime)));
						if (attrMask != NULL) {
							args.push_back(std::make_pair(L"File attributes", attrMask));
							LibTranslateBitMaskValueStringFree(attrMask);
						} else args.push_back(std::make_pair(L"File attributes", Ptr2Hex(f->Arg7)));
					} else {
						args.push_back(std::make_pair(L"IRP address", Ptr2Hex(f->Arg1)));
						args.push_back(std::make_pair(L"Buffer", Ptr2Hex(f->Arg2)));
					}
				} break;
				case ReleaseForModWrite: {
					args.push_back(std::make_pair(L"Lock", Ptr2Hex(f->Arg1)));
				} break;
				case AcquireForCcFlush:
				case ReleaseForCcFlush: // the same as the AcquireForCcFlush case
				case AcquireFileForNtCreateSection: // the same as the AcquireForCcFlush case
				case ReleaseFileForNtCreateSection: // the same as the AcquireForCcFlush case
					break;
				default:
					break;
			}

			res.push_back(std::make_pair(L"Type", _FastIoTypeToString(f->FastIoType)));
			res.push_back(std::make_pair(L"File object", Ptr2Hex(f->FileObject)));
			res.push_back(std::make_pair(L"Access mode", _AccessModeToString(f->PreviousMode)));
			for (auto it = args.cbegin(); it != args.cend(); ++it)
				res.push_back(*it);

			if (iosbValid) {
				res.push_back(std::make_pair(L"IOSB Status", LibTranslateGeneralIntegerValueToString(ltivtNTSTATUS, FALSE, f->IOSBStatus)));
				res.push_back(std::make_pair(L"IOSB Information", std::to_wstring(f->IOSBInformation)));
			}
		} break;
		case ertStartIo: {
			PREQUEST_STARTIO s = CONTAINING_RECORD(h, REQUEST_STARTIO, Header);
			std::wstring major = LibTranslateGeneralIntegerValueToString(ltivtFileIRPMajorFunction, FALSE, s->MajorFunction);
			std::wstring minor;
			std::vector<std::pair<std::wstring, std::wstring>> args;

			PWCHAR wFlagsStr = LibTranslateIRPFLagsToString(s->MajorFunction, s->MinorFunction, s->IrpFlags & (~0x60000), FALSE);
			std::wstring flagsStr;
			if (wFlagsStr != NULL) {
				flagsStr = std::wstring(wFlagsStr);
				LibTranslateBitMaskValueStringFree(wFlagsStr);
			}

			_ParseIRPParameters(s->MajorFunction, s->MinorFunction, s->Arg1, s->Arg2, s->Arg3, s->Arg4, minor, args);
			res.push_back(std::make_pair(L"IRP address", Ptr2Hex(s->IRPAddress)));
			res.push_back(std::make_pair(L"File object", Ptr2Hex(s->FileObject)));
			res.push_back(std::make_pair(L"Major function", major));
			res.push_back(std::make_pair(L"Minor function", minor));
			res.push_back(std::make_pair(L"Flags", flagsStr));
			for (auto it = args.cbegin(); it != args.cend(); ++it)
				res.push_back(*it);

			res.push_back(std::make_pair(L"IOSB status", LibTranslateGeneralIntegerValueToString(ltivtNTSTATUS, FALSE, (ULONG)s->Status)));
			res.push_back(std::make_pair(L"IOSB information", Ptr2Hex((PVOID)s->Information)));
		} break;
		case ertAddDevice: {
		} break;
		case ertDriverUnload: {
		} break;
	}

	return res;
}

/************************************************************************/
/*                  COMMANDS                                            */
/************************************************************************/

VOID EnumerateDriversAndDevices(VOID)
{
	ULONG count = 0;
	DWORD err = ERROR_GEN_FAILURE;
	PIRPMON_DEVICE_INFO deviceInfo = NULL;
	PIRPMON_DRIVER_INFO *driverInfo = NULL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	err = IRPMonDllSnapshotRetrieve(&driverInfo, &count);
	if (err == ERROR_SUCCESS) {
		ULONG i = 0;

		for (i = 0; i < count; ++i) {
			ULONG j = 0;
			PIRPMON_DRIVER_INFO dr = driverInfo[i];

			printf("%S (%p), %u devices\n", dr->DriverName, dr->DriverObject, dr->DeviceCount);
			for (j = 0; j < dr->DeviceCount; ++j) {
				PIRPMON_DEVICE_INFO device = dr->Devices[j];
				
				printf("  %S (0x%p), attached device 0x%p\n", device->Name, device->DeviceObject, device->AttachedDevice);
			}

			printf("\n");
		}

		IRPMonDllSnapshotFree(driverInfo, count);
	} else printf("ERROR: Failed to retrieve snapshot: %u\n", err);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

VOID PrintHooks(PHOOKED_DRIVER_UMINFO HookedDrivers, ULONG Count)
{
	PHOOKED_DRIVER_UMINFO hookedDriver = HookedDrivers;
	DEBUG_ENTER_FUNCTION("HookedDrivers=0x%p; Count=%u", HookedDrivers, Count);

	for (ULONG i = 0; i < Count; ++i) {
		printf("Driver: %S (0x%p)\n", hookedDriver->DriverName, hookedDriver->DriverObject);
		printf("  Object ID:           0x%p\n", hookedDriver->ObjectId);
		printf("  Monitoring enabled:  %u\n", hookedDriver->MonitoringEnabled);
		printf("  Monitoring settings\n");
		printf("    New devices:       %u\n", hookedDriver->MonitorSettings.MonitorNewDevices);
		printf("    AddDevice:         %u\n", hookedDriver->MonitorSettings.MonitorAddDevice);
		printf("    StartIo:           %u\n", hookedDriver->MonitorSettings.MonitorStartIo);
		printf("    Unload:            %u\n", hookedDriver->MonitorSettings.MonitorUnload);
		printf("    Fast I/O:          %u\n", hookedDriver->MonitorSettings.MonitorFastIo);
		printf("    IRP:               %u\n", hookedDriver->MonitorSettings.MonitorIRP);
		printf("    IRP completion:    %u\n", hookedDriver->MonitorSettings.MonitorIRPCompletion);		
		if (hookedDriver->NumberOfHookedDevices > 0) {
			PHOOKED_DEVICE_UMINFO hookedDevice = hookedDriver->HookedDevices;

			for (ULONG j = 0; j < hookedDriver->NumberOfHookedDevices; ++j) {
				printf("  Device: %S (0x%p)\n", hookedDevice->DeviceName, hookedDevice->DeviceObject);
				printf("    Object ID:          0x%p\n", hookedDevice->ObjectId);
				printf("    Monitoring enabled: %u\n", hookedDevice->MonitoringEnabled);
				++hookedDevice;
			}
		}

		printf("\n");
		++hookedDriver;
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

VOID HookAndMonitor(int argc, PWCHAR *argv)
{
	std::vector<HANDLE> hookedDrivers;
	std::vector<HANDLE> hookedDevices;
	int i = 0;
	BOOLEAN performMonitoring = FALSE;
	DWORD err = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("argc=%u; argv=0x%p", argc, argv);

	err = CacheInit();
	if (err == ERROR_SUCCESS) {
		while (err == ERROR_SUCCESS && i < argc) {
			PWCHAR argument = argv[i];
	
			if (wcsicmp(argument, L"--hook-driver") == 0) {
				HANDLE hookHandle = NULL;
				PWCHAR driverName = NULL;
				DRIVER_MONITOR_SETTINGS ms;

				ms.MonitorAddDevice = TRUE;
				ms.MonitorFastIo = TRUE;
				ms.MonitorIRP = TRUE;
				ms.MonitorIRPCompletion = TRUE;
				ms.MonitorNewDevices = FALSE;
				ms.MonitorStartIo = TRUE;
				ms.MonitorUnload = TRUE;
				++i;
				driverName = argv[i];
				err = IRPMonDllHookDriver(driverName, &ms, &hookHandle, NULL);
				if (err == ERROR_SUCCESS) {
					hookedDrivers.push_back(hookHandle);
					printf("The driver %S has been hooked successfully\n", driverName);
				} else printf("ERROR: Unable to hook the %S driver: %u\n", driverName, err);
			} else if (wcsicmp(argument, L"--hook-driver-nd") == 0) {
				HANDLE hookHandle = NULL;
				PWCHAR driverName = NULL;
				DRIVER_MONITOR_SETTINGS ms;

				ms.MonitorAddDevice = TRUE;
				ms.MonitorFastIo = TRUE;
				ms.MonitorIRP = TRUE;
				ms.MonitorIRPCompletion = TRUE;
				ms.MonitorNewDevices = TRUE;
				ms.MonitorStartIo = TRUE;
				ms.MonitorUnload = TRUE;
				++i;
				driverName = argv[i];
				err = IRPMonDllHookDriver(driverName, &ms, &hookHandle, NULL);
				if (err == ERROR_SUCCESS) {
					hookedDrivers.push_back(hookHandle);
					printf("The driver %S has been hooked successfully\n", driverName);
				} else printf("ERROR: Unable to hook the %S driver: %u\n", driverName, err);
			} else if (wcsicmp(argument, L"--hook-device-address") == 0) {
				HANDLE hookHandle = NULL;
				PVOID deviceAddress = NULL;

				++i;
				deviceAddress = (PVOID)wcstoul(argv[i], NULL, 0);
				err = IRPMonDllHookDeviceByAddress(deviceAddress, &hookHandle, NULL);
				if (err == ERROR_SUCCESS) {
					hookedDevices.push_back(hookHandle);
					printf("The device object (0x%p) has been hooked successfully\n", deviceAddress);
				} else printf("ERROR: Unable to hook device (0x%p): %u\n", deviceAddress, err);
			} else if (wcsicmp(argument, L"--hook-device-name") == 0) {
				HANDLE hookHandle = NULL;
				PWCHAR deviceName = NULL;

				++i;
				deviceName = argv[i];
				err = IRPMonDllHookDeviceByName(deviceName, &hookHandle, NULL);
				if (err == ERROR_SUCCESS) {
					hookedDevices.push_back(hookHandle);
					printf("The device object \"%S\" has been hooked successfully\n", deviceName);
				} else printf("ERROR: Unable to hook device \"%S\": %u\n", deviceName, err);
			} else if (wcsicmp(argument, L"--unhook-driver") == 0) {
				PVOID objectId = NULL;
				
				++i;
				argument = argv[i];
				if (StringToPointer(argument, &objectId)) {
					HANDLE handle = NULL;

					err = IRPMonDllOpenHookedDriver(objectId, &handle);
					if (err == ERROR_SUCCESS) {
						err = IRPMonDllUnhookDriver(handle);
						if (err == ERROR_SUCCESS) {
							printf("The driver has been unhooked successfully\n");
						} else {
							printf("ERROR: Unable to unhook the driver: %u\n", err);
							err = IRPMonDllCloseHookedDriverHandle(handle);
							if (err != ERROR_SUCCESS)
								printf("ERROR: Failed to close the handle: %u\n", err);
						}
					} else printf("ERROR: Unable to get handle to the hooked driver: %u\n", err);
				} else {
					printf("ERROR: Bad format of the \"%S\" handle\n", argument);
					err = ERROR_INVALID_PARAMETER;
				}
			} else if (wcsicmp(argument, L"--unhook-device") == 0) {
				PVOID objectId = NULL;

				++i;
				argument = argv[i];
				if (StringToPointer(argument, &objectId)) {
					HANDLE handle = NULL;

					err = IRPMonDllOpenHookedDevice(objectId, &handle);
					if (err == ERROR_SUCCESS) {
						err = IRPMonDllUnhookDevice((HANDLE)handle);
						if (err == ERROR_SUCCESS) {
							printf("The device has been unhooked successfully\n");
						} else {
							printf("ERROR: Unable to unhook the device: %u\n", err);
							IRPMonDllCloseHookedDeviceHandle(handle);
							if (err != ERROR_SUCCESS)
								printf("ERROR: Failed to close the handle: %u\n", err);
						}
					} else printf("ERROR: Unable to get handle to the hooked device: %u\n", err);
				} else {
					printf("ERROR: Bad format of the \"%S\" handle\n", argument);
					err = ERROR_INVALID_PARAMETER;
				}
			} else if (wcsicmp(argument, L"--enumerate-hooks") == 0) {
				ULONG hookedDriversCount = 0;
				PHOOKED_DRIVER_UMINFO hookedDrivers = NULL;

				err = IRPMonDllDriverHooksEnumerate(&hookedDrivers, &hookedDriversCount);
				if (err == ERROR_SUCCESS) {
					PrintHooks(hookedDrivers, hookedDriversCount);
					IRPMonDllDriverHooksFree(hookedDrivers, hookedDriversCount);
				} else printf("ERROR: Failed to enumerate hooked drivers: %u\n", err);
			} else if (wcsicmp(argument, L"--monitor") == 0) {
				performMonitoring = TRUE;
			} else {
				printf("ERROR: Unknown argument \"%S\"\n", argv[i]);
				err = ERROR_INVALID_PARAMETER;
			}

			if (err != ERROR_SUCCESS) {
				for (auto it = hookedDrivers.begin(); it != hookedDrivers.end(); ++it) {
					DWORD err2 = IRPMonDllUnhookDriver(*it);
					if (err2 != ERROR_SUCCESS)
						printf("ERROR: Unable to unhook the driver 0x%p: %u\n", *it, err2);
				}
			}

			++i;
		}

		if (err == ERROR_SUCCESS) {
			for (auto it = hookedDrivers.begin(); it != hookedDrivers.end(); ++it) {
				err = IRPMonDllDriverStartMonitoring(*it);
				if (err != ERROR_SUCCESS) {
					printf("ERROR: Unable to active the 0x%p driver: %u\n", *it, err);
					for (auto it2 = hookedDrivers.begin(); it2 != it; ++it2) {
						err = IRPMonDllDriverStopMonitoring(*it2);
						if (err != ERROR_SUCCESS)
							printf("WARNING: Unable to deactivate driver 0x%p: %u\n", *it2, err);
					}

					break;
				}
			}

			if (err != ERROR_SUCCESS) {
				for (auto it = hookedDrivers.begin(); it != hookedDrivers.end(); ++it) {
					err = IRPMonDllUnhookDriver(*it);
					if (err != ERROR_SUCCESS)
						printf("ERROR: Unable to unhook the 0x%p driver: %u\n", *it, err);
				}
			}
		}

		if (err != ERROR_SUCCESS)
			CacheFinit();
	}

	if (err == ERROR_SUCCESS) {
		HANDLE hSemaphore = NULL;
		HANDLE hEvent = NULL;

		if (performMonitoring) {
			hSemaphore = CreateSemaphoreW(NULL, 0, 0x7FFFFFFF, NULL);
			if (hSemaphore != NULL) {
				hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
				if (hEvent != NULL) {
					err = IRPMonDllConnect(hSemaphore);
					if (err == ERROR_SUCCESS) {
						DWORD waitRes = WAIT_FAILED;
						HANDLE objectsToWait[] = {hSemaphore, hEvent};
						ULONG numObjectsToWait = sizeof(objectsToWait) / sizeof(HANDLE);
						BOOLEAN terminate = FALSE;
						BOOLEAN disocnnected = FALSE;

						while (!terminate) {
							waitRes = WaitForMultipleObjects(numObjectsToWait, objectsToWait, FALSE, INFINITE);
							switch (waitRes) {
								REQUEST_FASTIO r;

								case WAIT_OBJECT_0:
									err = IRPMonDllGetRequest(&r.Header, sizeof(r));
									if (err == ERROR_SUCCESS) {
										switch (r.Header.Type) {
											case ertIRP:
												printf("IRP: ");
												break;
											case ertIRPCompletion:
												printf("IRPCOMPLETE: ");
												break;
											case ertFastIo:
												printf("FASTIO: ");
												break;
											case ertAddDevice:
												printf("ADDDEVICE: ");
												break;
											case ertStartIo:
												printf("STARTIO: ");
												break;
											case ertDriverUnload:
												printf("UNLOAD: ");
												break;
											default:
												printf("UNKNOWN (%u): ", r.Header.Type);
												break;
										}

										std::wstring driverName = CacheDriverNameGet(r.Header.Driver);
										std::wstring deviceName = CacheDeviceNameGet(r.Header.Device);
										if (driverName != L"")
											printf("%S: ", driverName.data());
										else printf("(0x%p): ", r.Header.Driver);

										if (deviceName != L"")
											printf("%S\n", deviceName.data());
										else printf("(0x%p)\n", r.Header.Device);

										std::vector<std::pair<std::wstring, std::wstring>> info = GetRequestDetails(&r.Header);
										std::wstring res = GetRequestResult(&r.Header);
										for (auto it = info.cbegin(); it != info.cend(); ++it)
											printf("  %S: %S\n", it->first.data(), it->second.data());

										printf("  Result: %S\n", res.data());
										printf("\n");
										fflush(stdout);
									} else printf("ERROR: failed to get request: %u\n", err);
									break;
								case WAIT_OBJECT_0 + 1:
									if (!disocnnected) {
										IRPMonDllDisconnect();
										disocnnected = TRUE;
									} else terminate = TRUE;
									break;
								case WAIT_TIMEOUT:
									break;
								default:
									break;
							}
						}
					}

					CloseHandle(hEvent);
				} else err = GetLastError();

				CloseHandle(hSemaphore);
			} else err = GetLastError();
		}

		CacheFinit();
		for (auto it = hookedDevices.begin(); it != hookedDevices.end(); ++it)
			IRPMonDllCloseHookedDeviceHandle(*it);

		hookedDevices.clear();
		for (auto it = hookedDrivers.begin(); it != hookedDrivers.end(); ++it)
			IRPMonDllCloseHookedDriverHandle(*it);

		hookedDrivers.clear();
	}

	fflush(stdout);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                               MAIN FUNCTIONS                         */
/************************************************************************/

int wmain( int argc, wchar_t *argv[])
{
	DWORD ret = ERROR_GEN_FAILURE;

	assert(sizeof(REQUEST_FASTIO) >= sizeof(REQUEST_IRP));
	switch (argc) {
		case 2:
			if (wcsicmp(argv[1], L"--install") == 0) {
				PWCHAR fn = NULL;

				ret = Install(L"irpmndrv.sys", L"IRPMon", SERVICE_DEMAND_START, &fn);
				if (ret == ERROR_SUCCESS)
					printf("SUCCESS\n");
				else printf("ERROR %S(): %u\n", fn, ret);
			} else if (wcsicmp(argv[1], L"--uninstall") == 0) {
				ret = Uninstall(L"IRPMon");
				if (ret == ERROR_SUCCESS)
					printf("SUCCESS\n");
				else printf("ERROR %u\n", ret);
			} else if (wcsicmp(argv[1], L"--enumerate") == 0) {
				ret = IRPMonDllInitialize();
				if (ret == ERROR_SUCCESS) {
					EnumerateDriversAndDevices();
					IRPMonDllFinalize();
				} else printf("ERROR: Failed to initialize irpmondll.dll: %u\n", ret);
			} else {
				ret = LibTranslateInitialize();
				if (ret == ERROR_SUCCESS) {
					ret = IRPMonDllInitialize();
					if (ret == ERROR_SUCCESS) {
						HookAndMonitor(argc - 1, argv + 1);
						IRPMonDllFinalize();
					} else printf("ERROR: Failed to initialize the irpmndrv.dll library: %u", ret);

					LibTranslateFinalize();
				} else printf("ERROR: Failed to initialize the libtranslate.dll library: %u", ret);
			}
			break;
		default:
			ret = LibTranslateInitialize();
			if (ret == ERROR_SUCCESS) {
				ret = IRPMonDllInitialize();
				if (ret == ERROR_SUCCESS) {
					HookAndMonitor(argc - 1, argv + 1);
					IRPMonDllFinalize();
				} else printf("ERROR: Failed to initialize the irpmndrv.dll library: %u", ret);
			
				LibTranslateFinalize();
			} else printf("ERROR: Failed to initialize the libtranslate.dll library: %u", ret);
			break;
	}

	return ret;
}
