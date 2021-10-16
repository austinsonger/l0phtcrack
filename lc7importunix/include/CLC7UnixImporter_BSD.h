#ifndef __INC_CLC7UNIXIMPORTER_BSD_H
#define __INC_CLC7UNIXIMPORTER_BSD_H

class CLC7UnixImporter_BSD : public CLC7UnixImporter
{
protected:
	CLC7UnixImporter_BSD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	bool CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray &hash, bool &disabled, bool &lockedout);
	bool GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes);
	bool GetMasterPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes);
	bool ImportMasterPasswdFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled);

	void ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account);

};

class CLC7UnixImporter_BSD_PASSWDMASTERPASSWD :public CLC7UnixImporter_BSD
{
public:
	CLC7UnixImporter_BSD_PASSWDMASTERPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

class CLC7UnixImporter_BSD_MASTERPASSWD :public CLC7UnixImporter_BSD
{
public:
	CLC7UnixImporter_BSD_MASTERPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

#endif