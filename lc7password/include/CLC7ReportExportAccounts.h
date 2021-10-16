#ifndef __INC_REPORTEXPORTACCOUNTS_H
#define __INC_REPORTEXPORTACCOUNTS_H

class CLC7ReportExportAccounts :public QObject, public ILC7Component
{
	Q_OBJECT;

private:
	QString m_format;
	QString m_filename;
	QStringList m_columns;

	ILC7AccountList *m_accountlist;

public:
	CLC7ReportExportAccounts();
	virtual ~CLC7ReportExportAccounts();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif