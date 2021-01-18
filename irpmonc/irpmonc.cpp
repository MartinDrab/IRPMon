
#include <string>
#include <algorithm>
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
#include "callback-stream.h"
#include "irpmondll.h"
#include "request.h"
#include "driver-hook.h"
#include "driver-settings.h"
#include "irpmonc.h"



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
//  --hook-device=<A|N>:<devicename>
//		A = address
//		N = name
//
//  --unhook-device=<A|N>:<devicename>
//		A = address
//		N = name
//
//	--boot-log={no|yes}



static 	OPTION_RECORD opts[] = {
	{otInput, L"input", true, 0, 1},
	{otOutput, L"output", false, 0, 1},
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
};


static bool _outputPresent = false;
static wchar_t *_inLogFileName = NULL;
static wchar_t *_outLogFileName = NULL;
static ERequestLogFormat _outLogFormat = rlfBinary;
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
static HANDLE _streamHandle = nullptr;


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
	if (wcscmp(Value, L"-") != 0) {
		_outLogFileName = wcsdup(Value);
		if (_outLogFileName == nullptr) {
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
			_outputPresent = (ret == 0);
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
		fprintf(stderr, "[INFO]: Hooking driver \"%ls\"...\n", hdr->Name().c_str());
		ret = (Hook) ? hdr->Hook() : hdr->Unhook();
		if (ret != 0) {
			if (Hook) {
				fprintf(stderr, "[ERROR]: Failed to hook driver \"%ls\": %u\n", hdr->Name().c_str(), ret);
				for (auto& tmp : _driversToHook) {
					if (wcsicmp(tmp->Name().c_str(), hdr->Name().c_str()) == 0)
						break;

					fprintf(stderr, "[INFO]: Unhooking driver \"%ls\"...\n", tmp->Name().c_str());
					tmp->Unhook();
				}

				break;
			} else fprintf(stderr, "[WARNING]: Failed to unhook driver \"%ls\": %u\n", hdr->Name().c_str(), ret);
		}
	}

	return ret;
}


static DWORD _device_action(bool Hook)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	for (auto& hdr : _devicesToHook) {
		fprintf(stderr, "[INFO]: Hooking device \"%ls\" (0x%p)...\n", hdr->Name().c_str(), hdr->Address());
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

	return ret;
}


static DWORD cdecl _on_stream_write(const void* Buffer, ULONG Length, void* Stream, void* Context)
{
	DWORD ret = 0;
	FILE* f = nullptr;

	f = (FILE*)Context;
	if (fwrite(Buffer, Length, 1, f) == 1)
		ret = Length;

	return ret;
}


static void cdecl _on_request(PREQUEST_HEADER Request, HANDLE RequestHandle, void* Context, PBOOLEAN Store)
{
	int err = 0;
	FILE* f = nullptr;

	*Store = FALSE;
	f = (FILE*)Context;
	err = RequestToStream(RequestHandle, _outLogFormat, _dpListHandle, _streamHandle);
	if (err == 0) {
		if (_outLogFormat != rlfBinary)
			fputs("\n", f);
	}
	else fprintf(stderr, "[WARNING]: Unable to write requests 0x%p to stream: %u\n", Request, err);

	return;
}


static DWORD _prepare_output(FILE **File)
{
	FILE *tmp = nullptr;
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	if (_outLogFileName != nullptr) {
		const wchar_t* fileMode = L"wb";

		if (_outLogFormat != rlfBinary)
			fileMode = L"w";

		tmp = _wfopen(_outLogFileName, fileMode);
		if (tmp == nullptr) {
			ret = errno;
			fprintf(stderr, "[ERROR]: Unable to access \"%ls\": %u\n", _outLogFileName, ret);
		}
	} else {
		tmp = stdout;
		if (_outLogFormat == rlfBinary) {
#ifdef _WIN32
			int fileDesc = 0;

			fileDesc = fileno(tmp);
			if (fileDesc >= 0) {
				if (setmode(fileDesc, O_BINARY) < 0) {
					ret = errno;
					fprintf(stderr, "[ERROR]: Unable to set binary mode for the standard output: %u\n", ret);
				}
			} else {
				ret = errno;
				fprintf(stderr, "[ERROR]: Unable to get file descriptor for the standard output: %u\n", ret);
			}
#endif
		}
	}

	if (ret == 0) {
		_streamHandle = CallbackStreamCreate(nullptr, _on_stream_write, nullptr, tmp);
		if (_streamHandle == nullptr)
			ret = ERROR_GEN_FAILURE;

		if (ret == 0) {
			ret = ReqListSetCallback(_reqListHandle, _on_request, tmp);
			if (ret == 0) {
				if (_outLogFormat == rlfBinary) {
					BINARY_LOG_HEADER hdr;

					memset(&hdr, 0, sizeof(hdr));
					hdr.Signature = LOGHEADER_SIGNATURE;
					hdr.Version = LOGHEADER_VERSION;
					hdr.Architecture = LOGHEADER_ARCHITECTURE;
					if (fwrite(&hdr, sizeof(hdr), 1, tmp) != 1)
						ret = errno;
				}

				if (ret == 0)
					*File = tmp;
			}

			if (ret != 0)
				CallbackStreamFree(_streamHandle);
		}

		if (ret != 0 && tmp != stdout)
			fclose(tmp);
	}

	return ret;
}


static void _free_output(FILE *File)
{
	ReqListUnregisterCallback(_reqListHandle);
	CallbackStreamFree(_streamHandle);
	if (_outLogFileName != nullptr)
		fclose(File);

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
	CallbackStreamModuleFinit();
	ReqListModuleFinit();
	DPListModuleFinit();

	return;
}


int wmain(int argc, wchar_t *argv[])
{
	int ret = 0;
	FILE * outputFile = nullptr;
	POPTION_RECORD opRec = nullptr;

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
		ret = _init_dlls();
		if (ret == 0) {
			ret = DPListCreate(&_dpListHandle);
			if (ret == 0) {
				ret = ReqListCreate(&_reqListHandle);
				if (ret == 0) {
					ReqListAssignParserList(_reqListHandle, _dpListHandle);
					ret = IRPMonDllInitialize(&_initInfo);
				}

				if (ret == 0) {
					if (_initInfo.ConnectorType != ictNone) {
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
					}

					if (ret == 0 && _outputPresent) {
						ret = _prepare_output(&outputFile);
						if (ret == 0) {
							switch (_initInfo.ConnectorType) {
								case ictDevice:
								case ictNetwork: {
									ret = IRPMonDllConnect();
									if (ret == 0) {
										DWORD requestSize = 0x1000;
										PREQUEST_HEADER request = nullptr;

										request = (PREQUEST_HEADER)calloc(1, requestSize);
										if (request != NULL) {
											do {
												ret = IRPMonDllGetRequest(request, RequestSize);
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
														if (tmp != NULL) {
															fprintf(stderr, "[INFO]: Request size extended to %u bytes\n", requestSize);
															request = tmp;
														} else {
															ret = ERROR_NOT_ENOUGH_MEMORY;
															fprintf(stderr, "[ERROR]: Unable to extend request size to %u bytes\n", requestSize);
														}
													} break;
												}
											} while (ret == ERROR_SUCCESS);
									
											free(request);
										} else fprintf(stderr, "[ERROR]: Unable to allocate memory to hold requests: %u\n", ret);

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
								_free_output(outputFile);
						}

						if (ret != 0 && _initInfo.ConnectorType != ictNone) {
							_free_output(outputFile);
							_device_action(false);
							_driver_action(false);
						}
					}

					IRPMonDllFinalize();
				} else fprintf(stderr, "[ERROR]: Unable to initialize IRPMon library: %u\n", ret);
				
				if (_reqListHandle != NULL)
					ReqListFree(_reqListHandle);

				DPListFree(_dpListHandle);
			} else fprintf(stderr, "[ERROR]: Unable to initialize parser list: %u\n", ret);

			_finit_dlls();
		} else fprintf(stderr, "[ERROR]: Unable to initialize DLLs: %u\n", ret);
	}

	return ret;
}
