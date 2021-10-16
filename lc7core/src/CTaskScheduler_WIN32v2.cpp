#include<stdafx.h>
#include<string>
#include<iostream>

//#define SECURITY_KERNEL
#define SECURITY_WIN32
//#define SECURITY_MAC
#include<Windows.h>
#include <security.h>
#include <secext.h>

#include <comdef.h>
#include <wincred.h>
//  Include the task header file.
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

// service stuff
#include <winsvc.h>


#define CHECK_HR(x,MSG) {HRESULT hr=(x); if (FAILED(hr)) { error=QString(MSG ": %1").arg(hr); return false; }}
#define CHECK_HR_NULL(x,MSG) {HRESULT hr=(x); if (FAILED(hr)) { error=QString(MSG ": %1").arg(hr); return nullptr; }}

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

class AutoReleaseBSTR
{
	BSTR m_ptr;
public:
	AutoReleaseBSTR(BSTR ptr)
	{
		m_ptr = ptr;
	}
	~AutoReleaseBSTR()
	{
		SysFreeString(m_ptr);
	}
};

class AutoReleaseDelete
{
	void *m_ptr;
	bool do_delete;
public:
	AutoReleaseDelete(void *ptr):do_delete(true), m_ptr(ptr)
	{
	}
	
	void Escape()
	{
		do_delete = false;
	}

	~AutoReleaseDelete()
	{
		if (do_delete)
		{
			delete m_ptr;
		}
	}
};


static std::wstring GetSchedulerName(CLC7Task *task)
{
	QString taskname = task->GetName();
	QString filtered_taskname;
	for (auto x : taskname)
	{
		if (iscntrl(x.toLatin1()))
			continue;
		if (QString("%*?:;|\\/<>~$").contains(x))
		{
			continue;
		}
		filtered_taskname += x;
	}
	return QString("%1(LC7_%2)").arg(filtered_taskname).arg(task->GetId()).toStdWString();
}

static bool GetTaskIdFromSchedulerName(std::wstring name, std::wstring &taskid)
{
	size_t len = name.size();
	if (name[len - 1] != L')')
		return false;
	size_t lparen = name.rfind(L'(');
	if (lparen == std::wstring::npos)
	{
		return false;
	}
	if (name.substr(lparen, 5) != L"(LC7_")
	{
		return false;
	}
	taskid = name.substr(lparen + 5, len - 1 - (lparen + 5));
	return true;
}

static bool GetNameFromSchedulerName(std::wstring name, std::wstring &taskname)
{
	size_t len = name.size();
	if (name[len - 1] != L')')
		return false;
	size_t lparen = name.rfind(L'(');
	if (lparen == std::wstring::npos)
	{
		return false;
	}
	if (name.substr(lparen, 5) != L"(LC7_")
	{
		return false;
	}
	taskname = name.substr(0, lparen);
	return true;
}

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


CTaskScheduler_WIN32v2::InternalTask::InternalTask()
{TR;
	m_id="";
	m_expiration_enabled=false;
	
	m_externally_modified = false;
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

CTaskScheduler_WIN32v2::CTaskScheduler_WIN32v2(CLC7Controller *ctrl)
{TR;
	m_ctrl = ctrl;
	m_tasklist_valid = false;
}

CTaskScheduler_WIN32v2::~CTaskScheduler_WIN32v2()
{TR;
}


bool CTaskScheduler_WIN32v2::Startup(QString & error)
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

bool CTaskScheduler_WIN32v2::RemoveTask(InternalTask *task, QString &error)
{TR;
	// create task scheduler object
	ITaskService *pITS = nullptr;
	CHECK_HR(CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void **)&pITS), "Couldn't instantiate task scheduler");
	AutoRelease arITS(pITS);

	// Connect to the task service
	CHECK_HR(pITS->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()), "Couldn't connect to task scheduler");

	// Get the folder
	ITaskFolder *pFolder = nullptr;
	CHECK_HR(pITS->GetFolder(_bstr_t(L"\\L0pht Holdings LLC\\L0phtCrack 7"), &pFolder), "Couldn't get task scheduler folder");
	AutoRelease arFolder(pFolder);

	CHECK_HR(pFolder->DeleteTask(_bstr_t(task->m_scheduler_name.c_str()), 0), "Couldn't delete task");
	
	m_tasks.removeAll(task);
	m_tasks_by_id.remove(task->m_id);

	return true;
}


void CTaskScheduler_WIN32v2::ClearTasks()
{TR;
	foreach(InternalTask *task, m_tasks)
	{
		delete task;
	}
	m_tasks.clear();
	m_tasks_by_id.clear();
	m_tasklist_valid = false;
}

bool CTaskScheduler_WIN32v2::SetupTrigger(CLC7Task *lc7task, CTaskScheduler_WIN32v2::InternalTask *task, ITaskDefinition *pTaskDefinition, QString &error)
{TR;
	
	//  Get the trigger collection to insert the trigger
	ITriggerCollection *pTriggerCollection = nullptr;
	CHECK_HR(pTaskDefinition->get_Triggers(&pTriggerCollection), "Unable to get trigger collection");
	AutoRelease ar_triggerCollection(pTriggerCollection);

	// Recurrence Type
	task->m_recurrence = lc7task->GetRecurrence();
	
	TASK_TRIGGER_TYPE2 triggerType;
	switch (task->m_recurrence)
	{
	case ILC7Task::ONE_TIME:
		triggerType = TASK_TRIGGER_TIME;
		break;

	case ILC7Task::DAILY:
		triggerType = TASK_TRIGGER_DAILY;
		task->m_daily_recurrence = lc7task->GetDailyRecurrence();
		break;

	case ILC7Task::WEEKLY:
		triggerType = TASK_TRIGGER_WEEKLY;
		task->m_weekly_recurrence = lc7task->GetWeeklyRecurrence();
		task->m_enabled_days_of_week = lc7task->GetEnabledWeekDaysBitMask();
		break;

	case ILC7Task::MONTHLY:
		task->m_monthly_mode = lc7task->GetMonthlyMode();
		task->m_enabled_months = lc7task->GetEnabledMonthsBitMask();
		if (task->m_monthly_mode == ILC7Task::SPECIFIC_DAY)
		{
			triggerType = TASK_TRIGGER_MONTHLY;
			task->m_specific_day_of_month = lc7task->GetSpecificDayOfMonth();
		}
		else
		{
			triggerType = TASK_TRIGGER_MONTHLYDOW;
			task->m_abstract_timing = lc7task->GetAbstractTiming();
			task->m_abstract_day_of_week = lc7task->GetAbstractDayOfWeek();
		}
		break;
	}

	// Create trigger
	ITrigger *pTrigger;
	CHECK_HR(pTriggerCollection->Create(triggerType, &pTrigger), "Unable to create trigger");
	AutoRelease ar_trigger(pTrigger);

	CHECK_HR(pTrigger->put_Id(_bstr_t(L"Trigger1")), "Unable to set trigger id");

	// Start Time
	task->m_start_time = lc7task->GetStartTime();
	QString starttimestr = task->m_start_time.toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
	pTrigger->put_StartBoundary(_bstr_t(starttimestr.toStdWString().c_str()));
	
	// Expiration Date
	task->m_expiration_enabled = lc7task->GetExpirationEnabled();
	task->m_expiration_date = lc7task->GetExpirationDate();
	if (task->m_expiration_enabled)
	{
		QString endtimestr = QDateTime(task->m_expiration_date,QTime(),QTimeZone::systemTimeZone()).toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
		pTrigger->put_EndBoundary(_bstr_t(endtimestr.toStdWString().c_str()));
	}

	// Recurrence Type
	switch (task->m_recurrence)
	{
	case ILC7Task::ONE_TIME:
		break;

	case ILC7Task::DAILY:
	{
		IDailyTrigger *pDailyTrigger = nullptr;
		CHECK_HR(pTrigger->QueryInterface(IID_IDailyTrigger, (void**)&pDailyTrigger), "Unable to query interface of daily trigger");
		AutoRelease ar_dailyTrigger(pDailyTrigger);
		CHECK_HR(pDailyTrigger->put_DaysInterval((short)task->m_daily_recurrence), "Unable to set days interval");
	}
		break;

	case ILC7Task::WEEKLY:
	{
		IWeeklyTrigger *pWeeklyTrigger = nullptr;
		CHECK_HR(pTrigger->QueryInterface(IID_IWeeklyTrigger, (void**)&pWeeklyTrigger), "Unable to query interface of weekly trigger");
		AutoRelease ar_weeklyTrigger(pWeeklyTrigger);
		CHECK_HR(pWeeklyTrigger->put_WeeksInterval((short)task->m_weekly_recurrence), "Unable to set weeks interval");
		CHECK_HR(pWeeklyTrigger->put_DaysOfWeek((short)task->m_enabled_days_of_week), "Unable to set days of week");
	}
		break;

	case ILC7Task::MONTHLY:
		if (task->m_monthly_mode == ILC7Task::SPECIFIC_DAY)
		{
			IMonthlyTrigger *pMonthlyTrigger = nullptr;
			CHECK_HR(pTrigger->QueryInterface(IID_IMonthlyTrigger, (void**)&pMonthlyTrigger), "Unable to query interface of monthly trigger");
			AutoRelease ar_monthlyTrigger(pMonthlyTrigger);
		
			CHECK_HR(pMonthlyTrigger->put_MonthsOfYear((short)task->m_enabled_months), "Unable to set months of year");
			CHECK_HR(pMonthlyTrigger->put_DaysOfMonth((long)1 << (task->m_specific_day_of_month - 1)), "Unable to set days of month");
		}
		else
		{
			IMonthlyDOWTrigger *pMonthlyDOWTrigger = nullptr;
			CHECK_HR(pTrigger->QueryInterface(IID_IMonthlyDOWTrigger, (void**)&pMonthlyDOWTrigger), "Unable to query interface of monthly-day-of-week trigger");
			AutoRelease ar_monthlyTrigger(pMonthlyDOWTrigger);

			CHECK_HR(pMonthlyDOWTrigger->put_DaysOfWeek((short)1<<((int)task->m_abstract_day_of_week)), "Unable to set days of week");
			CHECK_HR(pMonthlyDOWTrigger->put_WeeksOfMonth((short)1<<(int)(task->m_abstract_timing)), "Unable to set weeks of month");
			CHECK_HR(pMonthlyDOWTrigger->put_MonthsOfYear((short)task->m_enabled_months), "Unable to set months of year");
		}
		break;
	}

	return true;
}


CTaskScheduler_WIN32v2::InternalTask *CTaskScheduler_WIN32v2::SetupTask(ITaskDefinition *pTaskDefinition, ITaskFolder *pFolder, CLC7Task *lc7task, QString appname, QString parameters, QString passwd, QString &error)
{TR;
	InternalTask *task = new InternalTask();
	AutoReleaseDelete ar_task(task);

	task->m_id = lc7task->GetId();

	//  Get the registration info for setting the identification.
	IRegistrationInfo *pRegInfo = nullptr;
	CHECK_HR_NULL(pTaskDefinition->get_RegistrationInfo(&pRegInfo), "Cannot get registration info pointer");
	AutoRelease ar_regInfo(pRegInfo);

	CHECK_HR_NULL(pRegInfo->put_Author(L"L0phtCrack 7"), "Cannot put Author into registration info");
	
	task->m_description = lc7task->GetTaskDescriptions().join("\n");
	CHECK_HR_NULL(pRegInfo->put_Description(_bstr_t(task->m_description.toStdWString().c_str())), "Cannot put description");

	//  Create the principal for the task - these credentials
	//  are overwritten with the credentials passed to RegisterTaskDefinition
	IPrincipal *pPrincipal = nullptr;
	CHECK_HR_NULL(pTaskDefinition->get_Principal(&pPrincipal),"Cannot get principal pointer");
	AutoRelease ar_principal(pPrincipal);

	// Set up principal logon type to use password
	CHECK_HR_NULL(pPrincipal->put_LogonType(TASK_LOGON_PASSWORD), "Cannot put logon type");
	
	// ------------------------------------------------------
	// Add an action to the task. 
	// Command line
	QString apppath = QDir::toNativeSeparators(QFileInfo(QDir::fromNativeSeparators(appname)).absoluteDir().absolutePath());
	task->m_appname = appname;
	task->m_parameters = parameters;
	IActionCollection *pActionCollection = nullptr;

	// Get the task action collection pointer.
	CHECK_HR_NULL(pTaskDefinition->get_Actions(&pActionCollection), "Unable to get actions collection");
	AutoRelease ar_actionCollection(pActionCollection);

	//  Create the action, specifying that it is an executable action.
	IAction *pAction = nullptr;
	CHECK_HR_NULL(pActionCollection->Create(TASK_ACTION_EXEC, &pAction), "Unable to create exec action");
	AutoRelease ar_action(pAction);

	IExecAction *pExecAction = nullptr;
	CHECK_HR_NULL(pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction), "Unable to query interface of exec action");
	AutoRelease ar_execAction(pExecAction);
	
	// Set the path
	CHECK_HR_NULL(pExecAction->put_Path(_bstr_t(appname.toStdWString().c_str())), "Unable to put path to exec action");
	CHECK_HR_NULL(pExecAction->put_Arguments(_bstr_t(parameters.toStdWString().c_str())), "Unable to put parameters to exec action");
	CHECK_HR_NULL(pExecAction->put_WorkingDirectory(_bstr_t(apppath.toStdWString().c_str())), "Unable to put working directory to exec action");
	   	
	// Set up trigger
	if (!SetupTrigger(lc7task, task, pTaskDefinition, error)) 
	{
		return false;
	}

	// Set settings
	ITaskSettings *pSettings;
	CHECK_HR_NULL(pTaskDefinition->get_Settings(&pSettings), "Unable to get task settings");
	AutoRelease ar_settings(pSettings);

	// Set time limit to unlimited
	CHECK_HR_NULL(pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S")), "Unable to set time limit");
	
	// ------------------------------------------------------
	// Register task definition

	// Run as user
	task->m_run_as_user = m_username;

	// Get task name
	std::wstring taskname(GetSchedulerName(lc7task));
	task->m_scheduler_name = taskname;

	// Save the task in the folder.
	IRegisteredTask *pRegisteredTask = nullptr;
	CHECK_HR_NULL(pFolder->RegisterTaskDefinition(
		_bstr_t(taskname.c_str()),
		pTaskDefinition,
		TASK_CREATE_OR_UPDATE,
		_variant_t(_bstr_t(m_username.toStdWString().c_str())),
		_variant_t(_bstr_t(passwd.toStdWString().c_str())),
		TASK_LOGON_PASSWORD,
		_variant_t(L""),
		&pRegisteredTask), "Unable to register task definition");
	AutoRelease ar_registeredTask(pRegisteredTask);
	
	ar_task.Escape();
	return task;
}


CTaskScheduler_WIN32v2::InternalTask *CTaskScheduler_WIN32v2::LoadTask(QString taskid, IRegisteredTask *pTask, QString &error)
{TR;
	ITaskDefinition *pTaskDefinition;
	CHECK_HR_NULL(pTask->get_Definition(&pTaskDefinition), "Unable to get task definition");
	AutoRelease arTaskDefinition(pTaskDefinition);

	InternalTask *task = new InternalTask();
	AutoReleaseDelete ar_task(task);

	task->m_id = taskid;
	
	IRegistrationInfo *pRegInfo = nullptr;
	CHECK_HR_NULL(pTaskDefinition->get_RegistrationInfo(&pRegInfo), "Cannot get registration info pointer");
	AutoRelease ar_regInfo(pRegInfo);

	LPWSTR description;
	CHECK_HR_NULL(pRegInfo->get_Description(&description), "Unable to get description");
	if (!description)
	{
		return nullptr;
	}
	AutoReleaseBSTR ar_description(description);
	task->m_description = QString::fromWCharArray(description);

	IPrincipal *pPrincipal;
	CHECK_HR_NULL(pTaskDefinition->get_Principal(&pPrincipal), "Unable to get principal");
	AutoRelease arPrincipal(pPrincipal);

	LPWSTR userid;
	CHECK_HR_NULL(pPrincipal->get_UserId(&userid), "Unable to get user id");
	if (!userid)
	{
		return nullptr;
	}
	AutoReleaseBSTR ar_userid(userid);
	task->m_run_as_user = QString::fromWCharArray(userid);
			
	ITriggerCollection *pAllTriggers;
	CHECK_HR_NULL(pTaskDefinition->get_Triggers(&pAllTriggers), "Unable to get trigger collection");
	AutoRelease arAllTriggers(pAllTriggers);
	LONG triggerCount=0;
	CHECK_HR_NULL(pAllTriggers->get_Count(&triggerCount), "Couldn't get trigger count");

	IActionCollection *pAllActions;
	CHECK_HR_NULL(pTaskDefinition->get_Actions(&pAllActions), "Unable to get action collection");
	AutoRelease arAllActions(pAllActions);
	LONG actionCount=0;
	CHECK_HR_NULL(pAllActions->get_Count(&actionCount), "Couldn't get action count");
		
	if(triggerCount!=1 || actionCount!=1)
	{
		task->m_externally_modified = true;
		ar_task.Escape();
		return task;
	}

	IAction *pAction;
	CHECK_HR_NULL(pAllActions->get_Item(1, &pAction), "Couldn't get action");
	AutoRelease arAction(pAction);
	
	TASK_ACTION_TYPE tatype;
	CHECK_HR_NULL(pAction->get_Type(&tatype), "Couldn't get task action type");
	if (tatype != TASK_ACTION_EXEC)
	{
		task->m_externally_modified = true;
		ar_task.Escape();
		return task;
	}

	IExecAction *pExecAction = nullptr;
	CHECK_HR_NULL(pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction), "Unable to query interface of exec action");
	AutoRelease ar_execAction(pExecAction);

	// Get the command line
	LPWSTR application_name;
	LPWSTR parameters;
	LPWSTR working_directory;
	CHECK_HR_NULL(pExecAction->get_Path(&application_name), "Unable to get path to exec action");
	if (!application_name)
	{
		return nullptr;
	}
	AutoReleaseBSTR arapplication_name(application_name);
	CHECK_HR_NULL(pExecAction->get_Arguments(&parameters), "Unable to get parameters to exec action");
	if (!parameters)
	{
		return nullptr;
	}
	AutoReleaseBSTR arparameters(parameters);
	CHECK_HR_NULL(pExecAction->get_WorkingDirectory(&working_directory), "Unable to get working directory to exec action");
	if (!working_directory)
	{
		return nullptr;
	}
	AutoReleaseBSTR arworking_directory(working_directory);

	// Verify working directory
	task->m_appname = QString::fromWCharArray(application_name);
	task->m_parameters = QString::fromWCharArray(parameters);
	QString apppath = QDir::toNativeSeparators(QFileInfo(QDir::fromNativeSeparators(task->m_appname)).absoluteDir().absolutePath());
	if (apppath != QString::fromWCharArray(working_directory))
	{
		task->m_externally_modified = true;
		ar_task.Escape();
		return task;
	}
	
	// Get trigger
	ITrigger *pTrigger;
	CHECK_HR_NULL(pAllTriggers->get_Item(1, &pTrigger), "Couldn't get trigger");
	AutoRelease arTrigger(pTrigger);
	   
	// Start Time
	LPWSTR startboundary;
	CHECK_HR_NULL(pTrigger->get_StartBoundary(&startboundary), "Couldn't get start boundary");
	if (!startboundary)
	{
		return nullptr;
	}
	AutoReleaseBSTR arStartBoundary(startboundary);
	task->m_start_time = QDateTime::fromString(QString::fromWCharArray(startboundary), Qt::ISODate);

	LPWSTR endboundary=nullptr;
	HRESULT hr = pTrigger->get_EndBoundary(&endboundary);
	if (!FAILED(hr) && endboundary)
	{
		AutoReleaseBSTR arEndBoundary(endboundary);
		QDateTime dt = QDateTime::fromString(QString::fromWCharArray(endboundary), Qt::ISODate);
		if (dt.isValid())
		{
			task->m_expiration_enabled = true;
			task->m_expiration_date = dt.date();
		}
	}

	// Recurrence Type
	TASK_TRIGGER_TYPE2 tttype;
	CHECK_HR_NULL(pTrigger->get_Type(&tttype), "Couldn't get task trigger type");
	   
	switch (tttype)
	{
	case TASK_TRIGGER_TIME:
		{
			task->m_recurrence = ILC7Task::ONE_TIME;
		}
		break;
	case TASK_TRIGGER_DAILY:
		{
			IDailyTrigger *pDailyTrigger = nullptr;
			CHECK_HR(pTrigger->QueryInterface(IID_IDailyTrigger, (void**)&pDailyTrigger), "Unable to query interface of daily trigger");
			AutoRelease ar_dailyTrigger(pDailyTrigger);
			short daysinterval;
			CHECK_HR(pDailyTrigger->get_DaysInterval(&daysinterval), "Unable to get days interval");

			task->m_daily_recurrence = daysinterval;
		}
		break;
	case TASK_TRIGGER_WEEKLY:
		{
			IWeeklyTrigger *pWeeklyTrigger = nullptr;
			CHECK_HR(pTrigger->QueryInterface(IID_IWeeklyTrigger, (void**)&pWeeklyTrigger), "Unable to query interface of weekly trigger");
			AutoRelease ar_weeklyTrigger(pWeeklyTrigger);
			short weeks_interval;
			CHECK_HR(pWeeklyTrigger->get_WeeksInterval(&weeks_interval), "Unable to get weeks interval");
			task->m_weekly_recurrence = weeks_interval;
			short enabled_days_of_week;
			CHECK_HR(pWeeklyTrigger->get_DaysOfWeek(&enabled_days_of_week), "Unable to get days of week");
			task->m_enabled_days_of_week = enabled_days_of_week;
		}
		break;
	case TASK_TRIGGER_MONTHLY:
		{
			IMonthlyTrigger *pMonthlyTrigger = nullptr;
			CHECK_HR(pTrigger->QueryInterface(IID_IMonthlyTrigger, (void**)&pMonthlyTrigger), "Unable to query interface of monthly trigger");
			AutoRelease ar_monthlyTrigger(pMonthlyTrigger);

			short enabled_months;
			CHECK_HR(pMonthlyTrigger->get_MonthsOfYear(&enabled_months), "Unable to get months of year");
			task->m_enabled_months = enabled_months;

			long days_of_month;
			CHECK_HR(pMonthlyTrigger->get_DaysOfMonth(&days_of_month), "Unable to get days of month");

			DWORD d = days_of_month;
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
						task->m_externally_modified = true;
						ar_task.Escape();
						return task;
					}
				}
			}
			if (day == 0)
			{
				task->m_externally_modified = true;
				ar_task.Escape();
				return task;
			}
			task->m_specific_day_of_month = day;
		}
		break;
	case TASK_TRIGGER_MONTHLYDOW:
		{
			IMonthlyDOWTrigger *pMonthlyDOWTrigger = nullptr;
			CHECK_HR(pTrigger->QueryInterface(IID_IMonthlyDOWTrigger, (void**)&pMonthlyDOWTrigger), "Unable to query interface of monthly-day-of-week trigger");
			AutoRelease ar_monthlyTrigger(pMonthlyDOWTrigger);

			short days_of_week;
			CHECK_HR(pMonthlyDOWTrigger->get_DaysOfWeek(&days_of_week), "Unable to get days of week");

			DWORD d = (DWORD)days_of_week;
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
						task->m_externally_modified = true;
						ar_task.Escape();
						return task;
					}
				}
			}
			if (day == -1)
			{
				task->m_externally_modified = true;
				ar_task.Escape();
				return task;
			}
			task->m_abstract_day_of_week = (ILC7Task::DAY_OF_WEEK)day;

			short weeks_of_month;
			CHECK_HR(pMonthlyDOWTrigger->get_WeeksOfMonth(&weeks_of_month), "Unable to get weeks of month");
			DWORD w = (DWORD)weeks_of_month;
			int week = -1;
			for (int x = 0; x < 5; x++)
			{
				if (w & (1 << x))
				{
					if (week == -1)
					{
						week = x;
					}
					else
					{
						task->m_externally_modified = true;
						ar_task.Escape();
						return task;
					}
				}
			}
			if (week == -1)
			{
				task->m_externally_modified = true;
				ar_task.Escape();
				return task;
			}

			task->m_abstract_timing = (ILC7Task::TIMING)week;

			short enabled_months;
			CHECK_HR(pMonthlyDOWTrigger->get_MonthsOfYear(&enabled_months), "Unable to set months of year");	
			task->m_enabled_months = enabled_months;
		}
		break;

	default:
		task->m_externally_modified = true;
		ar_task.Escape();
		return task;
	}

	ar_task.Escape();
	return task;
}

bool CTaskScheduler_WIN32v2::RefreshScheduledTasks(QString &error)
{TR;
	ClearTasks();

	// create task scheduler object
	ITaskService *pITS = nullptr;
	CHECK_HR(CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void **)&pITS), "Couldn't instantiate task scheduler");
	AutoRelease arITS(pITS);

	// Connect to the task service
	CHECK_HR(pITS->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()), "Couldn't connect to task scheduler");

	// Get the folder
	ITaskFolder *pFolder = nullptr;
	HRESULT hr = pITS->GetFolder(_bstr_t(L"\\L0pht Holdings LLC\\L0phtCrack 7"), &pFolder);
	if (hr != NO_ERROR)
	{
		// No folder, no tasks
		m_tasklist_valid = true;
		return true;
	}
	AutoRelease arFolder(pFolder);
	
	// Get task collection
	IRegisteredTaskCollection *pAllTasks;
	CHECK_HR(pFolder->GetTasks(0, &pAllTasks), "Couldn't get tasks in folder");
	AutoRelease arAllTasks(pAllTasks);

	LONG count;
	CHECK_HR(pAllTasks->get_Count(&count), "Unable to count tasks in folder");

	for(LONG i=1;i<=count;i++)
	{
		IRegisteredTask *pTask;
		CHECK_HR(pAllTasks->get_Item(_variant_t(i), &pTask), "Unable to get task from folder");
			   
		LPWSTR name;
		CHECK_HR(pTask->get_Name(&name), "Unable to get task name");
		AutoReleaseBSTR ar_name(name);

		std::wstring taskidstr;
		if (!GetTaskIdFromSchedulerName(name, taskidstr))
		{
			continue;
		}
		QString taskid = QString::fromStdWString(taskidstr);
		
		InternalTask *task = LoadTask(taskid, pTask, error);
		if (!task)
		{
			continue;
		}
		task->m_scheduler_name = name;
		
		m_tasks.append(task);
		m_tasks_by_id[taskid] = task;
	}

	m_tasklist_valid = true;
	return true;
}

bool CTaskScheduler_WIN32v2::GetCredentialFromUser(void **credential, QString &error, bool &cancelled)
{TR;

	cancelled = false;

	// Ask for password
	bool got_password = false;
	QString passwd;
	HANDLE hToken;
	while (!got_password)
	{
		if (!m_ctrl->GetGUILinkage()->AskForUsernameAndPassword("Enter Credentials", QString("Specify the credentials under which to run the scheduled job"), m_username, passwd))
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

		if (LogonUserW(username.toStdWString().c_str(), hasdomain ? domain.toStdWString().c_str() : NULL, passwd.toStdWString().c_str(), LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hToken))
		{
			got_password = true;
		}
	}

	CloseHandle(hToken);

	*credential = (void *)new QString(passwd);

	return true;
}

void CTaskScheduler_WIN32v2::ReleaseCredential(void *credential)
{TR;
	QString *ps = (QString *)credential;
	delete ps;
}


bool CTaskScheduler_WIN32v2::ScheduleTask(CLC7Task *task, QString appname, QString parameters, void *credential, QString &error)
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
	ITaskService *pITS = nullptr;
	CHECK_HR(CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void **)&pITS), "Couldn't instantiate task scheduler");
	AutoRelease arITS(pITS);

	// Connect to the task service
	CHECK_HR(pITS->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()), "Couldn't connect to task scheduler");

	// Get the folder
	ITaskFolder *pFolder = nullptr;
	HRESULT hr = pITS->GetFolder(_bstr_t(L"\\L0pht Holdings LLC\\L0phtCrack 7"), &pFolder);
	if (hr != NO_ERROR)
	{
		ITaskFolder *pRootFolder = nullptr;
		CHECK_HR(pITS->GetFolder(_bstr_t(L"\\"), &pRootFolder), "Unable to get root folder");
		AutoRelease arRootFolder(pRootFolder);

		ITaskFolder *pL0phtFolder = nullptr;
		CHECK_HR(pRootFolder->CreateFolder(_bstr_t(L"L0pht Holdings LLC"), _variant_t(), &pL0phtFolder), "Unable to create L0pht Holdings LLC folder");
		AutoRelease arL0phtFolder(pL0phtFolder);

		CHECK_HR(pL0phtFolder->CreateFolder(_bstr_t(L"L0phtCrack 7"), _variant_t(), &pFolder), "Unable to create L0phtCrack 7 folder");
	}

	AutoRelease arFolder(pFolder);

	ITaskDefinition *pTaskDefinition = nullptr;
	CHECK_HR(pITS->NewTask(0, &pTaskDefinition), "Couldn't create system scheduler task");
	AutoRelease arTask(pTaskDefinition);

	InternalTask *inttask = SetupTask(pTaskDefinition, pFolder, task, appname, parameters, passwd, error);
	if (!inttask)
	{
		return false;
	}

	m_tasks.append(inttask);
	m_tasks_by_id[inttask->m_id] = inttask;

	return true;
}

bool CTaskScheduler_WIN32v2::UnscheduleTask(CLC7Task *task, QString &error)
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


bool CTaskScheduler_WIN32v2::IsTaskScheduled(const CLC7Task *task)
{
	TR;
	InternalTask *inttask = m_tasks_by_id.value(task->GetId(), NULL);
	if (!inttask)
	{
		return false;
	}
	return true;
};

bool CTaskScheduler_WIN32v2::ValidateScheduledTask(CLC7Task *task)
{TR;
	InternalTask *inttask = m_tasks_by_id.value(task->GetId(), NULL);
	if (!inttask)
	{
		return false;
	}

	if (inttask->m_externally_modified)
	{
		task->SetExternallyModified(true);
		return true;
	}

	std::wstring name;
	if (!GetNameFromSchedulerName(inttask->m_scheduler_name, name) && task->GetName()!=QString::fromStdWString(name))
	{
		task->SetName(QString::fromStdWString(name));
	}

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

bool CTaskScheduler_WIN32v2::CommitTask(CLC7Task *task, void *credential, QString &error)
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

void CTaskScheduler_WIN32v2::PurgeInvalidTasks(QList<CLC7Task *> valid_tasks)
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

