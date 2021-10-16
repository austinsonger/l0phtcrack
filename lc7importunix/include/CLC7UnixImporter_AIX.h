#ifndef __INC_CLC7UNIXIMPORTER_AIX_H
#define __INC_CLC7UNIXIMPORTER_AIX_H


class CLC7UnixImporter_AIX :public CLC7UnixImporter
{
protected:
	bool ParseStanzaFile(QString filename, QMap<QString, QMap<QString, QString> > & stanzas, QString &error);
	bool CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray &hash, bool &disabled, bool &lockedout);
	bool GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes);
	bool GetAIXSecurityHashTypes(QString filename, QList<FOURCC> & hashtypes);
	bool VerifyAIXStanzaFile(QString filename);

	bool ImportAIXSecurityPasswdFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled);
	bool ImportAIXSecurityUserFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled);

	void ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account);

	CLC7UnixImporter_AIX(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
};

class CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER : public CLC7UnixImporter_AIX
{
public:
	CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

class CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD : public CLC7UnixImporter_AIX
{
public:
	CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};


class CLC7UnixImporter_AIX_SECURITYPASSWD : public CLC7UnixImporter_AIX
{
public:
	CLC7UnixImporter_AIX_SECURITYPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

#endif
