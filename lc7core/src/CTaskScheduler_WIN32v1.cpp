#include<stdafx.h>

//#define SECURITY_KERNEL
#define SECURITY_WIN32
//#define SECURITY_MAC
#include<Windows.h>
#include <security.h>
#include <secext.h>

// task scheduler
#include <mstask.h>

// service stuff
#include <winsvc.h>

class AutoRelease
{
	IUnknown *m_ptr;
public:
	AutoRelease(IUnknown *ptr)
	{
		m_ptr = ptr;
	}
	~AutoRelease()
	{
		m_ptr->Release();
	}
};

class AutoReleaseMem
{
	void *m_ptr;
public:
	AutoReleaseMem(void *ptr)
	{
		m_ptr = ptr;
	}
	~AutoReleaseMem()
	{
		CoTaskMemFree(m_ptr);
	}
};


static bool GetCurrentUserName(QString & user, QString & error)
{
	// Get current user name
	ULONG chars = 256;
	WCHAR *username = (WCHAR *)malloc(chars * sizeof(WCHAR));
	if (username == NULL)
	{
		error = "Out of memory";
		return false;
	}
	BOOLEAN ret = GetUserNameExW(NameSamCompatible, username, &chars);
	DWORD le = GetLastError();
	if (!ret)
	{
		if (le == ERROR_MORE_DATA)
		{
			username = (WCHAR *)realloc(username, chars*sizeof(WCHAR));
			if (username == NULL)
			{
				error = "Out of memory";
				return false;
			}
			ret = GetUserNameExW(NameSamCompatible, username, &chars);
			if (!ret)
			{
				free(username);
				error = "Out of memory";
				return false;
			}
		}
		else
		{
			error = "Unable to get user name";
			free(username);
			return false;
		}
	}
	user = QString::fromWCharArray(username);
	free(username);

	return true;
}


CTaskScheduler_WIN32v1::InternalTask::InternalTask()
{TR;
	m_id="";
	m_expiration_enabled=false;
	m_recurrence=ILC7Task::ONE_TIME;
	m_daily_recurrence=1;
	m_weekly_recurrence=1;
	m_enabled_days_of_week=0x0000007F;
	m_monthly_mode = ILC7Task::SPECIFIC_DAY;
	m_specific_day_of_month=1;
	m_abstract_timing = ILC7Task::FIRST;
	m_abstract_day_of_week = ILC7Task::FRIDAY;
	m_enabled_months = 0x00000FFF;
}

CTaskScheduler_WIN32v1::CTaskScheduler_WIN32v1(CLC7Controller *ctrl)
{TR;
	m_ctrl = ctrl;
	m_tasklist_valid = false;
}

CTaskScheduler_WIN32v1::~CTaskScheduler_WIN32v1()
{TR;
}


bool CTaskScheduler_WIN32v1::Startup(QString & error)
{TR;
	if (!GetCurrentUserName(m_username, error))
	{
		return false;
	}
	
	/*

	BOOL bOsVersionInfoEx;
	OSVERSIONINFOEX osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *)&osvi)))
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx((OSVERSIONINFO *)&osvi);
	}

	if (osvi.dwMajorVersion < 5)
	{
		error = "Scheduling is only supported on Windows 2000 or later.";
		return false;
	}
	*/

	// start task scheduler service in case it isn't !

	SC_HANDLE   hSC = NULL;
	SC_HANDLE   hSchSvc = NULL;

	hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (hSC != NULL)
	{
		hSchSvc = OpenService(hSC, "Schedule", SERVICE_QUERY_STATUS);
		if (hSchSvc != NULL)
		{
			SERVICE_STATUS SvcStatus;
			if (QueryServiceStatus(hSchSvc, &SvcStatus))
			{
				if (SvcStatus.dwCurrentState != SERVICE_RUNNING)
				{
					CloseServiceHandle(hSchSvc);
					hSchSvc = OpenService(hSC, "Schedule", SERVICE_START);
					if (hSchSvc)
					{
						if (StartService(hSchSvc, 0, NULL) == FALSE)
						{
							CloseServiceHandle(hSchSvc);
							CloseServiceHandle(hSC);
							error = "The Task Scheduler service could not be started. You may not have permission to perform this action.";
							return false;
						}
					}
				}
			}
			CloseServiceHandle(hSchSvc);
		}
		CloseServiceHandle(hSC);
	}

	if (!RefreshScheduledTasks(error))
	{
		return false;
	}

	return true;
}

bool CTaskScheduler_WIN32v1::RemoveTask(InternalTask *task, QString &error)
{TR;
	// create task scheduler object

	ITaskScheduler *pITS;

	HRESULT hr = CoCreateInstance(CLSID_CTaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskScheduler,
		(void **)&pITS);

	if (FAILED(hr))
	{
		error = "Couldn't access task scheduler";
		return false;
	}

	AutoRelease arITS(pITS);

	hr = pITS->Delete(QString("LC7_%1").arg(task->m_id).toStdWString().c_str());
	if (FAILED(hr))
	{
		hr = pITS->Delete(QString("(LC7) %1").arg(task->m_id).toStdWString().c_str());
		if (FAILED(hr))
		{
			error = "Couldn't delete task from system scheduler";
			return false;
		}
	}
	
	m_tasks.removeAll(task);
	m_tasks_by_id.remove(task->m_id);

	return true;
}


void CTaskScheduler_WIN32v1::ClearTasks()
{TR;
	foreach(InternalTask *task, m_tasks)
	{
		delete task;
	}
	m_tasks.clear();
	m_tasks_by_id.clear();
	m_tasklist_valid = false;
}

void CTaskScheduler_WIN32v1::SetupTrigger(TASK_TRIGGER & trigger, CLC7Task *lc7task, CTaskScheduler_WIN32v1::InternalTask *task)
{TR;
	ZeroMemory(&trigger, sizeof(TASK_TRIGGER));
	trigger.cbTriggerSize = sizeof(TASK_TRIGGER);

	// Start Time
	task->m_start_time = lc7task->GetStartTime();
	trigger.wBeginDay = task->m_start_time.date().day();
	trigger.wBeginMonth = task->m_start_time.date().month();
	trigger.wBeginYear = task->m_start_time.date().year();
	trigger.wStartHour = task->m_start_time.time().hour();
	trigger.wStartMinute = task->m_start_time.time().minute();

	// Expiration Date
	task->m_expiration_enabled = lc7task->GetExpirationEnabled();
	task->m_expiration_date = lc7task->GetExpirationDate();
	if (task->m_expiration_enabled)
	{
		trigger.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
		trigger.wEndDay = task->m_expiration_date.day();
		trigger.wEndMonth = task->m_expiration_date.month();
		trigger.wEndYear = task->m_expiration_date.year();
	}

	// Recurrence Type
	task->m_recurrence = lc7task->GetRecurrence();
	switch (task->m_recurrence)
	{
	case ILC7Task::ONE_TIME:
		trigger.TriggerType = TASK_TIME_TRIGGER_ONCE;
		break;

	case ILC7Task::DAILY:

		trigger.TriggerType = TASK_TIME_TRIGGER_DAILY;
		task->m_daily_recurrence = lc7task->GetDailyRecurrence();
		trigger.Type.Daily.DaysInterval = task->m_daily_recurrence;
		break;

	case ILC7Task::WEEKLY:
		trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;

		task->m_weekly_recurrence = lc7task->GetWeeklyRecurrence();
		trigger.Type.Weekly.WeeksInterval = task->m_weekly_recurrence;

		task->m_enabled_days_of_week = lc7task->GetEnabledWeekDaysBitMask();
		trigger.Type.Weekly.rgfDaysOfTheWeek = task->m_enabled_days_of_week;
		break;

	case ILC7Task::MONTHLY:
		task->m_monthly_mode = lc7task->GetMonthlyMode();
		task->m_enabled_months = lc7task->GetEnabledMonthsBitMask();
		if (task->m_monthly_mode == ILC7Task::SPECIFIC_DAY)
		{
			trigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;

			task->m_specific_day_of_month = lc7task->GetSpecificDayOfMonth();
			trigger.Type.MonthlyDate.rgfDays = 1 << (task->m_specific_day_of_month-1);

			trigger.Type.MonthlyDate.rgfMonths = task->m_enabled_months;
		}
		else
		{
			trigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;

			task->m_abstract_timing = lc7task->GetAbstractTiming();
			trigger.Type.MonthlyDOW.wWhichWeek = ((int)task->m_abstract_timing) + 1;

			task->m_abstract_day_of_week = lc7task->GetAbstractDayOfWeek();

			trigger.Type.MonthlyDOW.rgfDaysOfTheWeek = 1 << task->m_abstract_day_of_week;

			trigger.Type.MonthlyDOW.rgfMonths = task->m_enabled_months;
		}
		break;
	}
}


#define CHECK_HR(x) if (FAILED(x)) { delete task; return NULL; }
CTaskScheduler_WIN32v1::InternalTask *CTaskScheduler_WIN32v1::SetupTask(ITask *pITask, CLC7Task *lc7task, QString appname, QString parameters, QString passwd)
{TR;
	InternalTask *task = new InternalTask();
	task->m_id = lc7task->GetId();

	// Command line
	QString apppath = QDir::toNativeSeparators(QFileInfo(QDir::fromNativeSeparators(appname)).absoluteDir().absolutePath());
	task->m_appname = appname;
	task->m_parameters = parameters;
	task->m_description = lc7task->GetTaskDescriptions().join("\n");

	pITask->SetApplicationName(task->m_appname.toStdWString().c_str());
	pITask->SetParameters(task->m_parameters.toStdWString().c_str());
	pITask->SetComment(task->m_description.toStdWString().c_str());
	pITask->SetMaxRunTime(INFINITE);
	pITask->SetWorkingDirectory(apppath.toStdWString().c_str());

	// Run as user
	task->m_run_as_user = m_username;
	CHECK_HR(pITask->SetAccountInformation(m_username.toStdWString().c_str(), passwd.toStdWString().c_str()));

	// Create trigger
	ITaskTrigger *pITaskTrigger;
	WORD piNewTrigger;
	CHECK_HR(pITask->CreateTrigger(&piNewTrigger, &pITaskTrigger));
	AutoRelease arTrigger(pITaskTrigger);

	// Build the trigger
	TASK_TRIGGER trigger;
	SetupTrigger(trigger, lc7task, task);
		
	// Set the trigger
	CHECK_HR(pITaskTrigger->SetTrigger(&trigger));

	// Save the system task
	IPersistFile *pIPF;
	CHECK_HR(pITask->QueryInterface(IID_IPersistFile, (void **)&pIPF));
	AutoRelease arIPF(pIPF);
	CHECK_HR(pIPF->Save(NULL, TRUE));
		
	return task;
}
#undef CHECK_HR


#define CHECK_HR(x) if (FAILED(x)) { delete task; return NULL; }
CTaskScheduler_WIN32v1::InternalTask *CTaskScheduler_WIN32v1::LoadTask(QString taskid, ITask *pITask)
{TR;
	InternalTask *task = new InternalTask();

	task->m_id = taskid;

	// Command line
	LPWSTR application_name;
	LPWSTR parameters;
	LPWSTR comment;
	CHECK_HR(pITask->GetApplicationName(&application_name));
	AutoReleaseMem ar_appname(application_name);
	CHECK_HR(pITask->GetParameters(&parameters));
	AutoReleaseMem ar_param(parameters);
	CHECK_HR(pITask->GetComment(&comment));
	AutoReleaseMem ar_comment(comment);

	task->m_appname = QString::fromWCharArray(application_name);
	task->m_parameters = QString::fromWCharArray(parameters);
	task->m_description = QString::fromWCharArray(comment);

	// Run as user
	LPWSTR account_name;
	CHECK_HR(pITask->GetAccountInformation(&account_name));
	AutoReleaseMem ar_acctname(account_name);

	task->m_run_as_user = QString::fromWCharArray(account_name);

	// Skip this one if it's not ours. Theoretically it's possible
	// for two different users to have scheduled LC7 jobs on the same machine.
	if (task->m_run_as_user != m_username)
	{
		delete task;
		return NULL;
	}

	// Start Time
	WORD trigcnt;
	CHECK_HR(pITask->GetTriggerCount(&trigcnt));
	if (trigcnt != 1)
	{
		delete task;
		return NULL;
	}
	ITaskTrigger *pITT;
	CHECK_HR(pITask->GetTrigger(0, &pITT)); 
	AutoRelease arITT(pITT);
	TASK_TRIGGER tt;
	memset(&tt, 0, sizeof(TASK_TRIGGER));
	tt.cbTriggerSize = sizeof(TASK_TRIGGER);
	CHECK_HR(pITT->GetTrigger(&tt));
	task->m_start_time = QDateTime(QDate(tt.wBeginYear, tt.wBeginMonth, tt.wBeginDay), QTime(tt.wStartHour, tt.wStartMinute));
	
	// Expiration Date
	if (tt.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
	{
		task->m_expiration_enabled = true;
		task->m_expiration_date = QDate(tt.wEndYear, tt.wEndMonth, tt.wEndDay);

	}
	else
	{
		task->m_expiration_enabled = false;
	}

	// Recurrence Type
	switch (tt.TriggerType)
	{
	case TASK_TIME_TRIGGER_ONCE:
		task->m_recurrence = ILC7Task::ONE_TIME;
		break;
	case TASK_TIME_TRIGGER_DAILY:
		task->m_recurrence = ILC7Task::DAILY;
		task->m_daily_recurrence = tt.Type.Daily.DaysInterval;
		break;
	case TASK_TIME_TRIGGER_WEEKLY:
		task->m_recurrence = ILC7Task::WEEKLY;
		task->m_weekly_recurrence = tt.Type.Weekly.WeeksInterval;
		task->m_enabled_days_of_week = tt.Type.Weekly.rgfDaysOfTheWeek;
		break;
	case TASK_TIME_TRIGGER_MONTHLYDATE:
	case TASK_TIME_TRIGGER_MONTHLYDOW:
		task->m_recurrence = ILC7Task::MONTHLY;
		if (tt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE)
		{
			task->m_monthly_mode = ILC7Task::SPECIFIC_DAY;
			DWORD d = tt.Type.MonthlyDate.rgfDays;
			int day = 0;
			for (int x = 0; x < 31; x++)
			{
				if (d & (1 << x))
				{
					if (day == 0)
					{
						day = x + 1;
					}
					else
					{
						delete task;
						return NULL;
					}
				}
			}
			if (day == 0)
			{
				delete task;
				return NULL;
			}

			task->m_specific_day_of_month = day;
			task->m_enabled_months = (quint32)tt.Type.MonthlyDate.rgfMonths;
		}
		else
		{
			task->m_monthly_mode = ILC7Task::ABSTRACT_DAY;
			task->m_abstract_timing = (ILC7Task::TIMING)(tt.Type.MonthlyDOW.wWhichWeek - 1);
			DWORD d = tt.Type.MonthlyDOW.rgfDaysOfTheWeek;
			int day = -1;
			for (int x = 0; x < 7; x++)
			{
				if (d & (1 << x))
				{
					if (day == -1)
					{
						day = x;
					}
					else
					{
						delete task;
						return NULL;
					}
				}
			}
			if (day == -1)
			{
				delete task;
				return NULL;
			}
			task->m_abstract_day_of_week = (ILC7Task::DAY_OF_WEEK)day;
			task->m_enabled_months = (quint32) tt.Type.MonthlyDOW.rgfMonths;
		}
	
		break;
	default:
		delete task;
		return NULL;
	}
	
	return task;
}
#undef CHECK_HR


bool CTaskScheduler_WIN32v1::RefreshScheduledTasks(QString &error)
{TR;
	ClearTasks();

	// create task scheduler object

	ITaskScheduler *pITS;

	HRESULT hr = CoCreateInstance(CLSID_CTaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskScheduler,
		(void **)&pITS);

	if (FAILED(hr))
	{
		error = "Couldn't access task scheduler";
		return false;
	}

	
	// get enumerator object
	IEnumWorkItems *pIEnum;
	hr = pITS->Enum(&pIEnum);
	if (FAILED(hr))
	{
		error = "Couldn't access task scheduler enumerator";
		pITS->Release();
		return false;
	}
		
	LPWSTR *lpwszNames=NULL;
	while (SUCCEEDED(pIEnum->Next(1, &lpwszNames, NULL)) && lpwszNames)
	{
		QString taskid = QString::fromWCharArray(lpwszNames[0]);
		if (!taskid.startsWith("LC7_") && !taskid.startsWith("(LC7) "))
		{
			CoTaskMemFree(lpwszNames[0]);
			CoTaskMemFree(lpwszNames);
			continue;
		}
		if(taskid.startsWith("("))
			taskid = taskid.mid(6);
		else
			taskid = taskid.mid(4);

		if (taskid.endsWith(".job"))
		{
			taskid = taskid.left(taskid.length() - 4);
		}
		
		ITask *pITask;
		hr = pITS->Activate(lpwszNames[0], IID_ITask, (IUnknown **)&pITask);
		CoTaskMemFree(lpwszNames[0]);
		CoTaskMemFree(lpwszNames);
		if (FAILED(hr))
		{
			continue;
		}

		InternalTask *task = LoadTask(taskid, pITask);
		pITask->Release();
		if (!task)
		{
			continue;
		}
		
		m_tasks.append(task);
		m_tasks_by_id[taskid] = task;
	}

	pIEnum->Release();
	pITS->Release();

	m_tasklist_valid = true;
	return true;
}

bool CTaskScheduler_WIN32v1::GetCredentialFromUser(void **credential, QString &error, bool &cancelled)
{TR;
	cancelled = false;

	// Ask for password
	bool got_password = false;
	QString passwd;
	HANDLE hToken;
	while (!got_password)
	{
		if(!m_ctrl->GetGUILinkage()->AskForPassword("Enter Password", QString("To schedule this job to run later, you must enter the password for user '%1'").arg(m_username), passwd))
		{
			cancelled = true;
			return true;
		}
		
		QString username;
		QString domain;
		bool hasdomain = false;
		if (m_username.contains("\\"))
		{
			QStringList parts = m_username.split('\\');
			if (parts[0].size() > 0)
			{
				domain = parts[0];
				hasdomain = true;
			}
			username = parts[1];
		}
		else
		{
			username = m_username;
		}

		if (LogonUserW(username.toStdWString().c_str(), hasdomain?domain.toStdWString().c_str():NULL, passwd.toStdWString().c_str(), LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hToken))
		{
			got_password = true;
		}

		
	}
	
	CloseHandle(hToken);

	*credential = (void *)new QString(passwd);

	return true;
}

void CTaskScheduler_WIN32v1::ReleaseCredential(void *credential)
{TR;
	QString *ps = (QString *)credential;
	delete ps;
}


bool CTaskScheduler_WIN32v1::ScheduleTask(CLC7Task *task, QString appname, QString parameters, void *credential, QString &error)
{TR;
	if (!m_tasklist_valid)
	{
		error = "Task list is inaccessible. You may not have permission to perform this action.";
		return false;
	}

	if (IsTaskScheduled(task))
	{
		error = "Task is already added";
		return false;
	}

	// Get password from opaque credential
	QString passwd = *(QString *)credential;

	// create task scheduler object

	ITaskScheduler *pITS;

	HRESULT hr = CoCreateInstance(CLSID_CTaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskScheduler,
		(void **)&pITS);
	if (FAILED(hr))
	{
		error = "Couldn't access task scheduler";
		return false;
	}
	AutoRelease arITS(pITS);

	ITask *pITask;
	hr = pITS->NewWorkItem(QString("LC7_%1").arg(task->GetId()).toStdWString().c_str(), CLSID_CTask, IID_ITask, (IUnknown **)&pITask);
	if (FAILED(hr))
	{
		error = "Couldn't create system scheduler task";
		return false;
	}
	AutoRelease arTask(pITask);

	InternalTask *inttask = SetupTask(pITask, task, appname, parameters, passwd);
	if (!inttask)
	{
		error = "Couldn't set up system scheduler task";
		return false;
	}

	m_tasks.append(inttask);
	m_tasks_by_id[inttask->m_id] = inttask;

	return true;
}

bool CTaskScheduler_WIN32v1::UnscheduleTask(CLC7Task *task, QString &error)
{TR;
	InternalTask *inttask = m_tasks_by_id.value(task->GetId(), NULL);
	if (!inttask)
	{
		error = "Task no longer exists";
		return false;
	}
	if (!RemoveTask(inttask, error))
	{
		return false;
	}
	return true;
}

bool CTaskScheduler_WIN32v1::IsTaskScheduled(const CLC7Task *task)
{TR;
	return m_tasks_by_id.contains(task->GetId());
}

bool CTaskScheduler_WIN32v1::ValidateScheduledTask(CLC7Task *task)
{
	InternalTask *inttask = m_tasks_by_id.value(task->GetId(), NULL);
	if (!inttask)
	{
		return false;
	}
/*
	if (inttask->m_externally_modified)
	{
		task->SetExternallyModified(true);
		return true;
	}

	std::wstring name;
	if (!GetNameFromSchedulerName(inttask->m_scheduler_name, name) && task->GetName() != QString::fromStdWString(name))
	{
		task->SetName(QString::fromStdWString(name));
	}
*/
	task->SetStartTime(inttask->m_start_time);
	task->SetExpirationEnabled(inttask->m_expiration_enabled);
	if (inttask->m_expiration_enabled)
	{
		task->SetExpirationEnabled(true);
		task->SetExpirationDate(inttask->m_expiration_date);
	}

	task->SetRecurrence(inttask->m_recurrence);
	task->SetDailyRecurrence(inttask->m_daily_recurrence);
	task->SetWeeklyRecurrence(inttask->m_weekly_recurrence);
	task->SetEnabledWeekDaysBitMask(inttask->m_enabled_days_of_week);
	task->SetMonthlyMode(inttask->m_monthly_mode);
	task->SetSpecificDayOfMonth(inttask->m_specific_day_of_month);
	task->SetAbstractTiming(inttask->m_abstract_timing);
	task->SetAbstractDayOfWeek(inttask->m_abstract_day_of_week);
	task->SetEnabledMonthsBitMask(inttask->m_enabled_months);

	return true;
}

bool CTaskScheduler_WIN32v1::CommitTask(CLC7Task *task, void *credential, QString &error)
{TR;
	InternalTask *inttask = m_tasks_by_id.value(task->GetId(), NULL);
	if (!inttask)
	{
		error = "Task no longer exists";
		return false;
	}

	if (!UnscheduleTask(task, error))
	{
		return false;
	}

	bool cancelled = false;
	if (!ScheduleTask(task, inttask->m_appname, inttask->m_parameters, credential, error))
	{
		return false;
	}

	return true;
}

void CTaskScheduler_WIN32v1::PurgeInvalidTasks(QList<CLC7Task *> valid_tasks)
{TR;
	QSet<QString> valid_task_ids;
	foreach(CLC7Task *valid_task, valid_tasks)
	{
		valid_task_ids.insert(valid_task->GetId());
	}

	QList<InternalTask *> invalid_tasks;
	foreach(InternalTask *task, m_tasks)
	{
		if (!valid_task_ids.contains(task->m_id))
		{
			invalid_tasks.append(task);
		}
	}

	foreach(InternalTask *task, invalid_tasks)
	{
		QString error;
		RemoveTask(task, error);
		// ignore errors here, this is mostly for cleanup purposes, if it fails it's not the end of the world.
		// yes, we may come to regret this. only time will tell.
	}
}

