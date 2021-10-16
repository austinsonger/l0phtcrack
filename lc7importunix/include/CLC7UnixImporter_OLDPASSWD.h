#ifndef __INC_CLC7UNIXIMPORTER_OLDPASSWD_H
#define __INC_CLC7UNIXIMPORTER_OLDPASSWD_H

class CLC7UnixImporter_OLDPASSWD :public CLC7UnixImporter
{
protected:
	bool CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray &hash, bool &disabled, bool &lockedout);
	bool GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes);
	void ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account);

public:
	CLC7UnixImporter_OLDPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	virtual QString name();
	virtual QString desc();
	virtual QStringList filetypes();
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes);
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled);
};

#endif