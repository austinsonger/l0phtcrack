#include<stdafx.h>
	
CLC7HistoricalData::CLC7HistoricalData(CLC7Controller *controller)
{TR;
	m_controller=controller;
	m_db_opened=false;
}

CLC7HistoricalData::~CLC7HistoricalData()
{TR;
	Q_ASSERT(m_db_opened==false);
}


ILC7Interface *CLC7HistoricalData::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7HistoricalData")
	{
		return this;
	}
	return NULL;
}

QString CLC7HistoricalData::CreateNewDataFilePath()
{TR;
	QString dataloc = m_controller->GetAppDataDirectory();
	QDir dfdir(dataloc);
	if(!dfdir.exists())
	{		dfdir.mkpath(".");
	}

	QString hdpath=dfdir.absoluteFilePath("historical_data_"+QUuid::createUuid().toString());
	return hdpath;
}

bool CLC7HistoricalData::Initialize(QString & error)
{TR;
	if(m_db_opened)
	{
		Q_ASSERT(0);
		return false;
	}

	ILC7Settings *settings=m_controller->GetSettings();

	QString datafilepath;
	if(!settings->contains("_core_:historical_data_file_path"))
	{
		datafilepath=CreateNewDataFilePath();
	}
	else
	{
		datafilepath=settings->value("_core_:historical_data_file_path").toString();
	}

	m_db = QSqlDatabase::addDatabase("QSQLITE","CLC7HistoricalData");
	m_db.setDatabaseName(datafilepath);
	if(!m_db.open())
	{
		QSqlError err=m_db.lastError();
		error=QString("Unable to open database driver: ")+err.text();

		QSqlDatabase::removeDatabase("CLC7HistoricalData");

		return false;
	}
	m_db_opened=true;

	settings->setValue("_core_:historical_data_file_path", datafilepath);

/*
	{
		QStringList testdata;
		testdata.append("12345");
		testdata.append("asdfhjklqwertyuipzxcvbnm");
	
		QString err;
		if(!AddData("test",QUuid::createUuid(),QDateTime::currentDateTime(),testdata,err))
		{
			Q_ASSERT(0);
		}
	}
	*/

	return true;
}

void CLC7HistoricalData::Terminate(void)
{TR;
	if(m_db_opened)
	{
		m_db.close();
		m_db_opened = false;
	}
	if(QSqlDatabase::contains("CLC7HistoricalData"))
	{
		QSqlDatabase::removeDatabase("CLC7HistoricalData");
	}
	
}

bool CLC7HistoricalData::IsInitialized(void)
{
	return m_db_opened;
}

bool CLC7HistoricalData::SetDataFilePath(QString path, bool allow_overwrite, QString & error)
{TR;
	ILC7Settings *settings=m_controller->GetSettings();

	bool is_open=m_db.isOpen();
	if(is_open)
	{
		Terminate();
	}

	bool has_old_file_path=settings->contains("_core_:historical_data_file_path");
	QString old_data_file_path;
	if(has_old_file_path)
	{
		old_data_file_path=settings->value("_core_:historical_data_file_path").toString();
	}

	// Try out new data file path
	settings->setValue("_core_:historical_data_file_path",path);
	
	// Try to initialize if things were open
	if(is_open && !Initialize(error))
	{
		// Restore old data file path
		if(has_old_file_path)
		{
			settings->setValue("_core_:historical_data_file_path",old_data_file_path);
		}

		// If was opened, reinitialize
		if(is_open)
		{
			QString reiniterror;
			if(!Initialize(reiniterror))
			{
				error=reiniterror;
				return false;
			}
		}

		return false;
	}

	return true;
}

QString CLC7HistoricalData::GetDataFilePath()
{TR;
	ILC7Settings *settings=m_controller->GetSettings();
	if(!settings->contains("_core_:historical_data_file_path"))
	{
		return "";
	}
	QString p=settings->value("_core_:historical_data_file_path",QVariant("")).toString();
	return p;
}

bool CLC7HistoricalData::GetTables(QStringList &tables, QString &error)
{TR;
	// Create tables if they don't exist
	tables = m_db.tables();
	return true;
}


bool CLC7HistoricalData::AddData(QString table, QUuid uuid, QDateTime timestamp, QVariant data, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Create tables if they don't exist
	{
		QSqlQuery q(m_db);
		q.prepare("CREATE TABLE IF NOT EXISTS :table(uuid STRING PRIMARY KEY ASC, timestamp INTEGER, data BLOB)");
		q.bindValue(":table", table);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
	}

	// Insert row
	{
		QSqlQuery q(m_db);
		q.prepare("INSERT INTO :table (uuid, timestamp, data) VALUES (:uuid, :timestamp, :data)");
		q.bindValue(":table",table);
		q.bindValue(":uuid",uuid.toString());
		q.bindValue(":timestamp",timestamp.toMSecsSinceEpoch());
		q.bindValue(":data",data);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
	}

	return true;
}

bool CLC7HistoricalData::GetDataDetailByUuid(QString table, QUuid uuid, QDateTime &timestamp, QVariant & data, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("SELECT timestamp, data FROM :table WHERE uuid=:uuid");
		q.bindValue(":table", table);
		q.bindValue(":uuid",uuid.toString());
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
		while (q.next())
		{
			qlonglong tsmsec = q.value(0).toLongLong();
			timestamp = QDateTime::fromMSecsSinceEpoch(tsmsec);
			data = q.value(1);
		}
	}

	return true;
}

bool CLC7HistoricalData::GetDataInTimeRange(QString table, QDateTime from, QDateTime to, QList<QUuid> & uuids, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("SELECT uuid FROM :table WHERE timestamp BETWEEN :from AND :to ORDER BY timestamp");
		q.bindValue(":table", table);
		q.bindValue(":from",from.toMSecsSinceEpoch());
		q.bindValue(":to",to.toMSecsSinceEpoch());
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
		while (q.next())
		{
			QUuid uuid = q.value(0).toUuid();
			uuids.append(uuid);
		}
	}

	return true;
}

bool CLC7HistoricalData::GetDataCount(QString table, quint32 & count, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("SELECT count(*) FROM :table");
		q.bindValue(":table", table);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
		while (q.next())
		{
			quint32 cnt = q.value(0).toInt();
			count = cnt;
		}
	}

	return true;
}

bool CLC7HistoricalData::GetDataAtIndex(QString table, quint32 idx, QUuid & uuid, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("SELECT uuid FROM :table ORDER BY timestamp LIMIT 1 OFFSET :idx");
		q.bindValue(":table", table);
		q.bindValue(":idx", idx);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
		while (q.next())
		{
			uuid = q.value(0).toUuid();
		}
	}

	return true;
}

bool CLC7HistoricalData::GetDataInIndexRange(QString table, quint32 from_idx, quint32 to_idx, QList<QUuid> & uuids, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("SELECT uuid FROM :table ORDER BY timestamp LIMIT :limit OFFSET :offset");
		q.bindValue(":table", table);
		q.bindValue(":limit",(to_idx-from_idx)+1);
		q.bindValue(":offset",from_idx);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
		while (q.next())
		{
			QUuid uuid = q.value(0).toUuid();
			uuids.append(uuid);
		}
	}

	return true;

}

bool CLC7HistoricalData::RemoveDataByUuid(QString table, QUuid uuid, QString & error)
{TR;

	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("DELETE FROM :table WHERE uuid=:uuid");
		q.bindValue(":table",table);
		q.bindValue(":uuid",uuid);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
	}

	return true;
}

bool CLC7HistoricalData::ClearData(QString table, QString & error)
{TR;
	if (!IsInitialized())
	{
		return false;
	}

	// Select data
	{
		QSqlQuery q(m_db);
		q.prepare("DROP TABLE :table");
		q.bindValue(":table",table);
		if(!q.exec())
		{
			error = q.lastError().text();
			return false;
		}
	}

	return true;
}


