#include"stdafx.h"

CLC7PasswordLinkage::CLC7PasswordLinkage()
{TR;
}

CLC7PasswordLinkage::~CLC7PasswordLinkage()
{TR;
}


ILC7Interface *CLC7PasswordLinkage::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7PasswordLinkage")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7PasswordLinkage::GetID()
{TR;
	return UUID_LC7PASSWORDLINKAGE;
}

void CLC7PasswordLinkage::RegisterHashType(fourcc fcc, QString name, QString description, QString category, QString platform, QUuid plugin)
{TR;
	if (m_types.contains(fcc))
	{
		m_types[fcc].registrants[category].insert(plugin);
	}
	else
	{
		m_types[fcc].fourcc = fcc;
		m_types[fcc].name = name;
		m_types[fcc].description = description;
		m_types[fcc].platform = platform;
		m_types[fcc].registrants[category].insert(plugin);
		
		m_types_in_order.append(fcc);
	}
}

void CLC7PasswordLinkage::UnregisterHashType(fourcc fcc, QString category, QUuid plugin)
{
	TR;
	if (!m_types.contains(fcc))
	{
		Q_ASSERT(0);
		return;
	}

	if (!m_types[fcc].registrants.contains(category))
	{
		Q_ASSERT(0);
		return;
	}

	m_types[fcc].registrants[category].remove(plugin);

	if (m_types[fcc].registrants[category].size()==0)
	{
		m_types[fcc].registrants.remove(category);
		if (m_types[fcc].registrants.size() == 0)
		{
			m_types.remove(fcc);
			m_types_in_order.removeAll(fcc);
		}
	}
}

bool CLC7PasswordLinkage::LookupHashType(fourcc fcc, LC7HashType & accttype, QString & error)
{TR;
	if (!m_types.contains(fcc))
	{
		error = "account type not registered";
		return false;
	}

	accttype = m_types[fcc];

	return true;
}

QList<fourcc> CLC7PasswordLinkage::ListHashTypes()
{TR;
	return m_types_in_order;
}


ILC7Component::RETURNCODE CLC7PasswordLinkage::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	return FAIL;
}

bool CLC7PasswordLinkage::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return false;
}

void CLC7PasswordLinkage::RegisterPasswordEngine(ILC7PasswordEngine *engine)
{
	if (!m_engines.contains(engine))
	{
		m_engines.append(engine);
	}
	else
	{
		Q_ASSERT(0);
	}
}

void CLC7PasswordLinkage::UnregisterPasswordEngine(ILC7PasswordEngine *engine)
{
	if (m_engines.contains(engine))
	{
		m_engines.removeOne(engine);
	}
	else
	{
		Q_ASSERT(0);
	}
}

QList<ILC7PasswordEngine *> CLC7PasswordLinkage::ListPasswordEngines(void)
{
	return m_engines;
}

ILC7PasswordEngine *CLC7PasswordLinkage::GetPasswordEngineByID(QUuid id)
{
	for (auto engine : m_engines)
	{
		if (engine->GetID() == id)
		{
			return engine;
		}
	}
	return nullptr;
}




ILC7CalibrationTable *CLC7PasswordLinkage::NewCalibrationTable()
{
	return new CLC7CalibrationTable();
}

ILC7CalibrationTable *CLC7PasswordLinkage::LoadCalibrationTable(QString table_key)
{
	ILC7Settings *settings = g_pLinkage->GetSettings();
	QVariant ctvar = settings->value(table_key);
	if (ctvar.isNull())
	{
		return nullptr;
	}

	QByteArray ba(ctvar.toByteArray());
	QDataStream ds(&ba, QIODevice::ReadOnly);
	
	CLC7CalibrationTable *pct = new CLC7CalibrationTable();
	ds >> *pct;

	if (ds.status() != QDataStream::Ok)
	{
		delete pct;
		return nullptr;
	}
	
	return pct;
}

bool CLC7PasswordLinkage::SaveCalibrationTable(QString table_key, ILC7CalibrationTable *table)
{
	QByteArray ba;
	{
		QDataStream ds(&ba, QIODevice::WriteOnly);

		ds << *(CLC7CalibrationTable *)table;

		if (ds.status() != QDataStream::Ok)
		{
			return false;
		}
	}

	ILC7Settings *settings = g_pLinkage->GetSettings();
	settings->setValue(table_key, ba);

	return true;
}
