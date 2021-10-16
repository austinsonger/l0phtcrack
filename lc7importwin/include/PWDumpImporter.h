#ifndef __INC_PWDUMPIMPORTER_H
#define __INC_PWDUMPIMPORTER_H

class PWDumpImporter
{
private:


	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;
	bool m_parse_cr;
	int m_account_limit;

	void UpdateStatus(QString statustext, quint32 cur, bool statuslog=true);

	bool IsNT(QStringList parts);
	bool IsCR(QStringList parts);
	bool ParseNT(QStringList parts, LC7Account & acct);
	bool ParseCR(QStringList parts, LC7Account & acct);

public:

	PWDumpImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~PWDumpImporter();

	void SetAccountLimit(quint32 alim);
	bool GetHashTypes(QString filename, QSet<fourcc> & hashtypes, QString & error);
	bool DoImport(QString filename, QString & error, bool & cancelled);
};


#endif