
#include <stdint.h>
#include <windows.h>
#include "general-types.h"
#include "hexer.h"




static BOOLEAN _displayAddress = TRUE;
static BOOLEAN _displayCharValues = TRUE;
static BOOLEAN _displayUpperDigits = FALSE;



static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const unsigned char *data = NULL;
	size_t dataLen = 0;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_STARTIO *startIo = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	wchar_t lowers[] = {L"0123456789abcdef"};
	wchar_t uppers[] = { L"0123456789ABCDEF" };
	const wchar_t *digits[] = {lowers, uppers};
	size_t lineCount = 0;
	size_t addressSize = 0;
	size_t totalLineChars = 0;
	wchar_t **tmpLines = NULL;
	wchar_t *line = NULL;
	size_t addr = 0;

	ret = ERROR_SUCCESS;
	switch (Request->Type) {
		case ertIRP:
			irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
			data = (unsigned char *)(irp + 1);
			dataLen = irp->DataSize;
			break;
		case ertIRPCompletion:
			irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
			data = (unsigned char *)(irpComp + 1);
			dataLen = irpComp->DataSize;
			break;
		case ertStartIo:
			startIo = CONTAINING_RECORD(Request, REQUEST_STARTIO, Header);
			data = (unsigned char *)(startIo + 1);
			dataLen = startIo->DataSize;
			break;
		default:
			ret = ERROR_NOT_SUPPORTED;
			break;
	}

	if (ret == ERROR_SUCCESS) {
		lineCount = (dataLen + 15) / 16;
		if (_displayAddress) {
			addressSize = 31;
			while (addressSize > 0 && (size_t)(1 << addressSize) >= lineCount * 16)
				--addressSize;

			++addressSize;
			addressSize = (addressSize + 3) / 4;
			totalLineChars += (addressSize + 2);
		}

		totalLineChars += (16 * 3 - 1);
		if (_displayCharValues)
			totalLineChars += (16 + 1);

		tmpLines = (wchar_t **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lineCount*sizeof(wchar_t *));
		if (tmpLines != NULL) {
			size_t bytesToDo = 0;

			for (size_t i = 0; i < lineCount; ++i) {
				line = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (totalLineChars + 1) * sizeof(wchar_t));
				if (line != NULL) {
					for (size_t j = 0; j < totalLineChars; ++j)
						line[j] = L' ';

					if (_displayAddress) {
						line[addressSize] = L':';
						line[addressSize + 1] = L'\t';
						for (size_t j = 0; j < addressSize; ++j)
							line[j] = digits[_displayUpperDigits][((addr >> ((addressSize - j - 1) * 4)) & 0xf)];
					}

					bytesToDo = (dataLen >= 16) ? 16 : dataLen;
					for (size_t j = 0; j < bytesToDo; ++j) {
						line[addressSize + 2 + j * 3] = digits[_displayUpperDigits][(data[j] >> 4)];
						line[addressSize + 2 + j * 3 + 1] = digits[_displayUpperDigits][(data[j] & 0xf)];
						line[addressSize + 2 + j * 3 + 2] = L' ';
						if (_displayCharValues)
							line[addressSize + 2 + 16 * 3 + j] = (data[j] >= L' ') ? data[j] : L'.';
					}

					if (_displayCharValues)
						line[addressSize + 2 + 16 * 3 - 1] = L'\t';
				
					tmpLines[i] = line;
				} else ret = GetLastError();

				if (ret != ERROR_SUCCESS) {
					for (size_t j = 0; j < i; ++j)
						HeapFree(GetProcessHeap(), 0, tmpLines[j]);
				
					break;
				}

				data += 16;
				dataLen -= 16;
				addr += 16;
			}

			if (ret == ERROR_SUCCESS) {
				*Names = NULL;
				*Values = tmpLines;
				*RowCount = lineCount;
				*Handled = TRUE;
			}

			if (ret != ERROR_SUCCESS)
				HeapFree(GetProcessHeap(), 0, tmpLines);
		} else ret = GetLastError();
	} else if (ret == ERROR_NOT_SUPPORTED) {
		*Handled = TRUE;
		ret = ERROR_SUCCESS;
	}

	return ret;
}


static void cdecl _FreeRoutine(wchar_t **Names, wchar_t **Values, size_t Count)
{
	for (size_t i = 0; i < Count; ++i)
		HeapFree(GetProcessHeap(), 0, Values[i]);

	HeapFree(GetProcessHeap(), 0, Values);


	return;
}



DWORD cdecl DP_INIT_ROUTINE_NAME(PIRPMON_DATA_PARSER Parser)
{
	RtlSecureZeroMemory(Parser, sizeof(IRPMON_DATA_PARSER));
	Parser->MajorVersion = 1;
	Parser->MinorVersion = 0;
	Parser->BuildVersion = 0;
	Parser->Name = L"Hexer";
	Parser->Priority = 1;
	Parser->ParseRoutine = _ParseRoutine;
	Parser->FreeRoutine = _FreeRoutine;

	return ERROR_SUCCESS;
}
