#ifndef __INC_ACCOUNTSLISTMODEL_H
#define __INC_ACCOUNTSLISTMODEL_H

#include<QAbstractTableModel>

class CLC7AccountList;

class AccountsListModel : public QAbstractTableModel
{
	Q_OBJECT
	friend class SortedAccountsListModel;

	ILC7AccountList *m_accounts;

	QList<QVector<QVariant> > m_normal_table;
	QMap<fourcc, QList<QVector<QVariant> > > m_hash_table;
		
	struct HASH_COLUMN
	{
		fourcc hashtype;
		size_t count;
	};

	QVector<size_t> m_normal_column_count;
	QVector<HASH_COLUMN> m_hash_column_count;
	
	int m_columncount;
	int m_rowcount;
	bool m_columnschanged;

	ILC7ColorManager *m_colman;
	QPixmap m_check_pixmap;
	QPixmap m_blank_pixmap;
	QIcon m_lock_icon;
	QIcon m_disabled_icon;
	QIcon m_expired_icon;
	QIcon m_infinity_icon;
	QColor m_textcolor;
	QColor m_inversetextcolor;

	QTimer m_enableSortingTimer;
	ILC7ProgressBox *m_progress_box;

	void add_columns(const LC7Account *acct);
	void remove_columns(int pos);
	void update_column_count(void);
	bool get_column_type(int realcolumn, int & col, fourcc &hashtype, int &hcolnum) const;
	int get_crackstatecol(int row, int hcolnum) const;
	int get_crackstatesum(int row) const;


public:

    AccountsListModel(QObject *parent, ILC7AccountList *accounts);
	virtual ~AccountsListModel();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	bool lessThan(const QModelIndex & left, const QModelIndex & right) const;

	void setProgressBox(ILC7ProgressBox *box);

private:

	QVariant verticalHeaderData(int section, int role) const;
	QVariant horizontalHeaderData(int section, int role) const;

	void do_full_update(void);

	void update_action_clear();
	void update_action_append(const LC7Account *acct);
	void update_action_insert(int pos, const LC7Account *acct);
	void update_action_replace(int pos, const LC7Account *acct);
	void update_action_remove(int pos);

	void disableSortingBriefly();

signals:
	void sig_modelchanged(void);
	void sig_enable_sorting(bool enable);

private slots:
	void slot_update_action(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST updates);
	void slot_recolor_callback(void);
	void slot_enable_sorting_timer(void);

};

#endif