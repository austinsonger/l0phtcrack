#ifndef __INC_CLC7PASSWORDLINKAGE_H
#define __INC_CLC7PASSWORDLINKAGE_H


class CLC7PasswordLinkage:public QObject, public ILC7PasswordLinkage
{
	Q_OBJECT;

private:

	QList<fourcc> m_types_in_order;
	QMap<fourcc, LC7HashType> m_types;
	QList<ILC7PasswordEngine *> m_engines;

public:
	CLC7PasswordLinkage();
	virtual ~CLC7PasswordLinkage();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);

	// ILC7PasswordLinkage
	virtual void RegisterPasswordEngine(ILC7PasswordEngine *engine);
	virtual void UnregisterPasswordEngine(ILC7PasswordEngine *engine);
	virtual QList<ILC7PasswordEngine *> ListPasswordEngines();
	virtual ILC7PasswordEngine *GetPasswordEngineByID(QUuid id);

	virtual void RegisterHashType(fourcc fcc, QString name, QString description, QString category, QString platform, QUuid plugin);
	virtual void UnregisterHashType(fourcc fcc, QString category, QUuid plugin);
	virtual bool LookupHashType(fourcc fcc, LC7HashType & hashtype, QString & error);
	virtual QList<fourcc> ListHashTypes();

	virtual ILC7CalibrationTable *NewCalibrationTable();
	virtual ILC7CalibrationTable *LoadCalibrationTable(QString table_key);
	virtual bool SaveCalibrationTable(QString table_key, ILC7CalibrationTable *table);

};


#endif