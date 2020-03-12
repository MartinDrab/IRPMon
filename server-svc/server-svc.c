
#include <windows.h>
#include "libserver.h"


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

#define DEFAULT_SERVER_ADDRESS		"0.0.0.0"
#define DEFAULT_SERVER_PORT			"1234"


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

	_address = strdup(argc >= 2 ? argv[1] : DEFAULT_SERVER_ADDRESS);
	if (_address == NULL)
		ret = ENOMEM;

	if (ret == 0) {
		_port = strdup(argc == 3 ? argv[2] : DEFAULT_SERVER_PORT);
		if (_port == NULL) {
			free(_address);
			ret = ENOMEM;
		}
	}

	if (ret == 0) {
		memset(svcTable, 0, sizeof(svcTable));
		svcTable[0].lpServiceName = L"IRPMonSvc";
		svcTable[0].lpServiceProc = ServiceMain;
		if (!StartServiceCtrlDispatcherW(svcTable))
			ret = GetLastError();
	
		free(_port);
		free(_address);
	}

	return ret;
}
