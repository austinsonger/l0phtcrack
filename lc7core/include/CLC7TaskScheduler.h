#ifndef __INC_CLC7TASKSCHEDULER_H
#define __INC_CLC7TASKSCHEDULER_H

class CLC7Controller;
class CLC7Task;
class ITaskScheduler_BASE;

class CLC7TaskScheduler: public ILC7TaskScheduler
{
public:
	CLC7TaskScheduler(CLC7Controller *ctrl);
	virtual ~CLC7TaskScheduler();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual bool Initialize(QString &error);
	virtual bool IsAvailable(); 

	virtual ILC7EditableTask *NewTask();
	virtual void DeleteTask(ILC7EditableTask *task);

	virtual void RefreshTasks(void);

	virtual ILC7Task *ScheduleTask(ILC7EditableTask *task, QString & error, bool &cancelled);
	virtual ILC7EditableTask *UnscheduleTask(ILC7Task *task, QString & error);

	virtual int GetScheduledTaskCount(void);
	virtual ILC7Task *GetScheduledTask(int pos);
	virtual ILC7Task *FindScheduledTaskById(QString taskid);

	virtual ILC7EditableTask *EditScheduledTask(ILC7Task *task);
	virtual bool CommitScheduledTask(ILC7EditableTask *task, QString &error, bool &cancelled);

	virtual bool NewSessionFromTask(ILC7Task *task);
	virtual bool RunTask(ILC7Task *itask, bool quit_on_finish);
	virtual bool IsTaskRunning(void);

	virtual void RegisterTaskFinishedCallback(QObject *callback_object, void (QObject::*callback_function)(ILC7Task *));
	virtual void UnregisterTaskFinishedCallback(QObject *callback_object, void (QObject::*callback_function)(ILC7Task *));
	virtual void CallTaskFinishedCallbacks(ILC7Task *finished_task);

protected:
	void ClearTasks(void);
	bool SaveTaskFile(CLC7Task *task, QString & taskpath, QString &error);

private:
	CLC7Controller *m_ctrl;
	bool m_bAvailable;

	QList<CLC7Task *> m_tasks;
	QMap<QString, CLC7Task *> m_tasks_by_id;
	QList<QPair<QObject *, void (QObject::*)(ILC7Task *)>> m_task_finished_callbacks;

	ITaskScheduler_BASE *m_sched;
};


#endif
