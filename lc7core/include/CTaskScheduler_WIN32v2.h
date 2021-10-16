#ifndef __INC_TASK_SCHEDULER_WIN32_V2_H
#define __INC_TASK_SCHEDULER_WIN32_V2_H

#include "ITaskScheduler_BASE.h"

struct ITaskDefinition;
struct ITaskFolder;
struct IRegisteredTask;

class CTaskScheduler_WIN32v2:public ITaskScheduler_BASE
{
protected:

	struct InternalTask
	{
		InternalTask();

		QString m_id;
		std::wstring m_scheduler_name;
		QString m_description;
		QString m_run_as_user;

		bool m_externally_modified;

		QString m_appname;
		QString m_parameters;

		QDateTime m_start_time;
		bool m_expiration_enabled;
		QDate m_expiration_date;
		ILC7Task::RECURRENCE m_recurrence;
		int m_daily_recurrence;
		int m_weekly_recurrence;
		quint32 m_enabled_days_of_week;
		ILC7Task::MONTHLY_MODE m_monthly_mode;
		int m_specific_day_of_month;
		ILC7Task::TIMING m_abstract_timing;
		ILC7Task::DAY_OF_WEEK m_abstract_day_of_week;
		quint32 m_enabled_months;
	};

	QString m_username;
	QList<InternalTask *> m_tasks;
	QMap<QString, InternalTask *> m_tasks_by_id;
	bool m_tasklist_valid;
	CLC7Controller *m_ctrl;

	InternalTask *SetupTask(ITaskDefinition *pTaskDefinition, ITaskFolder *pFolder, CLC7Task *task, QString appname, QString parameters, QString passwd, QString &error);
	InternalTask *LoadTask(QString taskid, IRegisteredTask *pTask, QString &error);
	bool SetupTrigger(CLC7Task *lc7task, InternalTask *task, ITaskDefinition *pTaskDefinition, QString &error);
	bool RemoveTask(InternalTask *task, QString &error);
	void ClearTasks();

public:
	CTaskScheduler_WIN32v2(CLC7Controller *ctrl);
	virtual ~CTaskScheduler_WIN32v2();
	
	virtual bool Startup(QString & error);
	virtual bool RefreshScheduledTasks(QString &error);

	virtual bool GetCredentialFromUser(void **credential, QString &error, bool & cancelled);
	virtual void ReleaseCredential(void *credential);

	virtual bool ScheduleTask(CLC7Task *task, QString appname, QString parameters, void *credential, QString &error);
	virtual bool UnscheduleTask(CLC7Task *task, QString &error);
	virtual bool CommitTask(CLC7Task *task, void *credential, QString &error);
	virtual bool IsTaskScheduled(const CLC7Task *task);
	virtual bool ValidateScheduledTask(CLC7Task *task);
	virtual void PurgeInvalidTasks(QList<CLC7Task *> valid_tasks);
};

#endif