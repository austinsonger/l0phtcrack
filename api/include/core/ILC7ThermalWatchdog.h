#ifndef __INC_ILC7THERMALWATCHDOG_H
#define __INC_ILC7THERMALWATCHDOG_H

class ILC7ThermalWatchdog : public ILC7Interface
{
public:
	enum THERMAL_STATE {
		COOL = 0,
		WARM = 1,
		HOT = 2
	};

protected:
	virtual ~ILC7ThermalWatchdog() {}

public:

	virtual void RegisterNotifyThermalTransition(QObject *callback_object, void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate)) = 0;
	virtual void UnregisterNotifyThermalTransition(QObject *callback_object, void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate)) = 0;
	virtual THERMAL_STATE thermalState() const = 0;
};

#endif