#ifndef __INC_ILC7HISTORICALDATA_H
#define __INC_ILC7HISTORICALDATA_H

#include"core/ILC7Interface.h"

#include<quuid.h>
#include<qdatetime.h>
#include<qstring.h>

class ILC7HistoricalData:public ILC7Interface
{
protected:

	virtual ~ILC7HistoricalData() {}

public:

	virtual bool SetDataFilePath(QString path, bool allow_overwrite, QString & error)=0;
	virtual QString GetDataFilePath()=0;

	virtual bool AddData(QString table, QUuid uuid, QDateTime timestamp, QVariant data, QString & error)=0;
	virtual bool GetDataDetailByUuid(QString table, QUuid uuid, QDateTime &timestamp, QVariant & data, QString & error)=0;

	virtual bool GetDataInTimeRange(QString table, QDateTime from, QDateTime to, QList<QUuid> & uuids, QString & error)=0;
	virtual bool GetDataCount(QString table, quint32 & count, QString & error)=0;
	virtual bool GetDataAtIndex(QString table, quint32 idx, QUuid & uuid, QString & error)=0;
	virtual bool GetDataInIndexRange(QString table, quint32 from_idx, quint32 to_idx, QList<QUuid> & uuids, QString & error)=0;
	
	virtual bool RemoveDataByUuid(QString table, QUuid uuid, QString & error)=0;

	virtual bool ClearData(QString table, QString & error)=0;
};

#endif
