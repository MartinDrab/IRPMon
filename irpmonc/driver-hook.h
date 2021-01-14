
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
				ret = IRPMonDllDriverStartMonitoring(Handle_);
				MonitoringEnabled_ = (ret == 0);
				if (ret != 0) {
					IRPMonDllUnhookDriver(Handle_);
					Handle_ = nullptr;
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
