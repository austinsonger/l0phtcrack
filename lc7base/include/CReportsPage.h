#ifndef __INC_CREPORTSPAGE_H
#define __INC_CREPORTSPAGE_H

#include <QtWidgets/QMainWindow>
#include <QStandardItemModel>

#include "lc7api.h"

#include "ui_reports.h"

class CReportsPage : public QWidget
{
	Q_OBJECT
private:
	QStandardItemModel *m_report_tree_model;
	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;
	bool m_is_valid;

public:
	CReportsPage(QWidget *parent = 0);
	virtual ~CReportsPage();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

public slots:
	void onTreeEntered(const QModelIndex &selected);
	void onTreeViewportEntered(void);
	void onTreeCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected);
	void onAddQueueButton(bool checked = false);
	void onRunButton(bool checked = false);
	void slot_isValid(bool);
	void RecolorCallback(void);

public slots:
	void slot_uiEnable(bool enable);
	void slot_helpButtonClicked();

protected:
	virtual void showEvent(QShowEvent *evt);
	virtual void hideEvent(QHideEvent *evt);

private:
	Ui::ReportsForm ui;
	bool m_enable_ui;
	QAbstractButton *m_helpbutton;

	void setDescriptionText(QModelIndex selected);
	void RefreshContent();
	void AddCategory(ILC7ActionCategory *cat, QStandardItem *item, bool flatten);
	void UpdateUI();

};

extern CReportsPage *CreateReportsPage();

#endif
