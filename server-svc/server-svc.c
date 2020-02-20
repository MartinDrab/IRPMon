
#include <windows.h>
#include "libserver.h"


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static SERVICE_STATUS _statusRecord;
static SERVICE_STATUS_HANDLE _statusHandle = NULL;
static HANDLE _exitEventHandle = NULL;
static char *_address = NULL;
static char *_port = NULL;


static DWORD _ReportError(const char *Text, DWORD Code)
{

	return Code;
}


static DWORD WINAPI _IRPMonServiceHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext
)
{
	DWORD ret = NO_ERROR;

	switch (dwControl) {
		case SERVICE_CONTROL_STOP:
			_statusRecord.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(_statusHandle, &_statusRecord);
//			NetPipeTerminate();
			SetEvent(_exitEventHandle);
			break;
		default:
			break;
	}

	return ret;
}


void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
	DWORD dwError = 0;

	memset(&_statusRecord, 0, sizeof(_statusRecord));
	_statusHandle = RegisterServiceCtrlHandlerExW(L"IRPMonSvc", _IRPMonServiceHandlerEx, NULL);
	if (_statusHandle != NULL) {
		_statusRecord.dwCurrentState = SERVICE_START_PENDING;
		_statusRecord.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		if (SetServiceStatus(_statusHandle, &_statusRecord)) {
			_exitEventHandle = CreateEventW(NULL, TRUE, FALSE, NULL);
			if (_exitEventHandle == NULL) {
				_statusRecord.dwCurrentState = SERVICE_STOPPED;
				SetServiceStatus(_statusHandle, &_statusRecord);
			}
		}

		if (_exitEventHandle == NULL)
			_statusHandle = NULL;
	}

	if (_statusHandle != NULL) {
		if (dwError == ERROR_SUCCESS) {
			_statusRecord.dwCurrentState = SERVICE_RUNNING;
			_statusRecord.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			SetServiceStatus(_statusHandle, &_statusRecord);
			dwError = IRPMonServerStart(_address, _port, _exitEventHandle);
			WaitForSingleObject(_exitEventHandle, INFINITE);
			_statusRecord.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(_statusHandle, &_statusRecord);
			CloseHandle(_exitEventHandle);
		}

		_statusRecord.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(_statusHandle, &_statusRecord);
	}

	return;
}


int main(int argc, char *argv[])
{
	int ret = 0;
	SERVICE_TABLE_ENTRYW svcTable[2];

	switch (argc) {
		case 2:
			_address = strdup(argv[1]);
			if (_address == NULL)
				ret = ENOMEM;
		case 3:
			if (ret == 0) {
				_port = strdup(argv[2]);
				if (_port == 0)
					ret = ENOMEM;
			}
			break;
	}

	if (argc == 3) {
		memset(svcTable, 0, sizeof(svcTable));
		svcTable[0].lpServiceName = L"IRPMonSvc";
		svcTable[0].lpServiceProc = ServiceMain;
		if (!StartServiceCtrlDispatcherW(svcTable))
			ret = GetLastError();
	}

	return ret;
}
