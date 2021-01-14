
#include <string>
#include <map>
#include <vector>
#include <windows.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "general-types.h"
#include "dparser.h"
#include "reqlist.h"
#include "irpmondll.h"
#include "driver-hook.h"


//	--input=[L|D|N]:<value>
//		D:\\.\IRPMon
//		N:localhost:1234
//		L:C:\binarylog.log
//
//	--output=[T|J|B]:[filename|-]
//		- = stdout
//		J = JSON lines, the array mode is not supported
//
//	--hook-driver=[ICFASUNDW]:<drivername>
//		I = IRP
//		C = IRP completion
//		F = fast IO
//		A = AddDevice
//		S = StartIo
//		U = Unload
//		N = New devices
//		D = Associated data
//		W = Name watch
//		E = device extension hook
//		a = all devices
//
//	--unhook-driver=[W]:<drivername>
//		W = name watch
//
//	--boot-log={no|yes}


typedef enum _EOptionType {
	otInput,
	otOutput,
	otHookDriver,
	otUnhookDriver,
	otHookDevice,
	otBootLog,
} EOptionType, *PEOptionType;

typedef struct _OPTION_RECORD {
	EOptionType Type;
	const wchar_t *Name;
	bool Required;
	int Count;
	int MaxCount;
} OPTION_RECORD, *POPTION_RECORD;

typedef struct _HOOK_DRIVER_REQUEST {
	wchar_t *DriverName;
	DRIVER_MONITOR_SETTINGS Settings;
	bool NameWatch;
	bool DevExtHook;
	bool AllDevices;
	HANDLE Handle;
	void *ObjectId;
} HOOK_DRIVER_REQUEST, *PHOOK_DRIVER_REQUEST;

typedef struct _UNHOOK_DRIVER_REQUEST {
	wchar_t *DriverName;
	bool NameWatch;
} UNHOOK_DRIVER_REQUEST, *PUNHOOK_DRIVER_REQUEST;

typedef struct _HOOK_DEVICE_REQUEST {
	wchar_t *Name;
	PVOID Address;
	HANDLE Handle;
	void *ObjectId;
} HOOK_DEVICE_REQUEST, *PHOOK_DEVICE_REQUEST;


static 	OPTION_RECORD opts[] = {
	{otInput, L"input", true, 0, 1},
	{otOutput, L"output", true, 0, 1},
	{otHookDriver, L"hook-driver", false, 0, INT_MAX},
	{otUnhookDriver, L"unhook-driver", false, 0, INT_MAX},
	{otHookDevice, L"hook-device", false, 0, INT_MAX},
	{otBootLog, L"boot-log", false, 0, 1},
};


static wchar_t *_inLogFileName = NULL;
static wchar_t *_outLogFileName = NULL;
static ERequestLogFormat _outLogFormat = rlfBinary;
static IRPMON_INIT_INFO _initInfo;
static std::vector<HOOK_DRIVER_REQUEST> _driversToHook;
static std::map<std::wstring, HOOK_DRIVER_REQUEST> _hookedDrivers;
static std::vector<UNHOOK_DRIVER_REQUEST> _unhookDrivers;
static HANDLE _dpListHandle = NULL;
static HANDLE _reqListHandle = NULL;


static int _parse_input(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t* delimiter = NULL;

	delimiter = wcschr(Value, L':');
	if (delimiter == NULL) {
		ret = -7;
		fprintf(stderr, "[ERROR]: The input specification must contain the \":\" delimiter\n");
		goto Exit;
	}

	if (delimiter != Value + 1) {
		ret = -8;
		fprintf(stderr, "[ERROR]: Only one modifier can be present before the \":\" delimiter\n");
		goto Exit;
	}

	switch (*Value) {
		case L'L': _initInfo.ConnectorType = ictNone; break;
		case L'D': _initInfo.ConnectorType = ictDevice; break;
		case L'N': _initInfo.ConnectorType = ictNetwork; break;
		default:
			ret = -9;
			fprintf(stderr, "[ERROR]: Unknown input modifier \"%lc\"\n", *Value);
			goto Exit;
			break;
	}

	if (ret == 0) {
		Value = delimiter + 1;
		switch (_initInfo.ConnectorType) {
			case ictNone:
				_inLogFileName = wcsdup(Value);
				if (Value == NULL)
					ret = ENOMEM;
				break;
			case ictDevice:
				_initInfo.Data.Device.DeviceName = wcsdup(Value);
				if (_initInfo.Data.Device.DeviceName == NULL)
					ret = ENOMEM;
				break;
			case ictNetwork:
				_initInfo.Data.Network.Service = NULL;
				_initInfo.Data.Network.Address = wcsdup(Value);
				if (_initInfo.Data.Network.Address == NULL)
					ret = ENOMEM;
				break;
		}

		if (ret != 0) {
			fprintf(stderr, "[ERROR]: Unable to initialize init info for the IRPMon library: %u\n", ret);
			goto Exit;
		}
	}

Exit:
	return ret;
}


static int _parse_output(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t* delimiter = NULL;

	delimiter = wcschr(Value, L':');
	if (delimiter == NULL) {
		ret = -7;
		fprintf(stderr, "[ERROR]: The output specification must contain the \":\" delimiter\n");
		goto Exit;
	}

	if (delimiter != Value + 1) {
		ret = -8;
		fprintf(stderr, "[ERROR]: Only one modifier can be present before the \":\" delimiter\n");
		goto Exit;
	}

	switch (*Value) {
		case L'B': _outLogFormat = rlfBinary; break;
		case L'J': _outLogFormat = rlfJSONLines; break;
		case L'T': _outLogFormat = rlfText; break;
		default:
			ret = -10;
			fprintf(stderr, "[ERROR]: Invalid output modifier \"%lc\"\n", *Value);
			goto Exit;
			break;
	}

	Value = delimiter + 1;
	if (wcscmp(Value, L"-") == 0) {
#ifdef _WIN32
		if (_outLogFormat == rlfBinary)
			setmode(fileno(stdout), O_BINARY);
#endif
	} else {
		_outLogFileName = wcsdup(Value);
		if (_outLogFileName == NULL) {
			ret = ENOMEM;
			fprintf(stderr, "[ERROR]: Cannot allocate memory to hold the output log file name\n");
		}
	}

Exit:
	return ret;
}


static int _parse_hookdriver(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t *delimiter = NULL;
	HOOK_DRIVER_REQUEST hdr;

	memset(&hdr, 0, sizeof(hdr));
	delimiter = wcschr(Value, L':');
	if (delimiter != NULL) {
		while (ret == 0 && Value != delimiter) {
			switch (*Value) {
				case L'I': hdr.Settings.MonitorIRP = TRUE; break;
				case L'C': hdr.Settings.MonitorIRPCompletion = TRUE; break;
				case L'F': hdr.Settings.MonitorFastIo = TRUE;  break;
				case L'S': hdr.Settings.MonitorStartIo = TRUE; break;
				case L'A': hdr.Settings.MonitorAddDevice = TRUE; break;
				case L'U': hdr.Settings.MonitorUnload = TRUE; break;
				case L'N': hdr.Settings.MonitorNewDevices = TRUE; break;
				case L'D': hdr.Settings.MonitorData = TRUE; break;
				case L'E': hdr.DevExtHook = true; break;
				case L'W': hdr.NameWatch = true; break;
				case L'a': hdr.AllDevices = true; break;
				default:
					ret = -6;
					fprintf(stderr, "[ERROR]: Unknown driver hooking modifier \"%lc\"\n", *Value);
					break;
			}

			++Value;
		}

		++Value;
	}

	if (ret == 0) {
		hdr.DriverName = wcsdup(Value);
		if (hdr.DriverName == NULL)
			ret = ENOMEM;
	}

	if (ret == 0) {
		for (size_t i = 0; i < sizeof(hdr.Settings.IRPSettings) / sizeof(hdr.Settings.IRPSettings[0]); ++i)
			hdr.Settings.IRPSettings[i] = 1;
		
		for (size_t i = 0; i < sizeof(hdr.Settings.FastIoSettings) / sizeof(hdr.Settings.FastIoSettings[0]); ++i)
			hdr.Settings.FastIoSettings[i] = 1;

		_driversToHook.push_back(hdr);
	}

	return ret;
}


static int _parse_unhookdriver(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t* delimiter = NULL;
	UNHOOK_DRIVER_REQUEST udr;

	memset(&udr, 0, sizeof(udr));
	delimiter = wcschr(Value, L':');
	if (delimiter != NULL) {
		if (delimiter != Value + 1) {
			ret = -8;
			fprintf(stderr, "[ERROR]: Only one modifier can be present before the \":\" delimiter\n");
			goto Exit;
		}

		switch (*Value) {
			case L'W': udr.NameWatch = true; break;
			default:
				ret = -6;
				fprintf(stderr, "[ERROR]: Unknown driver unhooking modifier \"%lc\"\n", *Value);
				break;
		}

		Value = delimiter + 1;
	}

	if (ret == 0) {
		udr.DriverName = wcsdup(Value);
		if (udr.DriverName == NULL) {
			ret = ENOMEM;
			fprintf(stderr, "[ERROR]: Unable to allocate memory to hold the name of the unhooking driver\n");
		}
	
		if (ret == 0)
			_unhookDrivers.push_back(udr);
	}

Exit:
	return ret;
}


static int _parse_arg(wchar_t *Arg)
{
	int ret = 0;
	bool found = false;
	wchar_t *delimiter = NULL;
	wchar_t *name = NULL;
	wchar_t *value = NULL;
	POPTION_RECORD opRec = NULL;

	if (wcslen(Arg) < 2 || memcmp(Arg, L"--", sizeof(L"--") - sizeof(wchar_t)) != 0) {
		ret = -1;
		fprintf(stderr, "[ERROR]: The argument \"%ls\" does not start with \"--\"\n", Arg);
		goto Exit;
	}

	delimiter = wcschr(Arg, L'=');
	if (delimiter == NULL) {
		ret = -2;
		fprintf(stderr, "[ERROR]: No \"=\" delimiter in the \"%ls\" argument\n", Arg);
		goto Exit;
	}

	name = Arg + 2;
	value = delimiter + 1;
	*delimiter = L'\0';
	opRec = opts;
	for (size_t i = 0; i < sizeof(opts) / sizeof(opts[0]); ++i) {
		if (wcsicmp(opRec->Name, name) == 0) {
			++opRec->Count;
			if (opRec->Count > opRec->MaxCount) {
				ret = -3;
				fprintf(stderr, "[ERROR]: The argument \"--%ls\" can be specified at most %i times\n", opRec->Name, opRec->MaxCount);
				goto Exit;
			}

			found = true;
			break;
		}

		++opRec;
	}

	if (!found) {
		fprintf(stderr, "[ERROR]: Unknown option \"--%ls\"\n", name);
		goto Exit;
	}

	switch (opRec->Type) {
		case otInput:
			ret = _parse_input(value);
			break;
		case otOutput:
			ret = _parse_output(value);
			break;
		case otHookDriver:
			ret = _parse_hookdriver(value);
			break;
		case otUnhookDriver:
			ret = _parse_unhookdriver(value);
			break;
		case otHookDevice:
			break;
		case otBootLog:
			break;
	}

Exit:
	return ret;
}


static int _enum_hooked_objects(void)
{
	int ret = 0;
	ULONG infoCount = 0;
	PHOOKED_DRIVER_UMINFO info = NULL;
	HOOK_DRIVER_REQUEST hdr;
	ULONG dnCount = 0;
	PDRIVER_NAME_WATCH_RECORD dnArray = NULL;

	ret = IRPMonDllDriverHooksEnumerate(&info, &infoCount);
	if (ret == 0) {
		const HOOKED_DRIVER_UMINFO* tmp = NULL;

		tmp = info;
		for (size_t i = 0; i < infoCount; ++i) {
			memset(&hdr, 0, sizeof(hdr));
			hdr.Settings = tmp->MonitorSettings;
			hdr.DevExtHook = tmp->DeviceExtensionHooks;
			hdr.NameWatch = false;
			hdr.DriverName = (wchar_t *)calloc(tmp->DriverNameLen / sizeof(wchar_t) + 1, sizeof(wchar_t));
			if (hdr.DriverName != NULL) {
				memcpy(hdr.DriverName, tmp->DriverName, tmp->DriverNameLen);
				ret = IRPMonDllOpenHookedDriver(tmp->ObjectId, &hdr.Handle);
				if (ret == 0)
					_hookedDrivers.insert(std::make_pair(std::wstring(hdr.DriverName), hdr));
				
				if (ret != 0)
					free(hdr.DriverName);
			} else ret = ENOMEM;

			if (ret != 0)
				break;

			++tmp;
		}

		IRPMonDllDriverHooksFree(info, infoCount);
	}

	if (ret == 0) {
		ret = IRPMonDllDriverNameWatchEnum(&dnArray, &dnCount);
		if (ret == 0) {
			const DRIVER_NAME_WATCH_RECORD *tmp = NULL;

			tmp = dnArray;
			for (size_t i = 0; i < dnCount; ++i) {
				memset(&hdr, 0, sizeof(hdr));
				hdr.Settings = tmp->MonitorSettings;
				hdr.NameWatch = true;
				hdr.DevExtHook = false;
				hdr.DriverName = wcsdup(tmp->DriverName);
				if (hdr.DriverName != NULL) {
					_hookedDrivers.insert(std::make_pair(std::wstring(tmp->DriverName), hdr));
				} else ret = ENOMEM;
				
				if (ret != 0)
					break;

				++tmp;
			}

			IRPMonDllDriverNameWatchEnumFree(dnArray, dnCount);
		}
	}

	return ret;
}


int wmain(int argc, wchar_t *argv[])
{
	int ret = 0;
	POPTION_RECORD opRec = NULL;

	for (int i = 1; i < argc; ++i) {
		ret = _parse_arg(argv[i]);
		if (ret != 0)
			break;
	}

	if (ret == 0) {
		opRec = opts;
		for (size_t i = 0; i < sizeof(opts) / sizeof(opts[0]); ++i) {
			if (opRec->Count == 0 && opRec->Required) {
				ret = -5;
				fprintf(stderr, "[ERROR]: The required argument \"--%ls\" not specified\n", opRec->Name);
				break;
			}
		}
	}

	if (ret == 0) {
		ret = DPListModuleInit(L"dparser.dll");
		if (ret == 0) {
			ret = DPListCreate(&_dpListHandle);
			if (ret == 0)
				ret = ReqListModuleInit(L"reqlist.dll");
			
			if (ret == 0) {
				ret = ReqListCreate(&_reqListHandle);
				if (ret == 0) {
					ReqListAssignParserList(_reqListHandle, _dpListHandle);
					ret = IRPMonDllInitialize(&_initInfo);
				}

				if (ret == 0) {
					ret = _enum_hooked_objects();
					if (ret == 0) {
						for (auto & hdr : _driversToHook) {
							if (!hdr.NameWatch) {
								fprintf(stderr, "[INFO]: Hooking driver \"%ls\"...\n", hdr.DriverName);
								ret = IRPMonDllHookDriver(hdr.DriverName, &hdr.Settings, hdr.DevExtHook, &hdr.Handle, &hdr.ObjectId);
							} else {
								fprintf(stderr, "[INFO]: Starting to watch for driver \"%ls\"...\n", hdr.DriverName);
								ret = IRPMonDllDriverNameWatchRegister(hdr.DriverName, &hdr.Settings);
							}

							if (ret != 0) {
								fprintf(stderr, "[ERROR]: Failed to hook driver \"%ls\": %u\n", hdr.DriverName, ret);
								for (auto & tmp : _driversToHook) {
									if (wcscmp(tmp.DriverName, hdr.DriverName) == 0)
										break;

									if (!tmp.NameWatch) {
										fprintf(stderr, "[INFO]: Unhooking driver \"%ls\"...\n", tmp.DriverName);
										IRPMonDllUnhookDriver(tmp.Handle);
									} else {
										fprintf(stderr, "[INFO]: Stopping the watch for driver \"%ls\"...\n", tmp.DriverName);
										IRPMonDllDriverNameWatchUnregister(tmp.DriverName);
									}
								}

								break;
							}
						}
					} else fprintf(stderr, "[ERROR]: Unable to enumerate hooked objects: %u\n", ret);

					if (ret == 0) {
						for (auto & hdr : _driversToHook)
							_hookedDrivers.insert(std::make_pair(std::wstring(hdr.DriverName), hdr));

						if (ret != 0) {
							for (auto& tmp : _hookedDrivers) {
								if (!tmp.second.NameWatch) {
									fprintf(stderr, "[INFO]: Unhooking driver \"%ls\"...\n", tmp.second.DriverName);
									IRPMonDllUnhookDriver(tmp.second.Handle);
								} else {
									fprintf(stderr, "[INFO]: Stopping the watch for driver \"%ls\"...\n", tmp.second.DriverName);
									IRPMonDllDriverNameWatchUnregister(tmp.second.DriverName);
								}
							}
						}
					}

					IRPMonDllFinalize();
				} else fprintf(stderr, "[ERROR]: Unable to initialize IRPMon library: %u\n", ret);
				
				if (_reqListHandle != NULL)
					ReqListFree(_reqListHandle);

				ReqListModuleFinit();
			} else fprintf(stderr, "[ERROR]: Unable to initialize ReqList.dll: %u\n", ret);

			if (_dpListHandle != NULL)
				DPListFree(_dpListHandle);

			DPListModuleFinit();
		} else fprintf(stderr, "[ERROR]: Unable to initialize DParser.dll: %u\n", ret);
	}

	return ret;
}
