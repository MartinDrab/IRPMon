
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include "stop-event.h"



#define STOP_EVENT_PREFIX			L"Global\\irpmonc"


static HANDLE _allStopEvent = NULL;
static HANDLE _processStopEvent = NULL;


int stop_event_create(void)
{
	int ret = 0;
	wchar_t eventName[sizeof(STOP_EVENT_PREFIX) / sizeof(wchar_t) + 30];

	memset(eventName, 0, sizeof(eventName));
	swprintf(eventName, sizeof(eventName) / sizeof(eventName[0]), STOP_EVENT_PREFIX L"-%u", GetCurrentProcessId());
	_processStopEvent = CreateEventW(NULL, TRUE, FALSE, eventName);
	if (_processStopEvent != NULL) {
		memset(eventName, 0, sizeof(eventName));
		wcscpy(eventName, STOP_EVENT_PREFIX);
		_allStopEvent = CreateEventW(NULL, TRUE, FALSE, eventName);
		if (_allStopEvent == NULL)
			ret = GetLastError();

		if (ret != 0)
			CloseHandle(_processStopEvent);
	} else ret = GetLastError();

	return ret;
}


int stop_event_oepn(int ProcessId)
{
	int ret = 0;
	wchar_t eventName[sizeof(STOP_EVENT_PREFIX) / sizeof(wchar_t) + 30];
	PHANDLE peventHandle = NULL;

	memset(eventName, 0, sizeof(eventName));
	if (ProcessId != 0) {
		swprintf(eventName, sizeof(eventName) / sizeof(eventName[0]), STOP_EVENT_PREFIX L"-%u", GetCurrentProcessId());
		peventHandle = &_processStopEvent;
	} else {
		wcscpy(eventName, STOP_EVENT_PREFIX);
		peventHandle = &_allStopEvent;
	}

	*peventHandle = OpenEventW(EVENT_ALL_ACCESS, FALSE, eventName);
	if (*peventHandle == NULL)
		ret = GetLastError();

	return ret;
}


int stop_event_wait(unsigned int Timeout)
{
	int ret = 0;
	HANDLE otw[2];
	int count = 0;

	if (_processStopEvent != NULL) {
		otw[count] = _processStopEvent;
		++count;
	}

	if (_allStopEvent != NULL) {
		otw[count] = _allStopEvent;
		++count;
	}

	ret = WaitForMultipleObjects(count, otw, FALSE, Timeout);
	if (ret >= WAIT_OBJECT_0 && ret <= WAIT_OBJECT_0 + count)
		ret = 0;

	return ret;
}


void stop_event_set(void)
{
	if (_allStopEvent != NULL)
		SetEvent(_allStopEvent);

	if (_processStopEvent != NULL)
		SetEvent(_processStopEvent);

	return;	
}


void stop_event_finit(void)
{
	if (_allStopEvent != NULL)
		CloseHandle(_allStopEvent);

	if (_processStopEvent != NULL)
		CloseHandle(_processStopEvent);

	return;
}
