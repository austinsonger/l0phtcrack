#include"stdafx.h"

CLC7Preset::CLC7Preset(CLC7PresetGroup *group, QUuid id)
{TR;
	m_group = group;
	m_id = id;
	m_readonly = false;
}

CLC7Preset *CLC7Preset::Copy(QUuid copyid)
{TR;
	CLC7Preset *copy = new CLC7Preset(m_group, copyid);

	copy->m_name = m_name;
	copy->m_description = m_description;
	copy->m_config = m_config;

	return copy;
}

CLC7Preset::~CLC7Preset()
{TR;

}


ILC7Interface *CLC7Preset::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Preset")
	{
		return this;
	}
	return NULL;
}



bool CLC7Preset::Save(QDataStream &ds)
{TR;
	quint32 version = 1;
	ds << version;

	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds << m_readonly;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds << m_name;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds << m_description;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds << m_config;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return true;
}

bool CLC7Preset::Load(QDataStream &ds)
{TR;
	quint32 version;
	ds >> version;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds >> m_readonly;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds >> m_name;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds >> m_description;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	ds >> m_config;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return true;
}

bool CLC7Preset::readonly()
{
	return m_readonly;
}

void CLC7Preset::setReadonly(bool readonly)
{
	m_readonly = readonly;
}

int CLC7Preset::position()
{
	return m_group->idToPosition(m_id);
}

QUuid CLC7Preset::id()
{
	return m_id;
}

QString CLC7Preset::name()
{
	return m_name;
}

void CLC7Preset::setName(QString name)
{
	m_name = name;
}

QString CLC7Preset::description()
{
	return m_description;
}

void CLC7Preset::setDescription(QString description)
{
	m_description = description;
}

QVariant CLC7Preset::config()
{
	return m_config;
}

void CLC7Preset::setConfig(QVariant config)
{
	m_config = config;
}


