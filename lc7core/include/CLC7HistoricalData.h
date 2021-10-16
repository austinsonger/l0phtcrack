#ifndef __INC_CLC7HISTORICALDATA_H
#define __INC_CLC7HISTORICALDATA_H

#include<QSqlDatabase>

class CLC7Controller;

class CLC7HistoricalData:public QObject, public ILC7HistoricalData
{
	Q_OBJECT;
private:
	CLC7Controller *m_controller;
	QSqlDatabase m_db;
	bool m_db_opened;

	QString CreateNewDataFilePath();
	
public:

	CLC7HistoricalData(CLC7Controller *controller);
	virtual ~CLC7HistoricalData();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual bool Initialize(QString & error);
	virtual void Terminate(void);
	virtual bool IsInitialized(void);

	virtual bool SetDataFilePath(QString path, bool allow_overwrite, QString & error);
	virtual QString GetDataFilePath();

	virtual bool GetTables(QStringList &tables, QString &error);

	virtual bool AddData(QString table, QUuid uuid, QDateTime timestamp, QVariant data, QString & error);
	virtual bool GetDataDetailByUuid(QString table, QUuid uuid, QDateTime &timestamp, QVariant & data, QString & error);

	virtual bool GetDataInTimeRange(QString table, QDateTime from, QDateTime to, QList<QUuid> & uuids, QString & error);
	virtual bool GetDataCount(QString table, quint32 & count, QString & error);
	virtual bool GetDataAtIndex(QString table, quint32 idx, QUuid & uuid, QString & error);
	virtual bool GetDataInIndexRange(QString table, quint32 from_idx, quint32 to_idx, QList<QUuid> & uuids, QString & error);
	
	virtual bool RemoveDataByUuid(QString table, QUuid uuid, QString & error);

	virtual bool ClearData(QString table, QString & error);
};

#endif
