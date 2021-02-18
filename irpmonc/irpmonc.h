
#ifndef __IRPMONC_H__
#define __IRPMONC_H__



typedef enum _EOptionType {
	otInput,
	otOutput,
	otHookDriver,
	otUnhookDriver,
	otHookDevice,
	otUnhookDevice,

	otClearOnDisconnect,
	otCollectDisconnected,
	otProcessEventsCollect,
	otFileObjectEventsCollect,
	otDriverSnapshotEventsCollect,
	otProcessEmulateOnConnect,
	otDriverSnapshotOnConnect,
	otDataStripThreshold,
	otStripData,
	otBootLog,
	otSettingsSave,
	
	otSymPath,
	otSymFile,
	otSymDirectory,

	otHelp,
	otStop,
	otLoad,
	otUnload,
} EOptionType, *PEOptionType;

typedef struct _OPTION_RECORD {
	EOptionType Type;
	const wchar_t *Name;
	bool Required;
	int Count;
	int MaxCount;
} OPTION_RECORD, *POPTION_RECORD;





#endif
