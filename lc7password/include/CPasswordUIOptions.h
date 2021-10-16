#ifndef __INC_CPASSWORDUIOPTIONS_H
#define __INC_CPASSWORDUIOPTIONS_H


class CPasswordUIOptions:public QObject, public ILC7Component
{
	Q_OBJECT;

private:

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
	CPasswordUIOptions();
	virtual ~CPasswordUIOptions();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif