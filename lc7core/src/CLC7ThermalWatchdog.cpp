#include<stdafx.h>

CLC7ThermalWatchdog::CLC7ThermalWatchdog(CLC7Controller *ctrl)
{
	m_ctrl = ctrl;
	m_thermal_state = COOL;
	
}

CLC7ThermalWatchdog::~CLC7ThermalWatchdog()
{

}

ILC7Interface *CLC7ThermalWatchdog::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7ThermalWatchdog")
	{
		return this;
	}
	return NULL;
}

void CLC7ThermalWatchdog::NotifyThermalTransition(THERMAL_STATE oldstate, THERMAL_STATE newstate)
{
	foreach(THERMAL_CALLBACK callback, m_callbacks)
	{
		(callback.callback_object->*callback.callback_function)(oldstate, newstate);
	}
}


void CLC7ThermalWatchdog::RegisterNotifyThermalTransition(QObject *callback_object, void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate))
{
	THERMAL_CALLBACK cb;
	cb.callback_function = callback_function;
	cb.callback_object = callback_object;
	m_callbacks.append(cb);
}

void CLC7ThermalWatchdog::UnregisterNotifyThermalTransition(QObject *callback_object, void (QObject::*callback_function)(THERMAL_STATE oldstate, THERMAL_STATE newstate))
{
	THERMAL_CALLBACK cb;
	cb.callback_function = callback_function;
	cb.callback_object = callback_object;
	m_callbacks.removeAll(cb);
}

ILC7ThermalWatchdog::THERMAL_STATE CLC7ThermalWatchdog::thermalState() const
{
	return m_thermal_state;
}

void CLC7ThermalWatchdog::run(void)
{
	ILC7SystemMonitor *sysmon = m_ctrl->GetSystemMonitor();
	ILC7Settings *settings = m_ctrl->GetSettings();

	bool shutdown_batch = false;
	bool shutdown_single = false;
	
	while (!isInterruptionRequested())
	{
		sleep(5);

		bool enable = settings->value("_core_:enablethermal", true).toBool();
		if (!enable)
			continue;
	
		// Check thermals
		int lotemp = settings->value("_core_:gpucooltemp", 75).toInt();
		int hitemp = settings->value("_core_:gputemplimit", 95).toInt();

		QList<ILC7SystemMonitor::GPU_STATUS> gpustats;
		QString error;
		if (sysmon->GetAllGPUStatus(gpustats, error))
		{
			THERMAL_STATE old_thermal_state = m_thermal_state;
			THERMAL_STATE new_thermal_state = COOL;

			foreach(ILC7SystemMonitor::GPU_STATUS st, gpustats)
			{
				if (st.temperature >= lotemp)
				{
					if (st.temperature >= hitemp)
					{
						new_thermal_state = HOT;
						break;
					}
					else
					{
						new_thermal_state = WARM;
					}
				}
			}

			m_thermal_state = new_thermal_state;

			if (old_thermal_state != new_thermal_state)
			{
				NotifyThermalTransition(old_thermal_state, new_thermal_state);
			}
		}
	}
}
