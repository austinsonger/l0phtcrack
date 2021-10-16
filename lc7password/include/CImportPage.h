#ifndef __INC_CIMPORTPAGE_H
#define __INC_CIMPORTPAGE_H

#include <QtWidgets/QMainWindow>
#include "ui_import.h"

class CImportPage : public QWidget
{
	Q_OBJECT

public:
	CImportPage(QWidget *parent = 0);
	virtual ~CImportPage();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

public slots:		
	void onTreeEntered(const QModelIndex &selected);
	void onTreeViewportEntered(void);
	void onTreeCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected);
	void onAddQueueButton(bool checked = false);
	void onRunButton(bool checked=false);
	void RecolorCallback(void);
public slots:
	void slot_uiEnable(bool enable);
	void slot_helpButtonClicked();

private slots:
	void slot_isValid(bool valid);

protected:
	virtual void showEvent(QShowEvent *evt);
	virtual void hideEvent(QHideEvent *evt);
	
private:
	Ui::ImportForm ui;
	bool m_enable_ui;
	bool m_is_valid;
	int m_last_content_gen;
	QStandardItemModel *m_import_tree_model;
	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;
	ILC7AccountList *m_accountlist;
	QAbstractButton *m_helpbutton;

	void setDescriptionText(QModelIndex selected);
	void RefreshContent();
	void AddCategory(ILC7ActionCategory *cat, QStandardItem *item, bool flatten);
	void UpdateUI();

};

extern CImportPage *CreateImportPage();

#endif
