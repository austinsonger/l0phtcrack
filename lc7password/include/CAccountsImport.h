#ifndef __INC_CACCOUNTSIMPORT_H
#define __INC_CACCOUNTSIMPORT_H

class QPrinter;

class CAccountsImport:public QObject
{
private:
	Q_OBJECT;

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;
	ILC7PasswordLinkage *m_plink;

    QString m_filename;
	
protected:
	
public:

	CAccountsImport(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~CAccountsImport();

	void setFilename(QString filename);
	
	bool DoImport(QString &error, bool &cancelled);
};

#endif