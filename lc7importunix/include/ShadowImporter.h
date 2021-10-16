#ifndef __INC_SHADOWIMPORTER_H
#define __INC_SHADOWIMPORTER_H

class ShadowImporter
{
public:

private:

	friend class ImportShadowConfig;

	quint32 m_account_limit;

	ILC7UnixImporter *m_pimp_oldpasswd;
	ILC7UnixImporter *m_pimp_linux_passwdshadow;
	ILC7UnixImporter *m_pimp_linux_shadow;
	ILC7UnixImporter *m_pimp_solaris_passwdshadow;
	ILC7UnixImporter *m_pimp_solaris_shadow;
	ILC7UnixImporter *m_pimp_bsd_passwdmasterpasswd;
	ILC7UnixImporter *m_pimp_bsd_masterpasswd;
	ILC7UnixImporter *m_pimp_aix_passwdsecuritypasswdsecurityuser;
	ILC7UnixImporter *m_pimp_aix_passwdsecuritypasswd;
	ILC7UnixImporter *m_pimp_aix_securitypasswd;

	QList<ILC7UnixImporter *> m_importers;
	QMap<QString, ILC7UnixImporter *> m_importers_by_name;
	void RegisterImporter(ILC7UnixImporter *pimp);
	void RegisterImporters();

	//////

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;
	LC7Remediations m_remediations;


	void UpdateStatus(QString statustext, quint32 cur, bool statuslog=true);

public:

	enum IMPORT_FLAGS {
		exclude_none = 0,
		exclude_expired = 1,
		exclude_disabled = 2,
		exclude_lockedout = 4
	};

protected:

	bool IncludedInFlags(LC7Account & acct, IMPORT_FLAGS flags);

public:

	ShadowImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~ShadowImporter();

	void SetAccountLimit(quint32 alim);
	void SetRemediations(const LC7Remediations & remediations);
	void GetPasswdImporters(QList<ILC7UnixImporter *> & passwd_importers);
	ILC7UnixImporter *GetPasswdImporter(QString name);
	bool DoImport(QString importer_name, QStringList filenames, FOURCC hashtype, IMPORT_FLAGS flags, QString & error, bool & cancelled);
};


#endif