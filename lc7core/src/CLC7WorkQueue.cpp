#include"stdafx.h"

CLC7WorkQueue::CLC7WorkQueue(QUuid handler_id, CLC7Controller *controller, bool validate_limits) :m_acquire(QMutex::Recursive), m_commandctrl(this)
{TR;
	m_handler_id=handler_id;
	m_controller=controller;
	m_stop_enabled=false;
	m_pause_enabled=false;
	m_stop_requested=false;
	m_pause_requested=false;
	m_thermal_shutdown = false;
	m_validate_limits = validate_limits;
	
	SetQueueState(UNVALIDATED);

	connect(this, &CLC7WorkQueue::sig_saveCheckpointConfig, this, &CLC7WorkQueue::slot_saveCheckpointConfig);

	// Attach thermal watchdog
	ILC7ThermalWatchdog *twd = m_controller->GetThermalWatchdog();
	twd->RegisterNotifyThermalTransition(this, (void(QObject::*)(ILC7ThermalWatchdog::THERMAL_STATE, ILC7ThermalWatchdog::THERMAL_STATE))&CLC7WorkQueue::NotifyThermalTransition);
}

CLC7WorkQueue::~CLC7WorkQueue()
{TR;

	// Detach thermal watchdog
	ILC7ThermalWatchdog *twd = m_controller->GetThermalWatchdog();
	twd->UnregisterNotifyThermalTransition(this, (void(QObject::*)(ILC7ThermalWatchdog::THERMAL_STATE, ILC7ThermalWatchdog::THERMAL_STATE))&CLC7WorkQueue::NotifyThermalTransition);
}

ILC7Interface *CLC7WorkQueue::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7WorkQueue")
	{
		return this;
	}
	return NULL;
}

void CLC7WorkQueue::AddQueueChangedListener(QObject *slot_recv, void (QObject::*slot_method)(void))
{TR;
	connect(this,&CLC7WorkQueue::sig_NotifyQueueChanged,slot_recv,slot_method);
}

void CLC7WorkQueue::AddQueueStateChangedListener(QObject *slot_recv, void (QObject::*slot_method)(void))
{TR;
	connect(this, &CLC7WorkQueue::sig_NotifyQueueStateChanged, slot_recv, slot_method);
}


void CLC7WorkQueue::ReportModified()
{TR;
	m_controller->ReportSessionModified();
	emit sig_NotifyQueueChanged();	
}

void CLC7WorkQueue::SaveCheckpointConfig(QMap<QString, QVariant> checkpoint_config)
{
	TR;
	emit sig_saveCheckpointConfig(checkpoint_config);
}

void CLC7WorkQueue::slot_saveCheckpointConfig(QMap<QString, QVariant> checkpoint_config)
{
	TR;
	m_checkpoint_config = checkpoint_config;
}

bool CLC7WorkQueue::IsCheckpointed(void)
{
	TR;
	if (m_queue_state == ILC7WorkQueue::IN_PROGRESS && !isRunning() && !m_checkpoint_config.isEmpty() )
	{
		return true;
	}
	return false;
}

void CLC7WorkQueue::ResumeFromCheckpoint(void)
{
	TR;
	if (!IsCheckpointed())
	{
		Q_ASSERT(0);
		return;
	}
	m_queue_state = ILC7WorkQueue::PAUSED;
	m_paused_config = m_checkpoint_config;
		
	for (int stpos = 0; stpos < m_queueitems_state.size(); stpos++)
	{
		if (m_queueitems_state[stpos] == ILC7WorkQueue::IN_PROGRESS)
		{
			m_queueitems_state[stpos] = ILC7WorkQueue::PAUSED;
		}
	}
}

void CLC7WorkQueue::SetQueueState(STATE st)
{TR;
	{
		QMutexLocker locker(&m_acquire);

		m_queue_state=st;
	}

	ReportModified();
	emit sig_NotifyQueueStateChanged();
	emit sig_uiEnable(st!=IN_PROGRESS);
}

void CLC7WorkQueue::SetQueueItemState(int n,STATE st)
{TR;
	{
		QMutexLocker locker(&m_acquire);
		m_queueitems_state[n]=st;
	}
	ReportModified();
}

bool CLC7WorkQueue::IsStopEnabled(void)
{
	return m_stop_enabled;
}

bool CLC7WorkQueue::IsPauseEnabled(void)
{
	return m_pause_enabled;
}

bool CLC7WorkQueue::IsThermalShutdown(void)
{
	return m_thermal_shutdown;
}

void CLC7WorkQueue::AppendWorkQueueItem(LC7WorkQueueItem item)
{TR;
	{
		QMutexLocker locker(&m_acquire);

		m_queueitems.append(item);
		m_queueitems_state.append(UNVALIDATED);
		m_queue_state=UNVALIDATED;

		emit sig_NotifyQueueStateChanged();

	}

	ReportModified();
}

void CLC7WorkQueue::RemoveWorkQueueItem(int n)
{TR;
	{
		QMutexLocker locker(&m_acquire);

		Q_ASSERT(n<m_queueitems.size() && n>=0);

		m_queueitems.removeAt(n);
		m_queueitems_state.removeAt(n);
		m_queue_state=UNVALIDATED;

		emit sig_NotifyQueueStateChanged();
	}
	ReportModified();
}

void CLC7WorkQueue::ClearWorkQueue(void)
{TR;
	{
		QMutexLocker locker(&m_acquire);

		Q_ASSERT(m_queue_state==UNVALIDATED); // If you hit this you forgot to call 'ResetWorkQueueState'

		m_queueitems.clear();
		m_queueitems_state.clear();
		
	
		m_stop_enabled=false;
		m_pause_enabled=false;
	}	
	ReportModified();
}

void CLC7WorkQueue::SwapWorkQueueItem(int from, int to)
{TR;
	{
		QMutexLocker locker(&m_acquire);

		Q_ASSERT(from<m_queueitems.size() && from>=0);
		Q_ASSERT(to<m_queueitems.size() && to>=0);

		if(from<to)
		{
			to--;
		}

		LC7WorkQueueItem item=m_queueitems.takeAt(from);
		m_queueitems.insert(to,item);
	
		STATE st=m_queueitems_state.takeAt(from);
		m_queueitems_state.insert(to,st);

		m_queue_state=UNVALIDATED;
		emit sig_NotifyQueueStateChanged();
	}
	ReportModified();
}


int CLC7WorkQueue::GetWorkQueueItemCount()
{
	QMutexLocker locker(&m_acquire);
	return m_queueitems.size();
}

LC7WorkQueueItem CLC7WorkQueue::GetWorkQueueItemAt(int i)
{
	QMutexLocker locker(&m_acquire);
	return m_queueitems.at(i);
}


void CLC7WorkQueue::ResetWorkQueueState(void)
{TR;
	{
		QMutexLocker locker(&m_acquire);
		SetQueueState(UNVALIDATED);
		for(int i=0;i<m_queueitems_state.size();i++)
		{
			m_queueitems_state[i]=UNVALIDATED;
		}
	}

	ReportModified();
}
	
CLC7WorkQueue::STATE CLC7WorkQueue::GetWorkQueueItemStateAt(int i)
{
	QMutexLocker locker(&m_acquire);
	return m_queueitems_state.at(i);
}

CLC7WorkQueue::STATE CLC7WorkQueue::GetWorkQueueState(void)
{
	QMutexLocker locker(&m_acquire);
	return m_queue_state;
}

bool CLC7WorkQueue::Validate(QString & error, int & failed_item)
{
	TR;
	if (GetWorkQueueState() == ILC7WorkQueue::PAUSED ||
		GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS ||
		GetWorkQueueState() == ILC7WorkQueue::VALIDATED)
	{
		Q_ASSERT(0);
		return false;
	}

	int n=0;
	QMap<QString, QVariant> state(*(m_controller->GetSessionState()));
	
	foreach(LC7WorkQueueItem item, m_queueitems)
	{
		// Get component
		ILC7Component *comp=m_controller->FindComponentByID(item.GetComponentId());
		if(!comp)
		{
			error="Component not found.";
			failed_item=n;
			SetQueueItemState(n,INVALID);
			SetQueueState(INVALID);

			ReportModified();
			return false;
		}

		// Validate command
		QMap<QString, QVariant> config(item.GetConfig());
		if(!comp->ValidateCommand(state, item.GetCommand(), item.GetArgs(), config, error))
		{
			failed_item=n;
			SetQueueItemState(n,INVALID);
			SetQueueState(INVALID);
					
			ReportModified();
			return false;
		}

		SetQueueItemState(n,VALIDATED);
		n++;
	}

	SetQueueState(VALIDATED);

	ReportModified();
	return true;
}

/*
bool CLC7WorkQueue::IsValidated(void)
{TR;
	QMutexLocker locker(&m_acquire);
	return m_queue_state==VALIDATED;
}
*/

void CLC7WorkQueue::run(void)
{TR;
	CoInitialize(nullptr);

	// Ensure we're validated or paused
	if (m_queue_state != VALIDATED && m_queue_state != PAUSED)
	{
		Q_ASSERT(false);
		return;
	}

	m_stop_requested=false;
	m_pause_requested=false;

	m_stop_enabled=false;
	m_pause_enabled=false;
	SetQueueState(IN_PROGRESS);

	// set up workqueue widget
	GetWorkQueueWidget()->SetStatusText("Preparing work queue");
	GetWorkQueueWidget()->UpdateCurrentActivity("Preparing work queue");
	GetWorkQueueWidget()->AppendToActivityLog("Preparing work queue\n");
	GetWorkQueueWidget()->UpdateCurrentProgressBar(0);
	GetWorkQueueWidget()->UpdateTotalProgressBar(0);

	// run all queue steps in order if they are not completed.
	// continue if we have a paused step
	bool paused=false;

	int step=0,totalstep=m_queueitems.size();
	foreach(LC7WorkQueueItem item,m_queueitems)
	{
		// Skip things that are completed
		if(m_queueitems_state[step]==COMPLETE)
		{
			step++;
			continue;
		}

		QString command = item.GetCommand();
		QStringList args = item.GetArgs();
		QUuid component_id = item.GetComponentId();
		QMap<QString,QVariant> config;
		if(m_queueitems_state[step]==PAUSED)
		{
			// Get paused state config from the last time
			config=m_paused_config;
		}
		else
		{
			// Get fresh config
			config=item.GetConfig();
		}
		m_stop_enabled=item.GetEnableStop();
		m_pause_enabled=item.GetEnablePause();

		QString description=item.GetDescription();
		ILC7Component *comp=m_controller->FindComponentByID(component_id);
		if(!comp)
		{
			m_error="Component not found: "+component_id.toString();
			SetQueueItemState(step,FAIL);
			SetQueueState(FAIL);

			m_stop_enabled=false;
			m_pause_enabled=false;

			ReportModified();
			return;
		}
		
		Q_ASSERT(m_queueitems_state[step]==PAUSED || m_queueitems_state[step]==VALIDATED);

		SetQueueItemState(step,IN_PROGRESS);

		GetWorkQueueWidget()->UpdateCurrentActivity(description);
		GetWorkQueueWidget()->AppendToActivityLog(description + "\n");
		
		GetWorkQueueWidget()->SetStatusText("Validating...");
		
		// Validate command using CURRENT STATE
		if(!comp->ValidateCommand(*(m_controller->GetSessionState()), command, args, config, m_error))
		{
			GetWorkQueueWidget()->SetStatusText("Validation Error");
			GetWorkQueueWidget()->UpdateCurrentActivity("Validation Error");
			GetWorkQueueWidget()->AppendToActivityLog("Validation Error: " + m_error + "\n");
		
 			SetQueueItemState(step,FAIL);
			SetQueueState(FAIL);
			
			m_stop_enabled=false;
			m_pause_enabled=false;
			
			ReportModified();
			return;
		}
		
		GetWorkQueueWidget()->SetStatusText("Running...");

		// Tag queue steps as they execute, 'in progress' and 'completed'
		ILC7Component::RETURNCODE ret=comp->ExecuteCommand(command, args, config, m_error, &m_commandctrl);
		if(ret==ILC7Component::PAUSED)
		{
			if (m_thermal_shutdown)
			{
				GetWorkQueueWidget()->SetStatusText("Thermal Shutdown");
				GetWorkQueueWidget()->UpdateCurrentActivity("Thermal Shutdown");
				GetWorkQueueWidget()->AppendToActivityLog("Thermal Shutdown\n");
			}
			else
			{
				GetWorkQueueWidget()->SetStatusText("Paused");
				GetWorkQueueWidget()->UpdateCurrentActivity("Paused");
				GetWorkQueueWidget()->AppendToActivityLog("Paused\n");
			}

			SetQueueItemState(step,PAUSED);
			SetQueueState(PAUSED);

			m_pause_requested=false;
			m_stop_requested=false;
			
			ReportModified();

			// Keep config for last stopped operation so we can resume it where we left off
			m_paused_config=config;

			return;
		}
		else if(ret==ILC7Component::STOPPED)
		{
			GetWorkQueueWidget()->SetStatusText("Stopped");
			GetWorkQueueWidget()->UpdateCurrentActivity("Stopped");
			GetWorkQueueWidget()->AppendToActivityLog("Stopped\n");

			SetQueueItemState(step,STOPPED);
			SetQueueState(STOPPED);

			m_pause_requested=false;
			m_stop_requested=false;

			m_stop_enabled=false;
			m_pause_enabled=false;

			ReportModified();

			return;
		}
		else if(ret==ILC7Component::FAIL)
		{
			GetWorkQueueWidget()->SetStatusText("Error: " + m_error);
			GetWorkQueueWidget()->UpdateCurrentActivity("Error");
			GetWorkQueueWidget()->AppendToActivityLog("Error: " + m_error + "\n");

			SetQueueItemState(step,FAIL);
			SetQueueState(FAIL);

			m_stop_enabled=false;
			m_pause_enabled=false;
			
			ReportModified();

			return;
		}

		SetQueueItemState(step,COMPLETE);

		step++;
		GetWorkQueueWidget()->UpdateTotalProgressBar(step * 100 / totalstep);
	}
	
	GetWorkQueueWidget()->SetStatusText("Finished");
	GetWorkQueueWidget()->UpdateCurrentActivity("Finished");
	GetWorkQueueWidget()->AppendToActivityLog("Finished\n");
	
	m_stop_enabled=false;
	m_pause_enabled=false;

	SetQueueState(COMPLETE);
}

bool CLC7WorkQueue::StartRequest(void)
{TR;
	if(isRunning())
	{
		Q_ASSERT(0);
		m_error="A queue operation is already in progress. Stop the currently executing operation first.";
		return false;
	}

	if (IsCheckpointed())
	{
		// Resume checkpoint if we have one
		ResumeFromCheckpoint();
	}

	start();

	return true;
}


bool CLC7WorkQueue::StopRequest(void)
{TR;
	if(!m_stop_enabled) 
	{
		Q_ASSERT(0);
		return false;
	}

	if(!isRunning())
	{
		int step=0,totalstep=m_queueitems.size();
		foreach(LC7WorkQueueItem item,m_queueitems)
		{
			// Skip things that are completed
			if(m_queueitems_state[step]==COMPLETE)
			{
				step++;
				continue;
			}
			SetQueueItemState(step,STOPPED);
			break;
		}

		// Just clear state
		m_paused_config.clear();
		
		GetWorkQueueWidget()->SetStatusText("Stopped");
		GetWorkQueueWidget()->UpdateCurrentActivity("Stopped");
		GetWorkQueueWidget()->AppendToActivityLog("Stopped\n");

		SetQueueState(STOPPED);

		m_pause_requested=false;
		m_stop_requested=false;

		m_stop_enabled=false;
		m_pause_enabled=false;

		ReportModified();

		return true;
	}

	if(m_queue_state==IN_PROGRESS)
	{
		m_stop_requested=true;
		ReportModified();
		return true;
	}
	else if(m_queue_state==PAUSED)
	{

		return true;
	}

	Q_ASSERT(0);
	m_error="Queue operation could not be stopped.";
	return false;
}

bool CLC7WorkQueue::PauseRequest(void)
{TR;
	if(!m_pause_enabled || !isRunning())
	{
		Q_ASSERT(0);
		m_error="No queue operation in progress.";
		return false;
	}
	if(m_queue_state==IN_PROGRESS)
	{
		m_pause_requested=true;
		ReportModified();
		return true;
	}

	Q_ASSERT(0);
	m_error="Queue operation could not be paused.";
	return false;
}

bool CLC7WorkQueue::IsStopRequested(void)
{
	return m_stop_requested;
}

bool CLC7WorkQueue::IsPauseRequested(void)
{
	return m_pause_requested;
}

QString CLC7WorkQueue::GetLastError(void)
{
	return m_error;
}

void CLC7WorkQueue::RegisterUIEnable(QWidget *widget)
{TR;
	connect(this, SIGNAL(sig_uiEnable(bool)), widget, SLOT(slot_uiEnable(bool)));
}

ILC7WorkQueueWidget *CLC7WorkQueue::GetWorkQueueWidget()
{
	return m_controller->GetGUILinkage()->GetWorkQueueWidget();
}

QUuid CLC7WorkQueue::GetId()
{
	return m_handler_id;
}

void CLC7WorkQueue::Acquire()
{TR;
	m_acquire.lock();
}

void CLC7WorkQueue::Release()
{TR;
	m_acquire.unlock();
}

bool CLC7WorkQueue::Save(QDataStream & out)
{TR;
	QMutexLocker locker(&m_acquire);

	quint32 version = 2;
	out << version;

	out << (int) m_queue_state;
	out << m_stop_enabled;
	out << m_pause_enabled;

	quint32 size=m_queueitems.size();
	out << size;

	foreach(LC7WorkQueueItem item, m_queueitems)
	{
		QUuid component_id=item.GetComponentId();
		QString command=item.GetCommand();
		QStringList args = item.GetArgs();
		QMap<QString,QVariant> config=item.GetConfig();
		QString description=item.GetDescription();
		bool enable_stop=item.GetEnableStop();
		bool enable_pause=item.GetEnablePause();

		out << component_id;
		out << command;
		out << args;
		out << config;
		out << description;
		out << enable_stop;
		out << enable_pause;
	}

	foreach(STATE st, m_queueitems_state)
	{
		out << (int)st;
	}

	out << m_paused_config;
	out << m_checkpoint_config;
	out << m_thermal_shutdown;

	return out.status()==QDataStream::Ok;
}

bool CLC7WorkQueue::Load(QDataStream & in)
{TR;
	bool ok = true;
	{
		QMutexLocker locker(&m_acquire);

		quint32 version;

		in >> version;
		if (version > 2)
		{
			return false;
		}

		in >> (int &)m_queue_state;
		in >> (bool &)m_stop_enabled;
		in >> (bool &)m_pause_enabled;

		quint32 size;
		in >> size;

		for (quint32 i = 0; i < size; i++)
		{
			QUuid component_id;
			QString command;
			QStringList args;
			QMap<QString, QVariant> config;
			QString description;
			bool enable_stop;
			bool enable_pause;

			in >> component_id;
			in >> command;
			in >> args;
			in >> config;
			in >> description;
			in >> enable_stop;
			in >> enable_pause;

			LC7WorkQueueItem item(component_id, command, args, config, description, enable_stop, enable_pause);

			m_queueitems.append(item);
		}

		for (quint32 i = 0; i < size; i++)
		{
			STATE st;
			in >> (int &)st;
			m_queueitems_state.append(st);
		}

		in >> m_paused_config;
		
		if (version >= 2)
		{
			in >> m_checkpoint_config;
			in >> m_thermal_shutdown;
		}

		ok = ok && (in.status() == QDataStream::Ok);
	}

	ReportModified();

	return ok;
}

void CLC7WorkQueue::NotifyThermalTransition(ILC7ThermalWatchdog::THERMAL_STATE oldstate, ILC7ThermalWatchdog::THERMAL_STATE newstate)
{
	if (!m_thermal_shutdown && newstate == ILC7ThermalWatchdog::HOT && IsPauseEnabled() && GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
	{
		if (PauseRequest())
		{
			m_thermal_shutdown = true;

			emit sig_NotifyQueueStateChanged();
		}
	}	
	else if (m_thermal_shutdown && newstate == ILC7ThermalWatchdog::COOL && IsPauseEnabled() && GetWorkQueueState() == ILC7WorkQueue::PAUSED)
	{
		if (StartRequest())
		{
			m_thermal_shutdown = false;

			emit sig_NotifyQueueStateChanged();
		}
	}
}
