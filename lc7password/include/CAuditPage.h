#ifndef __INC_CAUDITPAGE_H
#define __INC_CAUDITPAGE_H

#include <QtWidgets/QMainWindow>
#include <QStandardItemModel>

#include "lc7api.h"

#include "ui_audit.h"

class CAuditPage : public QWidget
{
	Q_OBJECT
private:
	ILC7ColorManager *m_colman;
	QStandardItemModel *m_technique_tree_model;
	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;
	ILC7AccountList *m_accountlist;
	bool m_is_valid;
	quint32 m_last_content_gen;

public:
	CAuditPage(QWidget *parent = 0);
	virtual ~CAuditPage();
	
	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
	
public slots:
	void onTreeEntered(const QModelIndex &selected);
	void onTreeViewportEntered(void);
	void onTreeCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected);
	void onAddQueueButton(bool checked = false);
	void onRunButton(bool checked=false);
	void slot_isValid(bool);
	void RecolorCallback(void);

public slots:	
	void slot_uiEnable(bool enable);
	void slot_helpButtonClicked();

protected:
	virtual void showEvent(QShowEvent *evt);
	virtual void hideEvent(QHideEvent *evt);
	
private:
	Ui::AuditForm ui;
	bool m_enable_ui;
	QAbstractButton *m_helpbutton;

	void setDescriptionText(QModelIndex selected);
	void RefreshContent();
	void AddCategory(ILC7ActionCategory *cat, QStandardItem *item, bool flatten);
	void UpdateUI();

};

extern CAuditPage *CreateAuditPage();

#endif
