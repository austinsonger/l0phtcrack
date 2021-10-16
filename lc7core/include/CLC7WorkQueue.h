#ifndef __INC_CLC7WORKQUEUE_H
#define __INC_CLC7WORKQUEUE_H

#include"CLC7CommandControl.h"

class CLC7Controller;

class CLC7WorkQueue:public QThread, public ILC7WorkQueue
{
	Q_OBJECT

private:
	QUuid m_handler_id;
	QMutex m_acquire;
	CLC7Controller *m_controller;
	QVector<LC7WorkQueueItem> m_queueitems;
	QVector<STATE> m_queueitems_state;
	QMap<QString,QVariant> m_paused_config;
	QMap<QString, QVariant> m_checkpoint_config;
	STATE m_queue_state;
	QString m_error;
	bool m_stop_enabled;
	bool m_pause_enabled;
	bool m_stop_requested;
	bool m_pause_requested;
	bool m_thermal_shutdown;
	bool m_validate_limits;

	CLC7CommandControl m_commandctrl;

	void SetQueueState(STATE st);
	void SetQueueItemState(int n, STATE st);
	void ReportModified();

	void NotifyThermalTransition(ILC7ThermalWatchdog::THERMAL_STATE oldstate, ILC7ThermalWatchdog::THERMAL_STATE newstate);

	// Execution thread entry point
	virtual void run(void);

signals:
	void sig_NotifyQueueChanged(void);
	void sig_NotifyQueueStateChanged(void);
	void sig_uiEnable(bool);
	void sig_saveCheckpointConfig(QMap<QString, QVariant> checkpoint_config);

public slots:

	void slot_saveCheckpointConfig(QMap<QString, QVariant> checkpoint_config);

public:
	CLC7WorkQueue(QUuid handler_id, CLC7Controller *controller, bool validate_limits);
	virtual ~CLC7WorkQueue();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

public:

	virtual void AddQueueChangedListener(QObject *slot_recv, void (QObject::*slot_method)(void));
	virtual void AddQueueStateChangedListener(QObject *slot_recv, void (QObject::*slot_method)(void));
	
	virtual void AppendWorkQueueItem(LC7WorkQueueItem item);
	virtual void RemoveWorkQueueItem(int n);
	virtual void ClearWorkQueue(void);

	virtual void SwapWorkQueueItem(int from, int to);
	
	virtual int GetWorkQueueItemCount();
	virtual LC7WorkQueueItem GetWorkQueueItemAt(int i);
	
	virtual void ResetWorkQueueState(void);
	virtual STATE GetWorkQueueItemStateAt(int i);
	virtual STATE GetWorkQueueState(void);

	virtual bool Validate(QString & error, int & failed_item);
//	virtual bool IsValidated(void);

	virtual bool IsStopEnabled(void);
	virtual bool IsPauseEnabled(void);
	virtual bool StartRequest(void);
	virtual bool StopRequest(void);
	virtual bool PauseRequest(void);
	virtual bool IsStopRequested(void);
	virtual bool IsPauseRequested(void);
	virtual bool IsThermalShutdown(void);

	virtual QString GetLastError(void);

	virtual void RegisterUIEnable(QWidget *widget);

	virtual ILC7WorkQueueWidget *GetWorkQueueWidget();

	virtual bool IsCheckpointed(void);
	virtual void ResumeFromCheckpoint(void);
	virtual void SaveCheckpointConfig(QMap<QString, QVariant> checkpoint_config);

	// ILC7SessionHandler
	virtual QUuid GetId();

	virtual void Acquire();
	virtual void Release();

	virtual bool Save(QDataStream & out);
	virtual bool Load(QDataStream & in);
};


#endif
