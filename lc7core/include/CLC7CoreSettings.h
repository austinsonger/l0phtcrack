#ifndef __INC_CLC7CORESETTINGS_H
#define __INC_CLC7CORESETTINGS_H


#ifndef UUID_LC7CORESETTINGS
#define UUID_LC7CORESETTINGS QUuid("{bfc0d6ed-ff0d-4a1b-a06d-8f85b4a38381}")
#endif

class CLC7CoreSettings:public QObject, public ILC7Component
{
	Q_OBJECT;

private:

	ILC7Linkage *m_pLinkage;

	bool GetOptions(QMap<QString, QVariant> & config, QString & error);
	virtual void AddOption(QList<QVariant> & keys,
		QString settingskey,
		QString name,
		QString desc,
		QVariant default_value,
		bool require_restart = false,
		QString option1key = QString(), QVariant option1value = QVariant(),
		QString option2key = QString(), QVariant option2value = QVariant(),
		QString option3key = QString(), QVariant option3value = QVariant(),
		QString option4key = QString(), QVariant option4value = QVariant(),
		QString option5key = QString(), QVariant option5value = QVariant());

public:
	CLC7CoreSettings(ILC7Linkage *pLinkage);
	virtual ~CLC7CoreSettings();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl=NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif