
#ifndef __DRIVER_HOOK_H__
#define __DRIVER_HOOK_H__


#include <string>
#include <cstring>
#include "general-types.h"
#include "irpmondll-types.h"
#include "irpmondll.h"



class CDriverHook {
public:
	CDriverHook(const HOOKED_DRIVER_UMINFO & Info)
		: Settings_(Info.MonitorSettings),
		DriverName_(Info.DriverName, Info.DriverName + Info.DriverNameLen),
		DevExtHook_(Info.DeviceExtensionHooks),
		AllDevices_(false),
		MonitoringEnabled_(Info.MonitoringEnabled),
		Handle_(nullptr),
		ObjectId_(Info.ObjectId)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		ret = IRPMonDllOpenHookedDriver(ObjectId_, &Handle_);
		// TODO: Throw an exception
	}
	CDriverHook (const std::wstring & Name)
		: DriverName_(Name),
		AllDevices_(false),
		DevExtHook_(false),
		ObjectId_(nullptr),
		Handle_(nullptr),
		MonitoringEnabled_(false)
	{ 
		memset(&Settings_, 0, sizeof(Settings_));
		Settings_.MonitorIRP = TRUE;
		Settings_.MonitorIRPCompletion = TRUE;
		Settings_.MonitorFastIo = TRUE;
		Settings_.MonitorData = TRUE;
		for (size_t i = 0; i < sizeof(Settings_.IRPSettings) / sizeof(Settings_.IRPSettings[0]); ++i)
			Settings_.IRPSettings[i] = TRUE;

		for (size_t i = 0; i < sizeof(Settings_.FastIoSettings) / sizeof(Settings_.FastIoSettings[0]); ++i)
			Settings_.FastIoSettings[i] = TRUE;

		return;
	}
	~CDriverHook(void)
	{
		if (Handle_ != nullptr) {
			IRPMonDllCloseHookedDriverHandle(Handle_);
			Handle_ = nullptr;
		}

		return;
	}
	DWORD Hook(void)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		if (Handle_ == nullptr) {
			ret = IRPMonDllHookDriver(DriverName_.c_str(), &Settings_, DevExtHook_, &Handle_, &ObjectId_);
			if (ret == 0) {
				if (AllDevices_) {
					void* obješctId = nullptr;
					HANDLE hookHandle = nullptr;
					ULONG driverCount = 0;
					PIRPMON_DRIVER_INFO *driverInfo = nullptr;
					const IRPMON_DRIVER_INFO *tmpDriver = nullptr;
					const IRPMON_DEVICE_INFO* tmpDevice = nullptr;

					ret = IRPMonDllSnapshotRetrieve(&driverInfo, &driverCount);
					if (ret == 0) {
						for (ULONG i = 0; i < driverCount; ++i) {
							tmpDriver = driverInfo[i];
							if (wcsicmp(DriverName_.c_str(), tmpDriver->DriverName) == 0) {
								for (size_t j = 0; j < tmpDriver->DeviceCount; ++j) {
									tmpDevice = tmpDriver->Devices[j];
									ret = IRPMonDllHookDeviceByAddress(tmpDevice->DeviceObject, &hookHandle, &obješctId);
									if (ret == 0)
										IRPMonDllCloseHookedDeviceHandle(hookHandle);

									ret = 0;
								}

								break;
							}
						}

						IRPMonDllSnapshotFree(driverInfo, driverCount);
					}
				}

				if (ret == 0) {
					ret = IRPMonDllDriverStartMonitoring(Handle_);
					MonitoringEnabled_ = (ret == 0);
				}

				if (ret != 0) {
					IRPMonDllUnhookDriver(Handle_);
					Handle_ = nullptr;
					ObjectId_ = nullptr;
				}
			}
		} else ret = ERROR_ALREADY_EXISTS;

		return ret;
	}
	DWORD Unhook(void)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		ret = 0;
		if (Handle_ != nullptr) {
			if (MonitoringEnabled_)
				ret = IRPMonDllDriverStopMonitoring(Handle_);
			
			if (ret == 0) {
				ret = IRPMonDllUnhookDriver(Handle_);
				if (ret == 0) {
					Handle_ = nullptr;
					ObjectId_ = nullptr;
					MonitoringEnabled_ = false;
				}

				if (ret != 0)
					IRPMonDllDriverStartMonitoring(Handle_);
			}
		} else ret = ERROR_NOT_READY;

		return ret;
	}
	DWORD SetInfo(const DRIVER_MONITOR_SETTINGS &Settings)
	{
		DWORD ret = ERROR_GEN_FAILURE;
		bool me = false;

		ret = 0;
		if (Handle_ != nullptr) {
			me = MonitoringEnabled_;
			if (me)
				ret = IRPMonDllDriverStopMonitoring(Handle_);

			if (ret == 0) {
			ret = IRPMonDllDriverSetInfo(Handle_, &Settings);
			if (ret == 0)
				Settings_ = Settings;

				if (me)
					ret = IRPMonDllDriverStartMonitoring(Handle_);
			}
		} else {
			Settings_ = Settings;
			ret = 0;
		}

		return ret;
	}
	DRIVER_MONITOR_SETTINGS Settings(void) const { return Settings_; }
	bool Hooked(void) const { return (Handle_ != nullptr); }
	std::wstring Name(void) const { return DriverName_; }
	bool DevExtHook(void) const { return DevExtHook_; }
	bool setDevExtHook(bool Value) 
	{
		if (Hooked())
			return false;
		
		DevExtHook_ = Value;
		
		return true;
	}
	bool MonitoringEnabled(void) const { return MonitoringEnabled_; }
	bool AllDevices(void) const { return AllDevices_; }
	bool setAllDevices(bool Value)
	{
		bool ret = true;

		if (Hooked())
			return false;

		AllDevices_ = Value;

		return true;
	}
private:
	std::wstring DriverName_;
	DRIVER_MONITOR_SETTINGS Settings_;
	bool DevExtHook_;
	bool AllDevices_;
	bool MonitoringEnabled_;
	HANDLE Handle_;
	void *ObjectId_;
};


class CDeviceHook {
public:
	CDeviceHook(const HOOKED_DEVICE_UMINFO& Info)
		: ObjectId_(Info.ObjectId),
		DeviceAddress_(Info.DeviceObject),
		Handle_(nullptr),
		MonitoringEnabled_(false),
		DeviceName_(Info.DeviceName, Info.DeviceName + Info.DeviceNameLen)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		ret = IRPMonDllOpenHookedDevice(ObjectId_, &Handle_);
		if (ret != 0) {
			// TODO: Throw an exception
		}
	}
	CDeviceHook(const std::wstring& Name)
		: DeviceName_(Name),
		Handle_(nullptr),
		ObjectId_(nullptr),
		MonitoringEnabled_(false),
		DeviceAddress_(nullptr)
	{ }
	CDeviceHook(void *Address)
		: Handle_(nullptr),
		ObjectId_(nullptr),
		MonitoringEnabled_(false),
		DeviceAddress_(Address)
	{ }
	~CDeviceHook(void)
	{
		if (Hooked())
			IRPMonDllCloseHookedDeviceHandle(Handle_);

		return;
	}
	bool Hooked(void) const { return (Handle_ != nullptr); }
	DWORD Hook(void)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		if (!Hooked()) {
			if (DeviceAddress_ != nullptr)
				ret = IRPMonDllHookDeviceByAddress(DeviceAddress_, &Handle_, &ObjectId_);
			else ret = IRPMonDllHookDeviceByName(DeviceName_.c_str(), &Handle_, &ObjectId_);
		
			if (ret == 0) {
				ret = IRPMonDllHookedDeviceSetInfo(Handle_, nullptr, nullptr, TRUE);
				MonitoringEnabled_ = (ret == 0);
				if (ret != 0) {
					IRPMonDllUnhookDevice(Handle_);
					Handle_ = nullptr;
					ObjectId_ = nullptr;
				}
			}
		} else ret = ERROR_ALREADY_EXISTS;

		return ret;
	}
	DWORD Unhook(void)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		if (Hooked()) {
			IRPMonDllUnhookDevice(Handle_);
			Handle_ = nullptr;
			ObjectId_ = nullptr;
		} else ret = ERROR_NOT_READY;

		return ret;
	}
	std::wstring Name(void) const { return DeviceName_; }
	void* Address(void) const { return DeviceAddress_; }
private:
	std::wstring DeviceName_;
	void *DeviceAddress_;
	bool MonitoringEnabled_;
	void *ObjectId_;
	HANDLE Handle_;
};


class CDriverNameWatch {
public:
	CDriverNameWatch(const DRIVER_NAME_WATCH_RECORD & Record)
		: DriverName_(Record.DriverName),
		Settings_(Record.MonitorSettings),
		Active_(true)
	{ }
	CDriverNameWatch(const std::wstring &Name)
		: DriverName_(Name),
		Active_(false)
	{
		memset(&Settings_, 0, sizeof(Settings_));
		Settings_.MonitorIRP = TRUE;
		Settings_.MonitorIRPCompletion = TRUE;
		Settings_.MonitorFastIo = TRUE;
		Settings_.MonitorData = TRUE;
		for (size_t i = 0; i < sizeof(Settings_.IRPSettings) / sizeof(Settings_.IRPSettings[0]); ++i)
			Settings_.IRPSettings[i] = TRUE;

		for (size_t i = 0; i < sizeof(Settings_.FastIoSettings) / sizeof(Settings_.FastIoSettings[0]); ++i)
			Settings_.FastIoSettings[i] = TRUE;

		return;
	}
	~CDriverNameWatch(void)
	{
		Unregister();

		return;
	}
	DWORD Unregister(void)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		if (Active_) {
			ret = IRPMonDllDriverNameWatchUnregister(DriverName_.c_str());
			Active_ = (ret != 0);
		} else ret = ERROR_NOT_READY;

		return ret;
	}
	DWORD Register(void)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		if (!Active_) {
			ret = IRPMonDllDriverNameWatchRegister(DriverName_.c_str(), &Settings_);
			Active_ = (ret == 0);
		} else ret = ERROR_NOT_READY;

		return ret;
	}
	DRIVER_MONITOR_SETTINGS Settings(void) const { return Settings_; }
	bool Active(void) const { return Active_; }
	std::wstring DriverName(void) const { return DriverName_; }
	DWORD SetInfo(const DRIVER_MONITOR_SETTINGS & Value)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		if (!Active_) {
			Settings_ = Value;
			ret = 0;
		} else ret = ERROR_NOT_SUPPORTED;

		return ret;
	}
private:
	bool Active_;
	std::wstring DriverName_;
	DRIVER_MONITOR_SETTINGS Settings_;
};



#endif
