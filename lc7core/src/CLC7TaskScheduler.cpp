#include"stdafx.h"

CLC7TaskScheduler::CLC7TaskScheduler(CLC7Controller *ctrl)
{TR;
	m_ctrl = ctrl;

	m_bAvailable = false;
#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
	bool usev2 = ctrl->GetSettings()->value("_core_:task_scheduler_version").toBool();

	if (!usev2)
	{
		m_sched = new CTaskScheduler_WIN32v1(ctrl);
	}
	else
	{
		m_sched = new CTaskScheduler_WIN32v2(ctrl);
	}
#else
#error implement me
#endif

}

CLC7TaskScheduler::~CLC7TaskScheduler()
{TR;
	ClearTasks();
	delete m_sched;
}

ILC7Interface *CLC7TaskScheduler::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7TaskScheduler")
	{
		return this;
	}
	return NULL;
}

bool CLC7TaskScheduler::Initialize(QString &error)
{TR;
	if (!m_sched->Startup(error))
	{
		return false;
	}
	RefreshTasks();

	m_bAvailable = true;
	return true;
}

bool CLC7TaskScheduler::IsAvailable()
{
	return m_bAvailable;
}


ILC7EditableTask *CLC7TaskScheduler::NewTask()
{TR;

	if (!m_ctrl->GetGUILinkage()->YesNoBox("Security Warning", "Creating a scheduled task may store the credentials you have entered in a file readable on this system to administrators or to those with physical access. Do you accept this risk and want to proceed?"))
	{
		return NULL;
	}

	ILC7WorkQueue *batchqueue = (ILC7WorkQueue *)m_ctrl->GetSessionHandler(BATCH_WORKQUEUE_HANDLER_ID);
	if (!batchqueue)
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Error creating task", "Session is not open.");
		return NULL;
	}

	CLC7Task *task = new CLC7Task();
	task->SetId(QUuid::createUuid().toString());

	QString tempfile = m_ctrl->NewTemporaryFile(false);

	if (!m_ctrl->SaveSession(tempfile, true, true))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Error creating task", "Error saving temporary session file");
		delete task;
		return NULL;
	}
	QString error;
	if (!task->SetSessionFile(tempfile, error))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Error creating task", error);
		delete task;
		return NULL;
	}
	QFile::remove(tempfile);

	if (!(batchqueue->GetWorkQueueState() == ILC7WorkQueue::VALIDATED ||
		batchqueue->GetWorkQueueState() == ILC7WorkQueue::STOPPED ||
		batchqueue->GetWorkQueueState() == ILC7WorkQueue::FAIL ||
		batchqueue->GetWorkQueueState() == ILC7WorkQueue::COMPLETE))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Error creating task", "Work queue is not validated");
		delete task;
		return NULL;
	}

	int cnt = batchqueue->GetWorkQueueItemCount();
	for (int i = 0; i < cnt; i++)
	{
		LC7WorkQueueItem item = batchqueue->GetWorkQueueItemAt(i);
		task->AppendTaskDescription(item.GetDescription());
	}

	return task;
}

void CLC7TaskScheduler::DeleteTask(ILC7EditableTask *task)
{TR;
	delete (CLC7Task *)task;
}

bool CLC7TaskScheduler::SaveTaskFile(CLC7Task *task, QString &taskpath, QString &error)
{TR;
	QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir schtasksdir(appdata);
	schtasksdir.mkpath("Tasks");
	if (!schtasksdir.cd("Tasks") || !schtasksdir.exists())
	{
		error = "Couldn't locate tasks folder";
		return false;
	}

	QString taskname = task->GetId() + ".lc7task";

	taskpath = schtasksdir.absoluteFilePath(taskname);

	QFile sf(taskpath);
	if (!sf.open(QIODevice::WriteOnly))
	{
		error = "Couldn't open task file.";
		return false;
	}

	QDataStream taskdata(&sf);
	if (!task->Save(taskdata))
	{
		error = "Couldn't write session file copy for task, possibly out of disk space.";
		sf.close();
		schtasksdir.remove(taskname);
		return false;
	}

	sf.close();
	return true;
}

ILC7Task *CLC7TaskScheduler::ScheduleTask(ILC7EditableTask *etask, QString & error, bool &cancelled)
{TR;
	cancelled = false;

	CLC7Task *task = (CLC7Task *)etask;

	if (m_sched->IsTaskScheduled(task))
	{
		error = "Task is already scheduled.";
		return NULL;
	}

	// Get credentials
	void *creds;
	if (!m_sched->GetCredentialFromUser(&creds, error, cancelled) || cancelled)
	{
		return NULL;
	}

	// Save session file to temp location
	QString taskpath;
	if (!SaveTaskFile(task, taskpath, error))
	{
		m_sched->ReleaseCredential(creds);
		return NULL;
	}

	// Build command line
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString appname = QDir::toNativeSeparators(qApp->applicationFilePath());

	QString parameters = QString("--task \"%1\"").arg(task->GetId());
#else
#error implement me!
#endif

	if (!m_sched->ScheduleTask(task, appname, parameters, creds, error))
	{
		QFile::remove(taskpath);
		m_sched->ReleaseCredential(creds);
		return NULL;
	}
	m_sched->ReleaseCredential(creds);

	if (cancelled)
	{
		QFile::remove(taskpath);
	}

	// Duplicate to destroy editable task
	CLC7Task *task2 = new CLC7Task(*(CLC7Task *)etask);
	delete (CLC7Task *)etask;
	task = task2;
	
	// Add this committed task to the list of tasks
	m_tasks.append(task);
	m_tasks_by_id[task->GetId()] = task;

	return task;
}

ILC7EditableTask *CLC7TaskScheduler::UnscheduleTask(ILC7Task *itask, QString & error)
{TR;
	CLC7Task *task = (CLC7Task *)itask;

	if (!m_tasks.contains(task))
	{
		return NULL;
	}

	if (!m_sched->UnscheduleTask(task, error))
	{
		return NULL;
	}
	
	QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir schtasksdir(appdata);
	if (!schtasksdir.cd("Tasks") || !schtasksdir.exists())
	{
		error = "Couldn't locate tasks folder";
		return NULL;
	}

	QString taskname = task->GetId() + ".lc7task";
	QString taskpath = schtasksdir.absoluteFilePath(taskname);

	if (QFileInfo(taskpath).isFile())
	{
		QFile::remove(taskpath);
	}

	m_tasks.removeAll(task);
	m_tasks_by_id.remove(task->GetId());

	return task;
}

int CLC7TaskScheduler::GetScheduledTaskCount(void)
{TR;
	return m_tasks.count();
}

ILC7Task *CLC7TaskScheduler::GetScheduledTask(int pos)
{TR;
	return m_tasks.at(pos);
}

ILC7Task *CLC7TaskScheduler::FindScheduledTaskById(QString taskid)
{TR;
	return m_tasks_by_id[taskid];
}


ILC7EditableTask *CLC7TaskScheduler::EditScheduledTask(ILC7Task *itask)
{TR;
	return new CLC7Task(*(CLC7Task *)itask);
}

bool CLC7TaskScheduler::CommitScheduledTask(ILC7EditableTask *task, QString &error, bool &cancelled)
{TR;
	cancelled = false;

	// Get credentials
	void *creds;
	if (!m_sched->GetCredentialFromUser(&creds, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	QString taskpath;
	if (!SaveTaskFile((CLC7Task *)task, taskpath, error))
	{
		m_sched->ReleaseCredential(creds);
		return false;
	}

	if (!m_sched->CommitTask((CLC7Task *)task, creds, error))
	{
		m_sched->ReleaseCredential(creds);
		return false;
	}

	m_sched->ReleaseCredential(creds);

	DeleteTask(task);
	return true;
}

void CLC7TaskScheduler::RefreshTasks(void)
{TR;
	ClearTasks();

	QString error;
	if (!m_sched->RefreshScheduledTasks(error))
	{
		return;
	}

	QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir schtasksdir(appdata);
	if (!schtasksdir.cd("Tasks") || !schtasksdir.exists())
	{
		return;
	}

	QStringList to_remove;
	QDirIterator diriter(schtasksdir.absolutePath(),
		QStringList() << "*.lc7task",
		QDir::NoSymLinks | QDir::Files);
	while (diriter.hasNext())
	{
		QString taskfile = diriter.next();

		QFile tf(taskfile);
		if (tf.open(QIODevice::ReadOnly))
		{
			QDataStream ds(&tf);

			CLC7Task *task = new CLC7Task();
			if (task->Load(ds))
			{
				if (m_sched->ValidateScheduledTask(task))
				{
					m_tasks.append(task);
					m_tasks_by_id[task->GetId()] = task;
					continue;
				}
			}
			delete task;
			tf.close();
		}

		to_remove.append(taskfile);
	}
	foreach(QString taskfile, to_remove)
	{
		QFile::remove(taskfile);
	}

	m_sched->PurgeInvalidTasks(m_tasks);
}

void CLC7TaskScheduler::ClearTasks(void)
{TR;
	foreach(CLC7Task *task, m_tasks)
	{
		delete task;
	}
	m_tasks.clear();
	m_tasks_by_id.clear();
}

bool CLC7TaskScheduler::NewSessionFromTask(ILC7Task *itask)
{TR;
	CLC7Task *task = (CLC7Task *)itask;

	QByteArray data = task->GetSessionData();
		
	if (!m_ctrl->NewSessionFromData(data))
	{
		return false;
	}

	return true;
}

bool CLC7TaskScheduler::RunTask(ILC7Task *itask, bool quit_on_finish)
{TR;
	CLC7Task *task = (CLC7Task *)itask;

	if (!m_ctrl->RunTask(itask->GetId(), quit_on_finish))
	{
		return false;
	}

	return true;
}

bool CLC7TaskScheduler::IsTaskRunning(void)
{TR;
	return m_ctrl->IsTaskRunning();
}


void CLC7TaskScheduler::RegisterTaskFinishedCallback(QObject *callback_object, void (QObject::*callback_function)(ILC7Task *))
{TR;
	m_task_finished_callbacks.append(QPair<QObject *, void (QObject::*)(ILC7Task *)>(callback_object, callback_function));
}

void CLC7TaskScheduler::UnregisterTaskFinishedCallback(QObject *callback_object, void (QObject::*callback_function)(ILC7Task *))
{TR;
	QPair<QObject *, void (QObject::*)(ILC7Task *)> cb = QPair<QObject *, void (QObject::*)(ILC7Task *)>(callback_object, callback_function);
	m_task_finished_callbacks.removeAll(cb);
}

void CLC7TaskScheduler::CallTaskFinishedCallbacks(ILC7Task *finished_task)
{TR;
	QPair<QObject *, void (QObject::*)(ILC7Task *)> cb;
	foreach(cb, m_task_finished_callbacks)
	{
		((cb.first)->*(cb.second))(finished_task);
	}
}
