#ifndef __INC_ILC7SYSTEMMONITOR_H
#define __INC_ILC7SYSTEMMONITOR_H

#include"core/ILC7Interface.h"

class ILC7SystemMonitor:public ILC7Interface
{

public:

	struct CPU_STATUS
	{
		quint32 max_mhz;		// mhz
		quint32 current_mhz;	// mhz
		quint32 mhz_limit;		// mhz
		quint32 utilization;	// percent
	};

	struct GPU_STATUS
	{
		QString gpu_name;	// Device Name
		QString gpu_type;	// "AMD" or "NVIDIA"
		qreal utilization; 	// percent
		qreal temperature; 	// degress celsius
		qreal fanspeed;    	// percent
		qreal fanspeed_rpm; // rpm
	};

protected:
	virtual ~ILC7SystemMonitor() {}

public:
	virtual bool GetAllCPUStatus(QList<ILC7SystemMonitor::CPU_STATUS> & status_per_core, QString &error)=0;
	virtual bool GetAllGPUStatus(QList<ILC7SystemMonitor::GPU_STATUS> & status_per_core, QString &error)=0;
	virtual bool GetErrorMessage(QString &title, QString &message)=0;

};


#endif
