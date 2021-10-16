#ifndef __INC_CLC7UNIXIMPORTER_LINUX_H
#define __INC_CLC7UNIXIMPORTER_LINUX_H

class CLC7UnixImporter_LINUX : public CLC7UnixImporter
{
protected:
	CLC7UnixImporter_LINUX(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	bool CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray &hash, bool &disabled, bool &lockedout);
	bool GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes);
	bool GetShadowHashTypes(QString filename, QList<FOURCC> & hashtypes);

	void ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account);
};

class CLC7UnixImporter_LINUX_PASSWDSHADOW :public CLC7UnixImporter_LINUX
{
public:
	CLC7UnixImporter_LINUX_PASSWDSHADOW(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

class CLC7UnixImporter_LINUX_SHADOW :public CLC7UnixImporter_LINUX
{
public:
	CLC7UnixImporter_LINUX_SHADOW(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);

	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

#endif