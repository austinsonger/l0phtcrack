#ifndef __INC_CLC7THERMALWATCHDOG_H
#define __INC_CLC7THERMALWATCHDOG_H

class CLC7ThermalWatchdog : public QThread, public ILC7ThermalWatchdog
{
	Q_OBJECT;
private:

	CLC7Controller *m_ctrl;
	THERMAL_STATE m_thermal_state;

	struct THERMAL_CALLBACK
	{
		QObject *callback_object;
		void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate);
		bool operator==(const THERMAL_CALLBACK &other) const
		{
			return callback_object == other.callback_object && callback_function == other.callback_function;
		}
	};
	QList<THERMAL_CALLBACK> m_callbacks;

	void NotifyThermalTransition(THERMAL_STATE oldstate, THERMAL_STATE newstate);
	
signals:

	void sig_thermalTransition(THERMAL_STATE oldstate, THERMAL_STATE newstate);

public:

	CLC7ThermalWatchdog(CLC7Controller *ctrl);
	virtual ~CLC7ThermalWatchdog();

	virtual void run(void);

	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	virtual void RegisterNotifyThermalTransition(QObject *callback_object, void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate));
	virtual void UnregisterNotifyThermalTransition(QObject *callback_object, void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate));
	virtual THERMAL_STATE thermalState() const;
	
};

#endif