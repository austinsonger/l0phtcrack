#ifndef __INC_NTDSIMPORTER_H
#define __INC_NTDSIMPORTER_H

#include"utils.h"
struct hive;

class NTDSImporter
{
private:
	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;
	int m_account_limit;
	bool m_with_history;
	bool m_include_machine_accounts;

	void UpdateStatus(QString statustext, quint32 cur, bool statuslog = true);
	bool AddEntry(LDAPAccountInfo ldapAccountInfo, QString &error);
	quint32 FindControlSet(struct hive *system_hive);
	unsigned char *GetBootKey(struct hive *system_hive, size_t &bootkeylen);

public:

	NTDSImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~NTDSImporter();

	void SetAccountLimit(quint32 alim);
	void SetIncludeMachineAccounts(bool include_machine_accounts);
	void SetWithHistory(bool with_history);
	bool DoImport(QString ntds_filename, QString system_filename, QString & error, bool & cancelled);
};


#endif