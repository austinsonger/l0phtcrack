#ifndef __INC_ILC7WORKQUEUE_H
#define __INC_ILC7WORKQUEUE_H

#include"core/ILC7Interface.h"

#include<quuid.h>
#include<qstring.h>
#include<qmap.h>
#include<qvariant.h>

class LC7WorkQueueItem
{
protected:
	
	QUuid m_component_id;
	QString m_command;
	QStringList m_args;
	QMap<QString,QVariant> m_config;
	QString m_description;
	bool m_enable_stop;
	bool m_enable_pause;

public:

	LC7WorkQueueItem()
	{
	}

	LC7WorkQueueItem(QUuid component_id, QString command, QStringList args, QMap<QString, QVariant> config, QString description, bool enable_stop, bool enable_pause):
		m_component_id(component_id),
		m_command(command),
		m_args(args),
		m_config(config),
		m_description(description),
		m_enable_stop(enable_stop),
		m_enable_pause(enable_pause)
	{
	}

	QUuid GetComponentId()
	{
		return m_component_id;
	}
	QString GetCommand()
	{
		return m_command;
	}
	QStringList GetArgs()
	{
		return m_args;
	}
	QMap<QString, QVariant> GetConfig()
	{
		return m_config;
	}
	QString GetDescription()
	{
		return m_description;
	}
	bool GetEnableStop()
	{
		return m_enable_stop;
	}
	bool GetEnablePause()
	{
		return m_enable_pause;
	}
};

class ILC7WorkQueue: public ILC7SessionHandler
{

public:
	enum STATE
	{
		UNVALIDATED=0,
		VALIDATED,
		INVALID,
		IN_PROGRESS,
		STOPPED,
		PAUSED,
		COMPLETE,
		FAIL
	};


protected:
	
	virtual ~ILC7WorkQueue() {}

public:
	
	virtual void AddQueueChangedListener(QObject *slot_recv, void (QObject::*slot_method)(void))=0;
	virtual void AddQueueStateChangedListener(QObject *slot_recv, void (QObject::*slot_method)(void))=0;

	virtual void AppendWorkQueueItem(LC7WorkQueueItem item)=0;
	virtual void RemoveWorkQueueItem(int n)=0;
	virtual void ClearWorkQueue(void)=0;

	virtual void SwapWorkQueueItem(int from, int to)=0;
	
	virtual int GetWorkQueueItemCount()=0;
	virtual LC7WorkQueueItem GetWorkQueueItemAt(int i)=0;
	
	virtual void ResetWorkQueueState(void)=0;
	virtual STATE GetWorkQueueItemStateAt(int i)=0;
	virtual STATE GetWorkQueueState(void)=0;

	virtual bool Validate(QString & error, int & failed_item)=0;
//	virtual bool IsValidated(void)=0;

	virtual bool IsStopEnabled(void)=0;
	virtual bool IsPauseEnabled(void)=0;
	virtual bool StartRequest(void)=0;
	virtual bool StopRequest(void)=0;
	virtual bool PauseRequest(void)=0;
	virtual bool IsStopRequested(void)=0;
	virtual bool IsPauseRequested(void)=0;
	virtual bool IsThermalShutdown(void)=0;

	virtual QString GetLastError(void)=0;

	virtual void RegisterUIEnable(QWidget *widget)=0;
	
	virtual bool IsCheckpointed(void)=0;
	virtual void ResumeFromCheckpoint(void)=0;
	virtual void SaveCheckpointConfig(QMap<QString, QVariant> checkpoint_config)=0;
		
	// ILC7SessionHandler
	virtual QUuid GetId()=0;

	virtual void Acquire()=0;
	virtual void Release()=0;

	virtual bool Save(QDataStream & out)=0;
	virtual bool Load(QDataStream & in)=0;
};

#define SINGLE_WORKQUEUE_HANDLER_ID (QUuid("{d203295d-a318-4272-9070-a46a755634ae}"))
#define BATCH_WORKQUEUE_HANDLER_ID (QUuid("{d4f9b151-8540-4057-8eb5-277df5b3b7b3}"))

#endif
