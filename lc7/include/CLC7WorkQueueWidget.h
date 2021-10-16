#ifndef __INC_CLC7WORKQUEUEWIDGET_H
#define __INC_CLC7WORKQUEUEWIDGET_H

#include "ui_LC7WorkQueueWidget.h"

class ILC7Controller;
class CAnimatedBarChart;

class CLC7WorkQueueWidget :public QWidget, public ILC7WorkQueueWidget
{
	Q_OBJECT

private:

	struct PROGRESSCTX {
		CAnimatedBarChart *widget;
		QString barid;
		qint32 newval;
		qint64 newtime;
		qint32 oldval;
		qint64 oldtime;
	};
	
	CAnimatedBarChart *m_cpuprog;
	CAnimatedBarChart *m_gpuprog;
	QLabel *m_thermal;

	ILC7Controller *m_ctrl;
	Ui::LC7WorkQueueWidget ui;
	bool m_is_new_line;
	QString m_last_strdate;
	QMovie *m_waitingMovie;
	QTextDocument *m_status_document;
	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;
	int m_active_workqueue;

	QMutex m_appendStringLock;
	QString m_appendString;
	QTimer m_append_to_activity_log_later_timer;
	bool m_append_to_activity_log_later;

	QList<PROGRESSCTX> m_cpu_progressbars;
	QList<PROGRESSCTX> m_gpu_progressbars;
	QThread m_sysmon_animate_thread;
	QElapsedTimer m_sysmon_elapsed;

protected:
	void UpdateCounts(void);
	void UpdateQueueIcon(ILC7WorkQueue *workqueue);
	void CreateSystemMonitor(void);
	void UpdateSystemMonitor(void);

	void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
	void appendLine(QString strdate, QString strtime, QString line);
	void NotifyThermalTransition(ILC7ThermalWatchdog::THERMAL_STATE oldstate, ILC7ThermalWatchdog::THERMAL_STATE newstate);
	void UpdateThermalState();

signals:
	void sig_updateCurrentActivity(QString text);
	void sig_scrollToBottom();
	void sig_clearActivityLog();
	void sig_setStatusText(QString text);
	void sig_updateCurrentProgressBar(quint32 cur);
	void sig_updateTotalProgressBar(quint32 cur);
	void sig_updateThermalState(void);

public slots:
	void RecolorCallback(void);

private slots:
	void slot_append_to_activity_log_later(void);
	void slot_pauseButton();
	void slot_stopButton();
	void slot_updateCurrentActivity(QString text);
	void slot_clearActivityLog();
	void slot_scrollToBottom();
	void slot_setStatusText(QString text);
	void slot_updateCurrentProgressBar(quint32 cur);
	void slot_updateTotalProgressBar(quint32 cur);
	void onBatchWorkQueueChanged(void);
	void onSingleWorkQueueChanged(void);
	void slot_createSystemMonitor(void);
	void slot_animateSystemMonitor(void);
	void slot_updateThermalState(void);

public:
	CLC7WorkQueueWidget(QWidget *parent, ILC7Controller *ctrl);
	virtual ~CLC7WorkQueueWidget();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	
	virtual void Shutdown(void);
	virtual void Decorate(void);
	virtual void UpdateUI(void);

	virtual void UpdateCurrentActivity(QString text);	
	virtual void ClearActivityLog();
	virtual void ScrollToBottom();
	virtual void AppendToActivityLog(QString text);
	virtual void SetStatusText(QString text);
	virtual void UpdateCurrentProgressBar(quint32 cur);
	virtual void UpdateTotalProgressBar(quint32 cur);

	virtual bool Save(QDataStream & ds);
	virtual bool Load(QDataStream & ds);
	
	virtual void Reset(void);

};

#endif