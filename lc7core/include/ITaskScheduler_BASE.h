#ifndef __INC_ITASKSCHEDULER_BASE_H
#define __INC_ITASKSCHEDULER_BASE_H

class CLC7Task;

class ITaskScheduler_BASE
{

public:

	virtual ~ITaskScheduler_BASE() {};

	virtual bool Startup(QString & error) = 0;
	virtual bool RefreshScheduledTasks(QString &error) = 0;

	virtual bool GetCredentialFromUser(void **credential, QString &error, bool & cancelled)=0;
	virtual void ReleaseCredential(void *credential)=0;

	virtual bool ScheduleTask(CLC7Task *task, QString appname, QString parameters, void *credential, QString &error) = 0;
	virtual bool UnscheduleTask(CLC7Task *task, QString &error) = 0;
	virtual bool CommitTask(CLC7Task *task, void *credential, QString &error) = 0;
	virtual bool IsTaskScheduled(const CLC7Task *task) = 0;
	virtual bool ValidateScheduledTask(CLC7Task *task) = 0;
	virtual void PurgeInvalidTasks(QList<CLC7Task *> valid_tasks) = 0;
};

#endif