
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <winsock2.h>
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
#include "callback-stream.h"
#include "symbols.h"
#include "irpmondll.h"
#include "request.h"
#include "driver-hook.h"
#include "driver-settings.h"
#include "request-output.h"
#include "stop-event.h"
#include "libvsock.h"
#include "irpmonc.h"



//	--input=[L|D|N]:<value>
//		D:\\.\IRPMon
//		N:localhost:1234
//		L:C:\binarylog.log
//		V:<CID>:<port>
//
//	--output=<T|J|B>:<filename|->
//		T = text lines
//		J = JSON lines, the array mode is not supported
//		B = output to IRPMon's binary log format
//		- = stdout
//
//	--hook-driver=[I][C][F][A][S][U][N][D][W][E][a]:<drivername>
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
//		s = capture stacktrace
//
//	--unhook-driver=[W]:<drivername>
//		W = name watch
//
//  --hook-device=<A|N>:<devicename|address>
//		A = address
//		N = name
//
//  --unhook-device=<A|N>:<devicename|address>
//		A = address
//		N = name
//
// --clear-on-disconnect={yes|no|true|false}
// --collect-disconnected={yes|no|true|false}
// --process-events={yes|no|true|false}
// --file-object-events={yes|no|true|false}
// --snapshot-events={yes|no|true|false}
// --process-emulate={yes|no|true|false}
// --snapshot-emulate={yes|no|true|false}
// --strip-threshold=<integer>
// --strip-data={yes|no|true|false}
//	--boot-log={yes|no|true|false}
// --save-settings={yes|no|true|false}
//
// --sym-path=<Path>
// --sym-file=<filename>
// --sym-dir=<dir>|[mask]
//
// --help
// --stop[=<PID>]
// --load=<ServiceName>
// --unload=<ServiceName>
//



static 	OPTION_RECORD opts[] = {
	{otInput, L"input", true, 0, 1},
	{otOutput, L"output", false, 0, INT_MAX},
	{otHookDriver, L"hook-driver", false, 0, INT_MAX},
	{otUnhookDriver, L"unhook-driver", false, 0, INT_MAX},
	{otHookDevice, L"hook-device", false, 0, INT_MAX},
	{otUnhookDevice, L"unhook-device", false, 0, INT_MAX},

	{otClearOnDisconnect, L"clear-on-disconnect", false, 0, 1},
	{otCollectDisconnected, L"collect-disconnected", false, 0, 1 },
	{otProcessEventsCollect, L"process-events", false, 0, 1 },
	{otFileObjectEventsCollect, L"file-object-events", false, 0, 1 },
	{otDriverSnapshotEventsCollect, L"snapshot-events", false, 0, 1 },
	{otProcessEmulateOnConnect, L"process-emulate", false, 0, 1 },
	{otDriverSnapshotOnConnect, L"snapshot-emulate", false, 0, 1 },
	{otDataStripThreshold, L"strip-threshold", false, 0, 1},
	{otStripData, L"strip-data", false, 0, 1},
	{otBootLog, L"boot-log", false, 0, 1},
	{otSettingsSave, L"save-settings", false, 0, 1},

	{otSymPath, L"sym-path", false, 0, 1},
	{otSymFile, L"sym-file", false, 0, INT_MAX},
	{otSymDirectory, L"sym-dir", false, 0, INT_MAX},

	{otHelp, L"help", false, 0, 1},
	{otStop, L"stop", false, 0, 1},
	{otLoad, L"load", false, 0, 1},
	{otUnload, L"unload", false, 0, 1},
};


static bool _help = false;
static bool _stop = false;
static DWORD _stopProcessId = 0;
static std::vector<CRequestOutput *> _outputs;
static wchar_t *_inLogFileName = NULL;
static IRPMON_INIT_INFO _initInfo;
static std::vector<CDriverHook *> _driversToHook;
static std::vector<CDriverNameWatch*> _nwsToRegister;
static std::vector<std::wstring> _nwsToUnregister;
static std::vector<CDeviceHook*> _devicesToHook;
static std::vector<void *> _devicesToUnhookAddrs;
static std::vector<std::wstring> _devicesToUnhookNames;
static std::map<std::wstring, CDriverHook *> _hookedDrivers;
static std::map<std::wstring, CDriverNameWatch *> _watchedDrivers;
static std::map<void *, CDeviceHook *> _hookedDevices;
static std::vector<std::wstring> _unhookDrivers;
static HANDLE _dpListHandle = nullptr;
static HANDLE _reqListHandle = nullptr;
static HANDLE _symStore = nullptr;
static std::wstring _loadServiceName;
static std::wstring _unloadServiceName;


static int _parse_input(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t *delimiter = NULL;

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
		case L'V': _initInfo.ConnectorType = ictVSockets; break;
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
			case ictVSockets: {
				wchar_t *endptr = NULL;
				unsigned int vmciVersion = 0xffffffff;
				unsigned int vmciAddress = 0xffffffff;

				vmciVersion = LibVSockGetVersion();
				if (vmciVersion == 0xffffffff) {
					ret = EINVAL;
					fprintf(stderr, "[ERROR]: VMWare vSockets not supported by this machine\n");
					goto Exit;
				}

				vmciAddress = LibVSockGetLocalId();
				if (vmciAddress == 0xffffffff) {
					ret = EINVAL;
					fprintf(stderr, "[ERROR]: Invalid local vSocket address\n");
					goto Exit;
				}

				fprintf(stderr, "[INFO]: vSockets version: %u.%u\n", vmciVersion & 0xffff, vmciVersion >> 16);
				fprintf(stderr, "[INFO]: vSocket address:  %u\n", vmciAddress);
				_initInfo.Data.VSockets.CID = wcstoul(Value, &endptr, 0);
				if (*endptr != L':') {
					ret = EINVAL;
					fprintf(stderr, "[ERROR]: Invalid vSocket address delimiter\n");
					goto Exit;
				}

				if (endptr == Value || (_initInfo.Data.VSockets.CID == ULONG_MAX && errno == ERANGE)) {
					ret = ERANGE;
					fprintf(stderr, "[ERROR]: Unable to parse vSocket CID\n");
					goto Exit;
				}

				Value = endptr + 1;
				_initInfo.Data.VSockets.Port = wcstoul(Value, &endptr, 0);
				if (endptr == Value || *endptr != L'\0' || (_initInfo.Data.VSockets.Port == ULONG_MAX && errno == ERANGE)) {
					ret = ERANGE;
					fprintf(stderr, "[ERROR]: Unable to parse vSocket port\n");
					goto Exit;
				}
			} break;
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
	ERequestLogFormat format;
	CRequestOutput *o = nullptr;

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
		case L'B': format = rlfBinary; break;
		case L'J': format = rlfJSONLines; break;
		case L'T': format = rlfText; break;
		default:
			ret = -10;
			fprintf(stderr, "[ERROR]: Invalid output modifier \"%lc\"\n", *Value);
			goto Exit;
			break;
	}

	Value = delimiter + 1;
	if (wcscmp(Value, L"-") == 0)
		Value = L"";

	o = new CRequestOutput(format, Value);
	_outputs.push_back(o);

Exit:
	return ret;
}


static int _parse_hookdriver(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t *delimiter = NULL;
	CDriverHook *dh = nullptr;
	CDriverNameWatch *dnw = nullptr;
	DRIVER_MONITOR_SETTINGS dms;
	bool nameWatch = false;
	bool allDevices = false;
	bool devExtHook = false;

	memset(&dms, 0, sizeof(dms));
	delimiter = wcschr(Value, L':');
	if (delimiter != NULL) {
		while (ret == 0 && Value != delimiter) {
			switch (*Value) {
				case L'I': dms.MonitorIRP = TRUE; break;
				case L'C': dms.MonitorIRPCompletion = TRUE; break;
				case L'F': dms.MonitorFastIo = TRUE;  break;
				case L'S': dms.MonitorStartIo = TRUE; break;
				case L'A': dms.MonitorAddDevice = TRUE; break;
				case L'U': dms.MonitorUnload = TRUE; break;
				case L'N': dms.MonitorNewDevices = TRUE; break;
				case L'D': dms.MonitorData = TRUE; break;
				case 's': dms.MonitorStackTrace = TRUE; break;
				case L'E': devExtHook = true; break;
				case L'W': nameWatch = true; break;
				case L'a': allDevices = true; break;
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
		for (size_t i = 0; i < sizeof(dms.IRPSettings) / sizeof(dms.IRPSettings[0]); ++i)
			dms.IRPSettings[i] = 1;
		
		for (size_t i = 0; i < sizeof(dms.FastIoSettings) / sizeof(dms.FastIoSettings[0]); ++i)
			dms.FastIoSettings[i] = 1;

		if (nameWatch) {
			dnw = new CDriverNameWatch(Value);
			dnw->SetInfo(dms);
			_nwsToRegister.push_back(dnw);
		} else {
			dh = new CDriverHook(Value);
			dh->SetInfo(dms);
			dh->setAllDevices(allDevices);
			dh->setDevExtHook(devExtHook);
			_driversToHook.push_back(dh);
		}
	}

	return ret;
}


static int _parse_hookdevice(const wchar_t *Value, bool Hook)
{
	int ret = 0;
	const wchar_t *delimiter = NULL;
	CDeviceHook *dh = nullptr;
	bool address = false;
	bool name = false;
	void *addr = nullptr;

	delimiter = wcschr(Value, L':');
	if (delimiter != NULL) {
		while (ret == 0 && Value != delimiter) {
			switch (*Value) {
				case L'A': address = true; break;
				case L'N': name = true; break;
			default:
				ret = -6;
				fprintf(stderr, "[ERROR]: Unknown device hooking modifier \"%lc\"\n", *Value);
				break;
			}

			++Value;
		}

		++Value;
	}

	if (ret == 0) {
		if (address) {
			addr = (void*)wcstoull(Value, nullptr, 0);
			if (Hook) {
				dh = new CDeviceHook(addr);
				_devicesToHook.push_back(dh);
			} else _devicesToUnhookAddrs.push_back(addr);
		} else if (name) {
			if (Hook)
				dh = new CDeviceHook(Value);
			else _devicesToUnhookNames.push_back(Value);
			_devicesToHook.push_back(dh);
		} else {
			ret = -8;
			fprintf(stderr, "[ERROR]: No device identifier type specified\n");
		}
	}

	return ret;
}


static int _parse_unhookdriver(const wchar_t *Value)
{
	int ret = 0;
	const wchar_t* delimiter = NULL;
	bool nameWatch = false;

	delimiter = wcschr(Value, L':');
	if (delimiter != NULL) {
		if (delimiter != Value + 1) {
			ret = -8;
			fprintf(stderr, "[ERROR]: Only one modifier can be present before the \":\" delimiter\n");
			goto Exit;
		}

		switch (*Value) {
			case L'W': nameWatch = true; break;
			default:
				ret = -6;
				fprintf(stderr, "[ERROR]: Unknown driver unhooking modifier \"%lc\"\n", *Value);
				break;
		}

		Value = delimiter + 1;
	}

	if (ret == 0) {
		if (nameWatch)
			_nwsToUnregister.push_back(std::wstring(Value));
		else _unhookDrivers.push_back(std::wstring(Value));
	}

Exit:
	return ret;
}


static int _parse_stop(const wchar_t *Value)
{
	int ret = 0;
	wchar_t *endPtr = nullptr;

	if (*Value != L'\0') {
		errno = 0;
		_stopProcessId = wcstoul(Value, &endPtr, 0);
		if (_stopProcessId == ULONG_MAX && errno == ERANGE)
			ret = errno;
		else if (endPtr == Value)
			ret = EINVAL;
	}

	_stop = (ret == 0);

	return ret;
}


static int _parse_load(const wchar_t *Value)
{
	int ret = 0;

	if (*Value != L'\0')
		_loadServiceName = std::wstring(Value);

	if (_loadServiceName.empty())
		_loadServiceName = L"irpmndrv";


	return ret;
}


static int _parse_unload(const wchar_t *Value)
{
	int ret = 0;

	if (*Value != L'\0')
		_unloadServiceName = std::wstring(Value);

	if (_unloadServiceName.empty())
		_unloadServiceName = L"irpmndrv";


	return ret;
}


static int _parse_sympath(EOptionType OpType, const wchar_t *Value)
{
	int ret = 0;

	switch (OpType) {
		case otSymPath: {
			ret = SymStoreSetSymPath(_symStore, Value);
			if (ret != 0)
				fprintf(stderr, "[ERROR]: Unable to set symbol path to \"%ls\": %u\n", Value, ret);
		} break;
		case otSymFile: {
			ret = SymStoreAddFile(_symStore, Value);
			if (ret != 0)
				fprintf(stderr, "[ERROR]: Unable to add file \"%ls\" to symbol store: %u\n", Value, ret);
		} break;
		case otSymDirectory: {
			ret = SymStoreAddDirectory(_symStore, Value, NULL);
			if (ret != 0)
				fprintf(stderr, "[ERROR]: Unable to add directory \"%ls\" to symbol store: %u\n", Value, ret);
		} break;
		default:
			break;
	}

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
	if (delimiter == NULL)
		delimiter = Arg + wcslen(Arg);

	name = Arg + 2;
	value = delimiter;
	if (*delimiter != L'\0') {
		value = delimiter + 1;
		*delimiter = L'\0';
	}

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
		ret = ENOENT;
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
			ret = _parse_hookdevice(value, true);
			break;
		case otUnhookDevice:
			ret = _parse_hookdevice(value, false);
			break;
		case otClearOnDisconnect:
		case otCollectDisconnected:
		case otProcessEventsCollect:
		case otFileObjectEventsCollect:
		case otDriverSnapshotEventsCollect:
		case otProcessEmulateOnConnect:
		case otDriverSnapshotOnConnect:
		case otDataStripThreshold:
		case otStripData:
		case otBootLog:
		case otSettingsSave:
			ret = parse_setting(opRec->Type, value);
			break;
		case otHelp:
			_help = true;
			break;
		case otStop:
			ret = _parse_stop(value);
			break;
		case otLoad:
			ret = _parse_load(value);
			break;
		case otUnload:
			ret = _parse_unload(value);
			break;
		case otSymPath:
		case otSymFile:
		case otSymDirectory:
			ret = _parse_sympath(opRec->Type, value);
			break;
	}

Exit:
	return ret;
}


static int _enum_hooked_objects(void)
{
	int ret = 0;
	ULONG infoCount = 0;
	PHOOKED_DRIVER_UMINFO info = nullptr;
	ULONG dnCount = 0;
	PDRIVER_NAME_WATCH_RECORD dnArray = nullptr;
	CDriverHook *dh = nullptr;
	CDeviceHook* deh = nullptr;
	CDriverNameWatch *dnw = nullptr;
	std::wstring n;

	ret = IRPMonDllDriverHooksEnumerate(&info, &infoCount);
	if (ret == 0) {
		const HOOKED_DRIVER_UMINFO* tmp = NULL;

		tmp = info;
		for (size_t i = 0; i < infoCount; ++i) {
			dh = new CDriverHook(*tmp);
			fprintf(stderr, "[INFO]: Driver \"%ls\" is already hooked\n", dh->Name().c_str());
			n = dh->Name();
			std::transform(n.begin(), n.end(), n.begin(), towlower);
			_hookedDrivers.insert(std::make_pair(n, dh));
			if (ret == 0) {
				const HOOKED_DEVICE_UMINFO *deviceInfo = nullptr;

				deviceInfo = info->HookedDevices;
				for (size_t j = 0; j < info->NumberOfHookedDevices; ++j) {
					deh = new CDeviceHook(*deviceInfo);
					fprintf(stderr, "[INFO]: Device \"%ls\" (0x%p) is already hooked\n", deh->Name().c_str(), deh->Address());
					_hookedDevices.insert(std::make_pair(deh->Address(), deh));
					if (ret != 0)
						break;

					++deviceInfo;
				}

				if (ret != 0) {
					_hookedDrivers.erase(dh->Name());
					delete dh;
				}
			}
			
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
				dnw = new CDriverNameWatch(*tmp);
				fprintf(stderr, "[INFO]: Already watching for driver \"%ls\"\n", dnw->DriverName().c_str());
				n = dnw->DriverName();
				std::transform(n.begin(), n.end(), n.begin(), towlower);
				_watchedDrivers.insert(std::make_pair(n, dnw));
				if (ret != 0)
					break;

				++tmp;
			}

			IRPMonDllDriverNameWatchEnumFree(dnArray, dnCount);
		}
	}

	return ret;
}


static DWORD _driver_action(bool Hook)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	for (auto& hdr : _driversToHook) {
		if (Hook)
			fprintf(stderr, "[INFO]: Hooking driver \"%ls\"...\n", hdr->Name().c_str());
		else fprintf(stderr, "[INFO]: Unhooking driver \"%ls\"...\n", hdr->Name().c_str());

		ret = (Hook) ? hdr->Hook() : hdr->Unhook();
		if (ret != 0) {
			if (Hook) {
				if (ret == ERROR_ALREADY_EXISTS) {
					ret = 0;
					fprintf(stderr, "[WARNING]: Driver \"%ls\" already hooked\n", hdr->Name().c_str());
				}

				if (ret != 0) {
					fprintf(stderr, "[ERROR]: Failed to hook driver \"%ls\": %u\n", hdr->Name().c_str(), ret);
					for (auto& tmp : _driversToHook) {
						if (wcsicmp(tmp->Name().c_str(), hdr->Name().c_str()) == 0)
							break;

						fprintf(stderr, "[INFO]: Unhooking driver \"%ls\"...\n", tmp->Name().c_str());
						tmp->Unhook();
					}

					break;
				}
			} else fprintf(stderr, "[WARNING]: Failed to unhook driver \"%ls\": %u\n", hdr->Name().c_str(), ret);
		}
	}

	if (ret == 0) {
		for (auto & dnw : _nwsToRegister) {
			if (Hook)
				fprintf(stderr, "[INFO]: Registering a name watch for \"%ls\"...\n", dnw->DriverName().c_str());
			
			ret = (Hook) ? dnw->Register() : dnw->Unregister();
			if (ret != 0) {
				if (ret == ERROR_ALREADY_EXISTS) {
					ret = 0;
					fprintf(stderr, "[WARNING]: Name watch for \"%ls\" already registered\n", dnw->DriverName().c_str());
				}

				if (ret != 0) {
					fprintf(stderr, "[ERROR]: Unable to register name watch for \"%ls\": %u\n", dnw->DriverName().c_str(), ret);
					break;
				}
			}
		}
	}

	if (ret == 0 && Hook) {
		for (auto& dnw : _nwsToUnregister) {
			fprintf(stderr, "[INFO]: Unregistering name watch for \"%ls\"...\n", dnw.c_str());
			ret = IRPMonDllDriverNameWatchUnregister(dnw.c_str());
			if (ret != 0)
				fprintf(stderr, "[WARNING]: Unable to unregister the name watch for \"%ls\": %u\n", dnw.c_str(), ret);

			ret = 0;
		}
	}

	if (ret == 0 && Hook) {
		HANDLE hookHandle = nullptr;
		ULONG driverCount = 0;
		PHOOKED_DRIVER_UMINFO driverInfo = nullptr;
		const HOOKED_DRIVER_UMINFO* tmp = nullptr;

		ret = IRPMonDllDriverHooksEnumerate(&driverInfo, &driverCount);
		if (ret == 0) {
			for (auto& duh : _unhookDrivers) {
				tmp = driverInfo;
				for (size_t i = 0; i < driverCount; ++i) {
					if (wcsicmp(tmp->DriverName, duh.c_str()) == 0) {
						fprintf(stderr, "[INFO]: Unhooking driver \"%ls\" (0x%p)...\n", tmp->DriverName, tmp->DriverObject);
						ret = IRPMonDllOpenHookedDriver(tmp->ObjectId, &hookHandle);
						if (ret == 0) {
							ret = IRPMonDllUnhookDriver(hookHandle);
							if (ret != 0)
								fprintf(stderr, "[WARNING]: Unable to unhook driver \"%ls\" (0x%p): %u\n", tmp->DriverName, tmp->DriverObject, ret);

							IRPMonDllCloseHookedDriverHandle(hookHandle);
						} else fprintf(stderr, "[WARNING]: Unable to get handle for hooked driver \"%ls\" (0x%p): %u\n", tmp->DriverName, tmp->DriverObject, ret);
					}

					++tmp;
					ret = 0;
				}
			}

			IRPMonDllDriverHooksFree(driverInfo, driverCount);
		} else fprintf(stderr, "[WARNING]: Unable to enumerate hooked drivers and devices: %u\n", ret);
	}

	return ret;
}


static DWORD _device_action(bool Hook)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	for (auto& hdr : _devicesToHook) {
		fprintf(stderr, "[INFO]: %ls device \"%ls\" (0x%p)...\n", (Hook) ? L"Hooking" : L"Unhooking", hdr->Name().c_str(), hdr->Address());
		ret = (Hook) ? hdr->Hook() : hdr->Unhook();
		if (ret != 0) {
			if (Hook) {
				fprintf(stderr, "[ERROR]: Failed to hook device \"%ls\" (0x%p): %u\n", hdr->Name().c_str(), hdr->Address(), ret);
				for (auto& tmp : _devicesToHook) {
					if (wcsicmp(tmp->Name().c_str(), hdr->Name().c_str()) == 0)
						break;

					fprintf(stderr, "[INFO]: Unhooking device \"%ls\" (0x%p)...\n", tmp->Name().c_str(), tmp->Address());
					tmp->Unhook();
				}

				break;
			} else fprintf(stderr, "[WARNING]: Failed to unhook device \"%ls\" (0x%p): %u\n", hdr->Name().c_str(), hdr->Address(), ret);
		}
	}

	if (ret == 0 && Hook) {
		HANDLE hookHandle = nullptr;
		ULONG driverCount = 0;
		PHOOKED_DRIVER_UMINFO driverInfo = nullptr;
		const HOOKED_DRIVER_UMINFO *tmp = nullptr;
		const HOOKED_DEVICE_UMINFO *devInfo = nullptr;

		ret = IRPMonDllDriverHooksEnumerate(&driverInfo, &driverCount);
		if (ret == 0) {
			for (auto & duh : _devicesToUnhookAddrs) {
				tmp = driverInfo;
				for (size_t i = 0; i < driverCount; ++i) {
					devInfo = tmp->HookedDevices;
					for (size_t j = 0; j < tmp->NumberOfHookedDevices; ++j) {
						if (devInfo->DeviceObject == duh) {
							fprintf(stderr, "[INFO]: Unhooking \"%ls\" (0x%p)...\n", devInfo->DeviceName, devInfo->DeviceObject);
							ret = IRPMonDllOpenHookedDevice(devInfo->ObjectId, &hookHandle);
							if (ret == 0) {
								ret = IRPMonDllUnhookDevice(hookHandle);
								if (ret != 0)
									fprintf(stderr, "[WARNING]: Unable to unhook \"%ls\" (0x%p): %u\n", devInfo->DeviceName, devInfo->DeviceObject, ret);

								IRPMonDllCloseHookedDeviceHandle(hookHandle);
							} else fprintf(stderr, "[WARNING]: Unable to get handle for hooked device \"%ls\" (0x%p): %u\n", devInfo->DeviceName, devInfo->DeviceObject, ret);
						}

						++devInfo;
						ret = 0;
					}

					++tmp;
					ret = 0;
				}
			}
	
			for (auto& duh : _devicesToUnhookNames) {
				tmp = driverInfo;
				for (size_t i = 0; i < driverCount; ++i) {
					devInfo = tmp->HookedDevices;
					for (size_t j = 0; j < tmp->NumberOfHookedDevices; ++j) {
						if (wcsicmp(devInfo->DeviceName, duh.c_str()) == 0) {
							fprintf(stderr, "[INFO]: Unhooking \"%ls\" (0x%p)...\n", devInfo->DeviceName, devInfo->DeviceObject);
							ret = IRPMonDllOpenHookedDevice(devInfo->ObjectId, &hookHandle);
							if (ret == 0) {
								ret = IRPMonDllUnhookDevice(hookHandle);
								if (ret != 0)
									fprintf(stderr, "[WARNING]: Unable to unhook \"%ls\" (0x%p): %u\n", devInfo->DeviceName, devInfo->DeviceObject, ret);

								IRPMonDllCloseHookedDeviceHandle(hookHandle);
							}
							else fprintf(stderr, "[WARNING]: Unable to get handle for hooked device \"%ls\" (0x%p): %u\n", devInfo->DeviceName, devInfo->DeviceObject, ret);
						}

						++devInfo;
					}

					++tmp;
				}
			}

			IRPMonDllDriverHooksFree(driverInfo, driverCount);
		} else fprintf(stderr, "[WARNING]: Unable to enumerate hooked drivers and devices: %u\n", ret);
	}

	return ret;
}


static void cdecl _on_request(PREQUEST_HEADER Request, HANDLE RequestHandle, void* Context, PBOOLEAN Store)
{
	int err = 0;

	*Store = FALSE;
	for (auto & o : _outputs) {
		err = RequestToStream(RequestHandle, o->Format(), _dpListHandle, _symStore, o->StreamHandle());
		if (err == 0) {
			if (o->Format() != rlfBinary)
				o->write("\n");
		} else fprintf(stderr, "[WARNING]: Unable to write requests 0x%p to stream: %u\n", Request, err);
	}

	return;
}


static int _prepare_output(void)
{
	int ret = 0;

	for (auto & o : _outputs) {
		ret = o->Prepare();
		if (ret != 0) {
			fprintf(stderr, "[ERROR]: Cannot initialize output \"%ls\": %u\n", o->FileName().c_str(), ret);
			break;
		}
	}

	if (ret == 0) {
		ret = ReqListSetCallback(_reqListHandle, _on_request, nullptr);
		if (ret != 0) {
			fprintf(stderr, "[ERROR]: Unable to register Request List Callback: %u\n", ret);
			for (auto& o : _outputs)
				delete o;

			_outputs.clear();
		}
	}

	return ret;
}


static void _free_output(void)
{
	ReqListUnregisterCallback(_reqListHandle);
	for (auto & o : _outputs)
		delete o;

	_outputs.clear();

	return;
}


static int _init_dlls(void)
{
	int ret = 0;

	ret = DPListModuleInit(L"dparser.dll");
	if (ret == 0) {
		ret = ReqListModuleInit(L"reqlist.dll");
		if (ret == 0) {
			ret = CallbackStreamModuleInit(L"callbackstream.dll");
			if (ret == 0) {
				ret = SymbolsModuleInit(L"symbols.dll");
				if (ret == 0) {
					ret = SymStoreCreate(NULL, &_symStore);
					if (ret != 0)
						fprintf(stderr, "[ERROR]: Unable to initialize the symbol store: %u\n", ret);

					if (ret != 0)
						SymbolsModuleFinit();
				} else fprintf(stderr, "[ERROR]: Unable to initialize symbols.dll: %u\n", ret);

				if (ret != 0)
					SymbolsModuleFinit();
			} else fprintf(stderr, "[ERROR]: Unable to initialize callbackstream.dll: %u\n", ret);

			if (ret != 0)
				ReqListModuleFinit();
		} else fprintf(stderr, "[ERROR]: Unable to initialize reqlist.dll: %u\n", ret);

		if (ret != 0)
			DPListModuleFinit();
	} else fprintf(stderr, "[ERROR]: Unable to initialize dparser.dll: %u\n", ret);

	return ret;
}


static void _finit_dlls(void)
{
	SymStoreFree(_symStore);
	SymbolsModuleFinit();
	CallbackStreamModuleFinit();
	ReqListModuleFinit();
	DPListModuleFinit();

	return;
}


static int _add_parsers(HANDLE ListHandle)
{
	int ret = 0;
	DWORD parserCount = 0;
	wchar_t appDir[MAX_PATH];
	wchar_t *tmp = nullptr;
	IRPMON_DATA_PARSER info;

	if (GetModuleFileNameW(nullptr, appDir, sizeof(appDir) / sizeof(appDir[0])) == 0) {
		ret = GetLastError();
		fprintf(stderr, "[ERROR]: Unable to get application file name: %u\n", ret);
	}

	if (ret == 0) {
		tmp = appDir + wcslen(appDir);
		while (tmp != appDir && *tmp != L'\\')
			--tmp;

		if (tmp != appDir) {
			*tmp = L'\0';
			ret = DPListAddDirectory(ListHandle, appDir);
			if (ret == 0) {
				parserCount = DPListGetCount(ListHandle);
				fprintf(stderr, "[INFO]: %u parsers loaded\n", parserCount);
				if (parserCount > 0) {
					for (ULONG i = 0; i < parserCount; ++i) {
						ret = DPListGetItemInfo(ListHandle, i, &info);
						if (ret != 0) {
							fprintf(stderr, "[ERROR]: Unable to get info for parser %u: %u\n", i, ret);
							break;
						}
					
						fprintf(stderr, "[INFO]:   %ls (version %u.%u.%u)\n", info.Name, info.MajorVersion, info.MinorVersion, info.BuildVersion);
						DPListItemInfoFree(&info);
					}

					fprintf(stderr, "[INFO]: \n");
				}
			}
		} else {
			ret = EINVAL;
			fprintf(stderr, "[ERROR]: Unable to find backlash in the application file name of \"%ls\"\n", appDir);
		}
	}

	return ret;
}


static int _service_action(bool Load)
{
	int ret = 0;
	DWORD access = 0;
	SC_HANDLE hScm = nullptr;
	SC_HANDLE hService = nullptr;
	const wchar_t* serviceName = nullptr;

	if ((Load && !_loadServiceName.empty()) ||
		(!Load && !_unloadServiceName.empty())) {
		hScm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
		if (hScm != nullptr) {
			access = (Load) ? SERVICE_START : SERVICE_STOP;
			serviceName = (Load) ? _loadServiceName.c_str() : _unloadServiceName.c_str();
			hService = OpenServiceW(hScm, serviceName, access);
			if (hService != nullptr) {
				if (Load) {
					if (!StartServiceW(hService, 0, nullptr)) {
						ret = GetLastError();
						if (ret == ERROR_SERVICE_ALREADY_RUNNING) {
							ret = 0;
							fprintf(stderr, "[WARNING]: The \"%ls\" service is already running, nothing to sart\n", serviceName);
						}

						if (ret != 0)
							fprintf(stderr, "[ERROR]: Unable to start the \"%ls\" service: %u\n", serviceName, ret);
					}
				} else {
					SERVICE_STATUS ss;
					
					if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss)) {
						ret = GetLastError();
						if (ret == ERROR_SERVICE_NOT_ACTIVE) {
							ret = 0;
							fprintf(stderr, "[WARNING]: The \"%ls\" service is not active, nothing to stop\n", serviceName);
						}

						if (ret != 0)
							fprintf(stderr, "[ERROR]: Unable to stop the \"%ls\" service: %u\n", serviceName, ret);
					}
				}

				CloseServiceHandle(hService);
			} else {
				ret = GetLastError();
				if (ret == ERROR_ACCESS_DENIED) {
					ret = 0;
					fprintf(stderr, "[WARNING]: Not enough privileges to access the \"%ls\" service, no action taken\n",serviceName);
				}

				if (ret != 0)
					fprintf(stderr, "[ERROR]: Unable to access driver service: %u\n", ret);
			}

			CloseServiceHandle(hScm);
		} else {
			ret = GetLastError();
			fprintf(stderr, "[ERROR]: Unable to open SCM: %u\n", ret);
		}
	}

	return ret;
}

int wmain(int argc, wchar_t *argv[])
{
	int ret = 0;
	POPTION_RECORD opRec = nullptr;

	ret = _init_dlls();
	if (ret == 0) {
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

			if (ret == 0)
				ret = _service_action(true);
		}

		if (ret == 0) {
			if (_help) {
			} else if (_stop) {
				if (_stopProcessId != 0)
					fprintf(stderr, "[INFO]: Sending a stop signal to irpmonc process with PID %u...\n", _stopProcessId);
				else fprintf(stderr, "[INFO]: Sending a stop signal to all irpmonc processes...\n");

				ret = stop_event_oepn(_stopProcessId);
				if (ret == 0) {
					stop_event_set();
					stop_event_finit();
					fprintf(stderr, "[INFO]: Done\n");
				} else fprintf(stderr, "[ERROR]: Unable to access the stop event object: %u\n", ret);
		
				return ret;
			}
		
			ret = DPListCreate(&_dpListHandle);
			if (ret == 0) {
				ret = _add_parsers(_dpListHandle);
				if (ret == 0)
					ret = ReqListCreate(&_reqListHandle);
				
				if (ret == 0) {
					ReqListAssignParserList(_reqListHandle, _dpListHandle);
					ReqListSetSymStore(_reqListHandle, _symStore);
					ret = IRPMonDllInitialize(&_initInfo);
				}

				if (ret == 0) {
					if (_initInfo.ConnectorType != ictNone) {
						ret = stop_event_create();
						if (ret == 0) {
							ret = sync_settings();
							if (ret == 0) {
								print_settings();
								ret = _enum_hooked_objects();
								if (ret == 0) {
									ret = _driver_action(true);
									if (ret == 0) {
										ret = _device_action(true);
										if (ret != 0)
											_device_action(false);
									}
								} else fprintf(stderr, "[ERROR]: Unable to enumerate hooked objects: %u\n", ret);
							} else fprintf(stderr, "[ERROR]: Unable to sync driver settings: %u\n", ret);
						
							if (ret != 0)
								stop_event_finit();
						} else {
							ret = GetLastError();
							fprintf(stderr, "[ERROR]: Unable to create the global stopping event: %u\n", ret);
						}
					}

					if (ret == 0 && _outputs.size() > 0) {
						ret = _prepare_output();
						if (ret == 0) {
							switch (_initInfo.ConnectorType) {
								case ictDevice:
								case ictNetwork:
								case ictVSockets: {
									fprintf(stderr, "[INFO]: Connecting to the driver...\n");
									ret = IRPMonDllConnect();
									if (ret == 0) {
										DWORD requestSize = 0x1000;
										PREQUEST_HEADER request = nullptr;

										request = (PREQUEST_HEADER)calloc(1, requestSize);
										if (request != NULL) {
											do {
												ret = IRPMonDllGetRequest(request, requestSize);
												switch (ret) {
													case 0: {
														ret = ReqListAdd(_reqListHandle, request);
														if (ret == 0) {
														} else fprintf(stderr, "[ERROR]: Unable to add the request to the list: %u\n", ret);
													} break;
													case ERROR_INSUFFICIENT_BUFFER: {
														PREQUEST_HEADER tmp = nullptr;

														requestSize *= 2;
														tmp = (PREQUEST_HEADER)realloc(request, requestSize);
														if (tmp != nullptr) {
															fprintf(stderr, "[INFO]: Request size extended to %u bytes\n", requestSize);
															request = tmp;
															ret = 0;
														} else {
															ret = ERROR_NOT_ENOUGH_MEMORY;
															fprintf(stderr, "[ERROR]: Unable to extend request size to %u bytes\n", requestSize);
														}
													} break;
													case ERROR_NO_MORE_ITEMS:
													case ERROR_NO_MORE_FILES:
														ret = 0;
														if (stop_event_wait(1000) == 0)
															ret = ERROR_NO_MORE_ITEMS;
														break;
													default:
														fprintf(stderr, "[ERROR]: Call to IRPMonDllGetRequest failed with error %u\n", ret);
														break;
												}
											} while (ret == ERROR_SUCCESS);
									
											if (ret == ERROR_NO_MORE_ITEMS)
												ret = ERROR_SUCCESS;

											free(request);
										} else fprintf(stderr, "[ERROR]: Unable to allocate memory to hold requests: %u\n", ret);

										fprintf(stderr, "[INFO]: Disconnecting the driver...\n");
										IRPMonDllDisconnect();
									} else fprintf(stderr, "[ERROR]: Unable to connect to the event queue: %u\n", ret);
								} break;
								case ictNone: {
									ret = ReqListLoad(_reqListHandle, _inLogFileName);
									if (ret == 0) {

									} else fprintf(stderr, "[ERROR]: Unable to load events from file \"%ls\": %u\n", _inLogFileName, ret);
								} break;
								default:
									ret = ERROR_NOT_SUPPORTED;
									fprintf(stderr, "[ERROR]: Unknown connection type of %u\n", _initInfo.ConnectorType);
									break;
							}
						
							if (ret != 0)
								_free_output();
						}

						if (_initInfo.ConnectorType != ictNone) {
							if (ret != 0) {
								_device_action(false);
								_driver_action(false);
							}
							
							stop_event_finit();
						}
					}

					IRPMonDllFinalize();
				} else fprintf(stderr, "[ERROR]: Unable to initialize IRPMon library: %u\n", ret);
				
				if (_reqListHandle != NULL)
					ReqListFree(_reqListHandle);

				DPListFree(_dpListHandle);
			} else fprintf(stderr, "[ERROR]: Unable to initialize parser list: %u\n", ret);
	
			_service_action(false);
		}

		_finit_dlls();
	}  else fprintf(stderr, "[ERROR]: Unable to initialize DLLs: %u\n", ret);

	return ret;
}
