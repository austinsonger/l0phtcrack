#ifndef __INC_CSCHEDULEPAGE_H
#define __INC_CSCHEDULEPAGE_H

#include <QtWidgets/QMainWindow>
#include "ui_schedule.h"

class CSchedulePage : public QWidget
{
	Q_OBJECT
public slots:
	void slot_uiEnable(bool enable);
	void slot_helpButtonClicked();
	void slot_scheduledTasksList_itemSelectionChanged(void);
	void slot_taskOutputSessionsTable_itemSelectionChanged(void);
	void slot_removeTaskButton_clicked(bool);
	void slot_copyTaskToNewSessionButton_clicked(bool);
	void slot_editTaskScheduleButton_clicked(bool);
	void slot_runTaskNowButton_clicked(void);
	void slot_openTaskOutputButton_clicked(void);
	void slot_removeTaskOutputSessionsButton_clicked(void);
	void slot_taskFinished(ILC7Task *);
	

public:
	CSchedulePage(QWidget *parent = 0);
	virtual ~CSchedulePage();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

	virtual void showEvent(QShowEvent *evt);
	virtual void hideEvent(QHideEvent *evt);

	void UpdateUI();
	void RefreshContent(void);
	void RefreshCurrentlyScheduledTasks(void);
	void RefreshTaskOutputSessions(void);

private:
	Ui::ScheduleForm ui;
	bool m_enable_ui;
	bool m_sch_enabled;
	QAbstractButton *m_helpbutton;

	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;
};

extern CSchedulePage *CreateSchedulePage();

#endif
