
#include <windows.h>
#include "libserver.h"


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static SERVICE_STATUS _statusRecord;
static SERVICE_STATUS_HANDLE _statusHandle = NULL;
static HANDLE _exitEventHandle = NULL;
static int _argc = 0;
static char **_argv = NULL;


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
	int argCount = 0;
	char **args = NULL;

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
		argCount = _argc;
		args = _argv;
		if (dwError == ERROR_SUCCESS) {
			_statusRecord.dwCurrentState = SERVICE_RUNNING;
			_statusRecord.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			SetServiceStatus(_statusHandle, &_statusRecord);
			dwError = IRPMonServerStart(args[0], args[1], _exitEventHandle);
			WaitForSingleObject(_exitEventHandle, INFINITE);
			_statusRecord.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(_statusHandle, &_statusRecord);
			CloseHandle(_exitEventHandle);
		}

		if (_statusHandle != NULL) {
			_statusRecord.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(_statusHandle, &_statusRecord);
		}
	}

	return;
}


int main(int argc, char *argv[])
{
	int ret = 0;
	SERVICE_TABLE_ENTRYW svcTable[2];

	_argc = argc - 1;
	_argv = argv + 1;
	memset(svcTable, 0, sizeof(svcTable));
	svcTable[0].lpServiceName = L"IRPMonSvc";
	svcTable[0].lpServiceProc = ServiceMain;
	if (!StartServiceCtrlDispatcherW(svcTable))
		ret = GetLastError();

	return ret;
}
