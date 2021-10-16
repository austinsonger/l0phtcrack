#ifndef __INC_CLC7SYSTEMMONITOR_H
#define __INC_CLC7SYSTEMMONITOR_H

struct IWbemLocator;
struct IWbemServices;

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64 || PLATFORM==PLATFORM_LINUX
#include <adl_sdk.h>
#endif

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
#include<pdh.h>
#include<pdhmsg.h>
#include<PowrProf.h>

class CLC7SystemMonitor;

class CWMIThread :public QThread
{
	Q_OBJECT;
public:
	CLC7SystemMonitor * m_monitor;
	CWMIThread(CLC7SystemMonitor *monitor);
public slots:
	void slot_wmiQuery(QString query, QList<QMap<QString, QVariant>> *results, QString *error, bool *ret);
};
#endif

class CLC7SystemMonitor :public QObject, public ILC7SystemMonitor
{
	Q_OBJECT;
	friend class CWMIThread;

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
private:

	CLC7Controller *m_ctrl;
	DWORD m_dwNumberOfProcessors;
	
	// WMI
	bool m_bInitWMI;
	bool m_bInitWMIFailed;	
	IWbemLocator *m_pLoc;
	IWbemServices *m_pSvc;
	CWMIThread m_wmithread;
	void InitializeWMI(void);
	bool WMIQuery(QString query, QList<QMap<QString, QVariant>> & results, QString & error);
	void TerminateWMI(void);

	// PDH
	bool m_bInitPDH;
	void InitializePDH(void);
	void TerminatePDH(void);
	QString m_processor, m_pct_processor_time;
	QList<wchar_t *> m_processer_counter_names;
	PDH_HQUERY m_pdhquery;
	QList<PDH_HCOUNTER> m_pdhcounters;
	QVector<int> m_last_utilizations;
	bool GetLocalPerformanceCounterName(quint32 index, QString & name);

	QMutex m_mutex;

signals:
	void sig_wmiQuery(QString query, QList<QMap<QString, QVariant>> *results, QString *error, bool *ret);

public slots:
	void slot_wmiThread_started();
	void slot_wmiThread_finished();

#endif

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
public:
	typedef int (*TYPEOF_ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
	typedef int (*TYPEOF_ADL_MAIN_CONTROL_DESTROY)();
	typedef int (*TYPEOF_ADL_ADAPTER_NUMBEROFADAPTERS_GET) (int*);
	typedef int (*TYPEOF_ADL_ADAPTER_ADAPTERINFO_GET) (LPAdapterInfo, int);
	typedef int (*TYPEOF_ADL_ADAPTER_ACTIVE_GET) (int, int*);
	typedef int (*TYPEOF_ADL_ADAPTER_ID_GET) (int iAdapterIndex, int *lpAdapterID);
	typedef int (*TYPEOF_ADL_OVERDRIVE_CAPS) (int iAdapterIndex, int *iSupported, int *iEnabled, int *iVersion);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_THERMALDEVICES_ENUM) (int iAdapterIndex, int iThermalControllerIndex, ADLThermalControllerInfo *lpThermalControllerInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_ODPARAMETERS_GET) (int  iAdapterIndex, ADLODParameters *  lpOdParameters);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_TEMPERATURE_GET) (int iAdapterIndex, int iThermalControllerIndex, ADLTemperature *lpTemperature);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_FANSPEED_GET) (int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue *lpFanSpeedValue);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_FANSPEEDINFO_GET) (int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedInfo *lpFanSpeedInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET) (int iAdapterIndex, int iDefault, ADLODPerformanceLevels *lpOdPerformanceLevels);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_ODPARAMETERS_GET) (int iAdapterIndex, ADLODParameters *lpOdParameters);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_CURRENTACTIVITY_GET) (int iAdapterIndex, ADLPMActivity *lpActivity);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_FANSPEED_SET)(int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue *lpFanSpeedValue);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET) (int iAdapterIndex, ADLODPerformanceLevels *lpOdPerformanceLevels);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_CAPS)(int iAdapterIndex, int *lpSupported);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_POWERCONTROLINFO_GET)(int iAdapterIndex, ADLPowerControlInfo *lpPowerControlInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_GET)(int iAdapterIndex, int *lpCurrentValue, int *lpDefaultValue);
	typedef int (*TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_SET)(int iAdapterIndex, int iValue);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_FANSPEED_GET)(int iAdapterIndex, ADLOD6FanSpeedInfo *lpFanSpeedInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS)(int iAdapterIndex, ADLOD6ThermalControllerCaps *lpThermalControllerCaps);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_TEMPERATURE_GET)(int iAdapterIndex, int *lpTemperature);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_CAPABILITIES_GET) (int iAdapterIndex, ADLOD6Capabilities *lpODCapabilities);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_STATEINFO_GET)(int iAdapterIndex, int iStateType, ADLOD6StateInfo *lpStateInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_CURRENTSTATUS_GET)(int iAdapterIndex, ADLOD6CurrentStatus *lpCurrentStatus);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_CAPS) (int iAdapterIndex, int *lpSupported);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_POWERCONTROLINFO_GET)(int iAdapterIndex, ADLOD6PowerControlInfo *lpPowerControlInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_GET)(int iAdapterIndex, int *lpCurrentValue, int *lpDefaultValue);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_FANSPEED_SET)(int iAdapterIndex, ADLOD6FanSpeedValue *lpFanSpeedValue);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_STATE_SET)(int iAdapterIndex, int iStateType, ADLOD6StateInfo *lpStateInfo);
	typedef int (*TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_SET)(int iAdapterIndex, int iValue);

	//typedef int(*TYPEOF_ADL_ADAPTERX2_CAPS) (int, int*);
	typedef int(*TYPEOF_ADL2_ADAPTER_ACTIVE_GET) (ADL_CONTEXT_HANDLE, int, int*);
	typedef int(*TYPEOF_ADL2_OVERDRIVE_CAPS) (ADL_CONTEXT_HANDLE context, int iAdapterIndex, int * iSupported, int * iEnabled, int * iVersion);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_CAPABILITIES_GET)	(ADL_CONTEXT_HANDLE, int, ADLODNCapabilities*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_SYSTEMCLOCKS_GET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevels*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_SYSTEMCLOCKS_SET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevels*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_MEMORYCLOCKS_GET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevels*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_MEMORYCLOCKS_SET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevels*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET) (ADL_CONTEXT_HANDLE, int, ADLODNPerformanceStatus*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_FANCONTROL_GET) (ADL_CONTEXT_HANDLE, int, ADLODNFanControl*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_FANCONTROL_SET) (ADL_CONTEXT_HANDLE, int, ADLODNFanControl*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_POWERLIMIT_GET) (ADL_CONTEXT_HANDLE, int, ADLODNPowerLimitSetting*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_POWERLIMIT_SET) (ADL_CONTEXT_HANDLE, int, ADLODNPowerLimitSetting*);
	typedef int(*TYPEOF_ADL2_OVERDRIVEN_TEMPERATURE_GET) (ADL_CONTEXT_HANDLE, int, int, int*);



	bool m_bInitADL;
	QLibrary *m_adl;
	bool m_adl_adapters_cached;
	
	bool m_error;
	QString m_error_message;
	QString m_error_title;

	struct ADL_ADAPTER_INFO {
		QString adapterName;
		int adapterId;
		int overdriveVersion;
	};
	QList<ADL_ADAPTER_INFO> m_adl_adapters;
	
	TYPEOF_ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;
	TYPEOF_ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy;
	TYPEOF_ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
	TYPEOF_ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get;
	TYPEOF_ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get;
	TYPEOF_ADL_ADAPTER_ID_GET ADL_Adapter_ID_Get;
	TYPEOF_ADL_OVERDRIVE_CAPS ADL_Overdrive_Caps;
	TYPEOF_ADL_OVERDRIVE5_THERMALDEVICES_ENUM ADL_Overdrive5_ThermalDevices_Enum;
	TYPEOF_ADL_OVERDRIVE5_ODPARAMETERS_GET ADL_Overdrive5_ODParameters_Get;
	TYPEOF_ADL_OVERDRIVE5_TEMPERATURE_GET ADL_Overdrive5_Temperature_Get;
	TYPEOF_ADL_OVERDRIVE5_FANSPEED_GET ADL_Overdrive5_FanSpeed_Get;
	TYPEOF_ADL_OVERDRIVE5_FANSPEEDINFO_GET ADL_Overdrive5_FanSpeedInfo_Get;
	TYPEOF_ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET ADL_Overdrive5_ODPerformanceLevels_Get;
	TYPEOF_ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;
	TYPEOF_ADL_OVERDRIVE5_FANSPEED_SET ADL_Overdrive5_FanSpeed_Set;
	TYPEOF_ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET ADL_Overdrive5_ODPerformanceLevels_Set;
	TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_CAPS ADL_Overdrive5_PowerControl_Caps;
	TYPEOF_ADL_OVERDRIVE5_POWERCONTROLINFO_GET ADL_Overdrive5_PowerControlInfo_Get;
	TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_GET ADL_Overdrive5_PowerControl_Get;
	TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_SET ADL_Overdrive5_PowerControl_Set;
	TYPEOF_ADL_OVERDRIVE6_FANSPEED_GET ADL_Overdrive6_FanSpeed_Get;
	TYPEOF_ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS ADL_Overdrive6_ThermalController_Caps;
	TYPEOF_ADL_OVERDRIVE6_TEMPERATURE_GET ADL_Overdrive6_Temperature_Get;
	TYPEOF_ADL_OVERDRIVE6_CAPABILITIES_GET ADL_Overdrive6_Capabilities_Get;
	TYPEOF_ADL_OVERDRIVE6_STATEINFO_GET ADL_Overdrive6_StateInfo_Get;
	TYPEOF_ADL_OVERDRIVE6_CURRENTSTATUS_GET ADL_Overdrive6_CurrentStatus_Get;
	TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_CAPS ADL_Overdrive6_PowerControl_Caps;
	TYPEOF_ADL_OVERDRIVE6_POWERCONTROLINFO_GET ADL_Overdrive6_PowerControlInfo_Get;
	TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_GET ADL_Overdrive6_PowerControl_Get;
	TYPEOF_ADL_OVERDRIVE6_FANSPEED_SET ADL_Overdrive6_FanSpeed_Set;
	TYPEOF_ADL_OVERDRIVE6_STATE_SET ADL_Overdrive6_State_Set;
	TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_SET ADL_Overdrive6_PowerControl_Set;
//	TYPEOF_ADL_ADAPTERX2_CAPS ADL_AdapterX2_Caps;

	TYPEOF_ADL2_ADAPTER_ACTIVE_GET ADL2_Adapter_Active_Get;
	TYPEOF_ADL2_OVERDRIVEN_CAPABILITIES_GET ADL2_OverdriveN_Capabilities_Get;
	TYPEOF_ADL2_OVERDRIVEN_SYSTEMCLOCKS_GET ADL2_OverdriveN_SystemClocks_Get;
	TYPEOF_ADL2_OVERDRIVEN_SYSTEMCLOCKS_SET ADL2_OverdriveN_SystemClocks_Set;
	TYPEOF_ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET ADL2_OverdriveN_PerformanceStatus_Get;
	TYPEOF_ADL2_OVERDRIVEN_FANCONTROL_GET ADL2_OverdriveN_FanControl_Get;
	TYPEOF_ADL2_OVERDRIVEN_FANCONTROL_SET ADL2_OverdriveN_FanControl_Set;
	TYPEOF_ADL2_OVERDRIVEN_POWERLIMIT_GET ADL2_OverdriveN_PowerLimit_Get;
	TYPEOF_ADL2_OVERDRIVEN_POWERLIMIT_SET ADL2_OverdriveN_PowerLimit_Set;
	TYPEOF_ADL2_OVERDRIVEN_MEMORYCLOCKS_GET ADL2_OverdriveN_MemoryClocks_Get;
	TYPEOF_ADL2_OVERDRIVEN_MEMORYCLOCKS_GET ADL2_OverdriveN_MemoryClocks_Set;
	TYPEOF_ADL2_OVERDRIVE_CAPS ADL2_Overdrive_Caps;
	TYPEOF_ADL2_OVERDRIVEN_TEMPERATURE_GET ADL2_OverdriveN_Temperature_Get;

	void InitializeADL(void);
	void TerminateADL(void);

	void GetOverdrive5Info(int adapterId, int &fanspeed, int &fanspeed_rpm, int &utilization, int &temperature);
	void GetOverdrive6Info(int adapterId, int &fanspeed, int &fanspeed_rpm, int &utilization, int &temperature);
	void GetOverdriveNInfo(int adapterId, int &fanspeed, int &fanspeed_rpm, int &utilization, int &temperature);

#endif

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
public:
	bool m_bInitNVAPI;

	void InitializeNVAPI(void);
	void TerminateNVAPI(void);


#endif

public:

	CLC7SystemMonitor(CLC7Controller *ctrl);
	virtual ~CLC7SystemMonitor();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual bool GetAllCPUStatus(QList<ILC7SystemMonitor::CPU_STATUS> & status_per_core, QString &error);
	virtual bool GetAllGPUStatus(QList<ILC7SystemMonitor::GPU_STATUS> & status_per_core, QString &error);
	virtual bool GetErrorMessage(QString &title, QString &message);

};

#endif