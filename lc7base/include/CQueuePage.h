#ifndef __INC_CQUEUEPAGE_H
#define __INC_CQUEUEPAGE_H

#include <QtWidgets/QMainWindow>
#include "ui_queue.h"

class CQueuePage : public QWidget
{
	Q_OBJECT

public slots:
	void onRemoveButton(bool checked = false);
	void onMoveUpButton(bool checked = false);
	void onMoveDownButton(bool checked = false);
    void onQueueTableCurrentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);
	void onRunQueueButton(bool checked = false);
	void onValidateQueueButton(bool checked = false);
	void onScheduleQueueButton(bool checked = false);
	void onQueueChanged(void);
	void RecolorCallback(void);

public slots:
	void slot_uiEnable(bool enable);
	void slot_helpButtonClicked();

public:
	CQueuePage(QWidget *parent = 0);
	virtual ~CQueuePage();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

private:
	Ui::QueueForm ui;
	bool m_enable_ui;
	bool m_sch_enabled;
	QAbstractButton *m_helpbutton;

	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;

	void UpdateUI(void);
	void RefreshContent();
	void showEvent(QShowEvent *evt);
	void hideEvent(QHideEvent *evt);
};

extern CQueuePage *CreateQueuePage();

#endif
