#ifndef __INC_IMPORTWINDOWSREMOTE_H
#define __INC_IMPORTWINDOWSREMOTE_H

class CImportWindowsRemote:public QObject, public ILC7Component
{
	Q_OBJECT;

private:
	
	QString m_host;
	QString m_username;
	QString m_password;
	QString m_domain;
	QString m_includemachineaccounts;

	ILC7AccountList *m_accountlist;

	bool SaveCreds(QString host, QString username, QString password, QString domain, QString &error);
	bool LoadCreds(QString host, QString &username, QString &password, QString &domain, QString &error);

	ILC7Component::RETURNCODE ImportWithReplication(QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl);
	ILC7Component::RETURNCODE ImportWithAgent(QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl);

public:
	CImportWindowsRemote();
	virtual ~CImportWindowsRemote();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl=NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);

};


#endif