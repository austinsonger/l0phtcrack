#ifndef __INC_ILC7ACCOUNTLIST_H
#define __INC_ILC7ACCOUNTLIST_H

#include"core/ILC7Interface.h"
#include<set>
#include "LC7Account.h"

class ILC7AccountList:public ILC7SessionHandler
{

public:

	struct STATS {
		quint32 total;
		quint32 cracked;
		quint32 partially_cracked;
		quint32 locked_out;
		quint32 disabled;
		quint32 must_change;
		quint32 non_expiring;
	};

	enum SORT_DIRECTION 
	{
		DOWN=0,
		UP=1
	};

	enum ACCOUNT_UPDATE_ACTION_TYPE
	{
		ACCT_CLEAR = 0,
		ACCT_APPEND = 1,
		ACCT_INSERT = 2,
		ACCT_REPLACE = 3,
		ACCT_REMOVE = 4
	};

	struct ACCOUNT_UPDATE_ACTION
	{
		ACCOUNT_UPDATE_ACTION_TYPE type;
		int index;
		int count;
		std::set<int> positions;
		QList<LC7Account> accts;
	};

	typedef QList<ACCOUNT_UPDATE_ACTION *> ACCOUNT_UPDATE_ACTION_LIST;

protected:	
	virtual ~ILC7AccountList() {}
	
public:
	// Get notified when changes happen to this list
	virtual void RegisterUpdateNotification(QObject *slot_obj, void (QObject::*slot_func)(ACCOUNT_UPDATE_ACTION_LIST)) = 0;

	// Actions that don't require acquired state (but may be use in acquired state)
	virtual int GetAccountCount() const = 0;
	virtual STATS GetStats() const = 0;
		
	// Actions that require acquired state
	// Acquire first, do not acquire or do any heavy lifting on the gui thread, use update notifications instead.
	virtual bool ClearAccounts() = 0;
	virtual const LC7Account *GetAccountAtConstPtrFast(int pos) const = 0;
	virtual bool AppendAccount(const LC7Account &acct) = 0;
	virtual bool InsertAccount(int pos, const LC7Account &acct)=0;
	virtual bool ReplaceAccountAt(int pos, const LC7Account & acct) = 0;
	virtual bool RemoveAccounts(const std::set<int> &positions) = 0;

	// Add remediation action for accounts
	virtual int AddRemediations(const LC7Remediations &remediations) = 0;
	virtual bool GetRemediations(int num, LC7Remediations &remediations) = 0;
};

#define ACCOUNTLIST_HANDLER_ID QUuid("{b0a9f82d-63ad-4c64-a8ca-e0de854ea8fd}")

#endif