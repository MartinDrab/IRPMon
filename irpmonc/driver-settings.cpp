
#include <errno.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <windows.h>
#include "general-types.h"
#include "irpmondll-types.h"
#include "irpmondll.h"
#include "irpmonc.h"
#include "driver-settings.h"


static std::set<EOptionType> _settingsSpecified;
static IRPMNDRV_SETTINGS _localSettings;
static IRPMNDRV_SETTINGS _driverSettings;
static BOOLEAN _settingsSave = FALSE;


static int _parse_bool(const wchar_t *Value, PBOOLEAN Result)
{
	int ret = 0;
	const wchar_t *trueValues[] = {
		L"yes",
		L"true",
		L"1",
	};
	const wchar_t *falseValues[] = {
		L"no",
		L"false",
		L"0",
	};

	ret = EINVAL;
	for (size_t i = 0; i < sizeof(trueValues) / sizeof(trueValues[0]); ++i) {
		if (wcsicmp(trueValues[i], Value) == 0) {
			*Result = TRUE;
			ret = 0;
			break;
		} else if (wcsicmp(falseValues[i], Value) == 0) {
			*Result = FALSE;
			ret = 0;
			break;
		}
	}

	return ret;
}



extern "C" int parse_setting(EOptionType Type, const wchar_t *Value)
{
	int ret = 0;
	wchar_t *endPtr = nullptr;

	switch (Type) {
		case otClearOnDisconnect:
			ret = _parse_bool(Value, &_localSettings.ReqQueueClearOnDisconnect);
			break;
		case otCollectDisconnected:
			ret = _parse_bool(Value, &_localSettings.ReqQueueCollectWhenDisconnected);
			break;
		case otProcessEventsCollect:
			ret = _parse_bool(Value, &_localSettings.ProcessEventsCollect);
			break;
		case otFileObjectEventsCollect:
			ret = _parse_bool(Value, &_localSettings.FileObjectEventsCollect);
			break;
		case otDriverSnapshotEventsCollect:
			ret = _parse_bool(Value, &_localSettings.DriverSnapshotEventsCollect);
			break;
		case otProcessEmulateOnConnect:
			ret = _parse_bool(Value, &_localSettings.ProcessEmulateOnConnect);
			break;
		case otDriverSnapshotOnConnect:
			ret = _parse_bool(Value, &_localSettings.DriverSnapshotOnConnect);
			break;
		case otDataStripThreshold:
			endPtr = (wchar_t *)Value;
			_localSettings.DataStripThreshold = wcstoul(Value, &endPtr, 0);
			if (_localSettings.DataStripThreshold == ULONG_MAX && errno == ERANGE)
				ret = errno;
			else if (endPtr == Value)
				ret = EINVAL;
			break;
		case otStripData:
			ret = _parse_bool(Value, &_localSettings.StripData);
			break;
		case otBootLog:
			ret = _parse_bool(Value, &_localSettings.LogBoot);
			break;
		case otSettingsSave:
			ret = _parse_bool(Value, &_settingsSave);
			break;
		default:
			ret = -10;
			fprintf(stderr, "[ERROR]: Unknown driver setting specified\n");
			break;
	}

	if (ret == 0)
		_settingsSpecified.insert(Type);

	return ret;
}


extern "C" int sync_settings(void)
{
	int ret = 0;

	ret = IRPMonDllSettingsQuery(&_driverSettings);
	if (ret == 0) {
		for (auto & v : _settingsSpecified) {
			switch (v) {
				case otClearOnDisconnect:
					_driverSettings.ReqQueueClearOnDisconnect = _localSettings.ReqQueueClearOnDisconnect;
					break;
				case otCollectDisconnected:
					_driverSettings.ReqQueueCollectWhenDisconnected = _localSettings.ReqQueueCollectWhenDisconnected;
					break;
				case otProcessEventsCollect:
					_driverSettings.ProcessEventsCollect = _localSettings.ProcessEventsCollect;
					break;
				case otFileObjectEventsCollect:
					_driverSettings.FileObjectEventsCollect = _localSettings.FileObjectEventsCollect;
					break;
				case otDriverSnapshotEventsCollect:
					_driverSettings.DriverSnapshotEventsCollect = _localSettings.DriverSnapshotEventsCollect;
					break;
				case otProcessEmulateOnConnect:
					_driverSettings.ProcessEmulateOnConnect = _localSettings.ProcessEmulateOnConnect;
					break;
				case otDriverSnapshotOnConnect:
					_driverSettings.DriverSnapshotOnConnect = _localSettings.DriverSnapshotOnConnect;
					break;
				case otDataStripThreshold:
					_driverSettings.DataStripThreshold = _localSettings.DataStripThreshold;
					break;
				case otStripData:
					_driverSettings.StripData = _localSettings.StripData;
					break;
				case otBootLog:
					_driverSettings.LogBoot = _localSettings.LogBoot;
					break;
			}
		}

		if (_settingsSpecified.size() > 0)
			ret = IRPMonDllSettingsSet(&_driverSettings, _settingsSave);
	}

	return ret;
}


extern "C" void print_settings(void)
{
	fprintf(stderr, "[INFO]: Driver settings:\n");
	fprintf(stderr, "[INFO]:   Clear on disconnect:         %u\n", _driverSettings.ReqQueueClearOnDisconnect);
	fprintf(stderr, "[INFO]:   Collect when disconnected:   %u\n", _driverSettings.ReqQueueCollectWhenDisconnected);
	fprintf(stderr, "[INFO]:   Collect process events:      %u\n", _driverSettings.ProcessEventsCollect);
	fprintf(stderr, "[INFO]:   Collect file name events:    %u\n", _driverSettings.FileObjectEventsCollect);
	fprintf(stderr, "[INFO]:   Collect object name events:  %u\n", _driverSettings.DriverSnapshotEventsCollect);
	fprintf(stderr, "[INFO]:   Process snapshot on connect: %u\n", _driverSettings.ProcessEmulateOnConnect);
	fprintf(stderr, "[INFO]:   Driver snapshot on connect:  %u\n", _driverSettings.DriverSnapshotOnConnect);
	fprintf(stderr, "[INFO]:   Strip data:                  %u\n", _driverSettings.StripData);
	fprintf(stderr, "[INFO]:   Data strip threshold:        %u bytes\n", _driverSettings.DataStripThreshold);
	fprintf(stderr, "[INFO]:   Log boot:                    %u\n", _driverSettings.LogBoot);
	fprintf(stderr, "[INFO]:   Save to registry:            %u\n", _settingsSave);
	fprintf(stderr, "[INFO]: \n");

	return;
}