#ifndef __INC_ILC7TASKSCHEDULER_H
#define __INC_ILC7TASKSCHEDULER_H

#include"core/ILC7Interface.h"

class ILC7Task;
class ILC7EditableTask;

class ILC7TaskScheduler:public ILC7Interface
{
protected:
	virtual ~ILC7TaskScheduler() {}

public:
	virtual bool IsAvailable() = 0;

	virtual ILC7EditableTask *NewTask() = 0;
	virtual void DeleteTask(ILC7EditableTask *task) = 0;

	virtual void RefreshTasks(void) = 0;

	virtual ILC7Task *ScheduleTask(ILC7EditableTask *task, QString & error, bool &cancelled) = 0;
	virtual ILC7EditableTask *UnscheduleTask(ILC7Task *task, QString & error) = 0;

	virtual int GetScheduledTaskCount(void) = 0;
	virtual ILC7Task *GetScheduledTask(int pos) = 0;
	virtual ILC7Task *FindScheduledTaskById(QString taskid) = 0;

	virtual ILC7EditableTask *EditScheduledTask(ILC7Task *task) = 0;
	virtual bool CommitScheduledTask(ILC7EditableTask *task, QString &error, bool &cancelled) = 0;

	virtual bool NewSessionFromTask(ILC7Task *task) = 0;
	virtual bool RunTask(ILC7Task *itask, bool quit_on_finish) = 0;
	virtual bool IsTaskRunning(void) = 0;

	virtual void RegisterTaskFinishedCallback(QObject *callback_object, void (QObject::*callback_function)(ILC7Task *)) = 0;
	virtual void UnregisterTaskFinishedCallback(QObject *callback_object, void (QObject::*callback_function)(ILC7Task *)) = 0;

};

#endif
