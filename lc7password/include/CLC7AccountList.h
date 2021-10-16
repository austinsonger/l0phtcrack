#ifndef __INC_CLC7ACCOUNTLIST_H
#define __INC_CLC7ACCOUNTLIST_H

Q_DECLARE_METATYPE(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST);

class CLC7AccountList:public QObject, public ILC7AccountList
{
	Q_OBJECT;

private:

	QUuid m_handler_id;
	QMutex m_acquire;
	bool m_is_acquired;

	int m_account_limit;

	QList<LC7Account> m_accounts;

	struct REMEDIATIONS
	{
		qint32 refcount;			// -1 if dead slot, 0 if brand new, refcount otherwise
		LC7Remediations remediations;
	};

	QList<REMEDIATIONS> m_remediations;
	
	STATS m_stats;
	void add_to_stats(const LC7Account &acct);
	void remove_from_stats(const LC7Account &acct);
	void clear_stats(void);
	bool increase_remediation_refcount(const LC7Account &acct);
	bool decrease_remediation_refcount(const LC7Account &acct);

	ACCOUNT_UPDATE_ACTION_LIST m_updates;
	void AddUpdateAction(ACCOUNT_UPDATE_ACTION *update);
	void SendUpdates();

	void ReportModified(void);

signals:

	void sig_update(ACCOUNT_UPDATE_ACTION_LIST updates);
	void sig_direct_update(ACCOUNT_UPDATE_ACTION_LIST updates);

public:
	
	CLC7AccountList(QUuid handler_id);
	virtual ~CLC7AccountList();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// Get notified when changes happen to this list
	virtual void RegisterUpdateNotification(QObject *slot_obj, void (QObject::*slot_func)(ACCOUNT_UPDATE_ACTION_LIST));

	// Actions that don't require acquired state (but may be use in acquired state)
	virtual int GetAccountCount() const;
	virtual STATS GetStats() const;

	// Actions that require acquired state
	// Acquire first, do not acquire or do any heavy lifting on the gui thread, use update notifications instead.
	virtual bool ClearAccounts();
	virtual const LC7Account *GetAccountAtConstPtrFast(int pos) const;
	virtual bool AppendAccount(const LC7Account &acct);
	virtual bool InsertAccount(int pos, const LC7Account &acct);
	virtual bool ReplaceAccountAt(int pos, const LC7Account & acct);
	virtual bool RemoveAccounts(const std::set<int> &positions);

	// Add remediation action for accounts
	virtual int AddRemediations(const LC7Remediations &remediations);
	virtual bool GetRemediations(int num, LC7Remediations &remediations);

	// ILC7SessionHandler
	virtual QUuid GetId();

	virtual void Acquire();
	virtual void Release();

	virtual bool Save(QDataStream & out);
	virtual bool Load(QDataStream & in);
};


#endif