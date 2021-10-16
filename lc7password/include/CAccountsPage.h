#ifndef __INC_CACCOUNTSPAGE_H
#define __INC_CACCOUNTSPAGE_H

#include <QtWidgets/QMainWindow>
#include <QMenu>

#include "lc7api.h"

#include "ui_accounts.h"

class CLC7AccountList;

class CAccountsPage : public QWidget
{
	Q_OBJECT

private slots:
	void slot_toolsMenuButtonPopup(void);
	void slot_toolsMenuToggled(bool);
	void slot_accountsListContextMenu(const QPoint &pos);
	void slot_copyToClipboard(bool checked);
	void slot_removeAccounts(bool checked);
	void slot_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void slot_resetSortingClicked(bool);
	void slot_remediationAction(void);

public slots:
	void slot_changeNotify(void);
	void slot_helpButtonClicked();
	void slot_uiEnable(bool enable);
	void slot_linkActivated(const QString & link);
	void slot_recolorCallback(void);

signals:
	void sig_doSort(int column, Qt::SortOrder order);

protected:
	virtual void showEvent(QShowEvent *evt);
	virtual void hideEvent(QHideEvent *evt);
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void resizeEvent(QResizeEvent * event);

public:
	CAccountsPage(QWidget *parent = 0);
	virtual ~CAccountsPage();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

private:
	Ui::AccountsForm ui;
	bool m_enable_ui;
	
	int m_selected_count;
	QList<int> m_selected;
	int m_current_status_rows;
	int m_previous_sort;

	CLC7AccountList *m_accountlist;
	ILC7ColorManager *m_colman;
	ILC7WorkQueue *m_single_workqueue;

	QAction *m_copy;
	QAction *m_remove;
	QAbstractButton *m_helpbutton;

	void UpdateTableHeaders();
	void UpdateUI();

	void setSorting(int column, Qt::SortOrder order);

	void UpdateStatusBar(void);
	void UpdateAccountList(CLC7AccountList *acctlist);
	void set_status_rows(int rows);

	typedef bool SELECTION_FILTER(const LC7Account *acct);
	void buildSelection(SELECTION_FILTER *filter, QItemSelection & sel);
};

extern CAccountsPage *CreateAccountsPage();

#endif
