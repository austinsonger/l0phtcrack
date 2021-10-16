#ifndef __INC_IMPORTPWDUMP_H
#define __INC_IMPORTPWDUMP_H

class CImportPWDump :public QObject, public ILC7Component
{
	Q_OBJECT;

private:
	QString m_filename;

	ILC7AccountList *m_accountlist;

public:
	CImportPWDump();
	virtual ~CImportPWDump();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
		
	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif