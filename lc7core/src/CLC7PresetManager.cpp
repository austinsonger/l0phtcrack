#include<stdafx.h>



CLC7PresetManager::CLC7PresetManager(CLC7Controller *ctrl)
{TR;
	m_ctrl = ctrl;
}

CLC7PresetManager::~CLC7PresetManager()
{TR;
	Q_ASSERT(m_groups.size() == 0);
}

ILC7Interface *CLC7PresetManager::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7PresetManager")
	{
		return this;
	}
	return NULL;
}

bool CLC7PresetManager::Initialize(QString &error, bool &presets_ok)
{TR;
	if (!Load())
	{
		presets_ok = false;
		Reset();
		return true;
	}

	presets_ok = true;
	return true;
}

void CLC7PresetManager::Reset()
{TR;
	foreach(CLC7PresetGroup *group, m_groups.values())
	{
		delete group;
	}
	m_groups.clear();

	Save();
}

void CLC7PresetManager::Terminate()
{TR;
	if (!Save())
	{
		Q_ASSERT(0);
	}
	foreach(CLC7PresetGroup *group, m_groups.values())
	{
		delete group;
	}
	m_groups.clear();
}


bool CLC7PresetManager::Save()
{TR;
	QByteArray data;
	QDataStream ds(&data, QIODevice::WriteOnly);

	ILC7Settings *settings = m_ctrl->GetSettings();

	quint32 version = 1;
	ds << version;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	qint32 cnt = m_groups.size();

	ds << cnt;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	foreach(QString key, m_groups.keys())
	{
		ds << key;
		if (ds.status() != QDataStream::Ok)
		{
			return false;
		}

		CLC7PresetGroup *group = m_groups[key];

		if (!group->Save(ds))
		{
			return false;
		}
	}

	settings->setValue("_core_:presets", QVariant(data));

	return true;
}

bool CLC7PresetManager::Load()
{TR;
	ILC7Settings *settings = m_ctrl->GetSettings();
	
	if (!settings->contains("_core_:presets"))
	{
		return true;
	}
	QByteArray data = settings->value("_core_:presets").toByteArray();
	if (data.size() == 0)
	{
		return true;
	}

	QDataStream ds(&data, QIODevice::ReadOnly);

	quint32 version;
	ds >> version;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}
	if (version != 1)
	{
		return false;
	}

	qint32 cnt;
	ds >> cnt;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	for (int i = 0; i < cnt; i++)
	{
		QString key;
		ds >> key;
		if (ds.status() != QDataStream::Ok)
		{
			return false;
		}

		CLC7PresetGroup *group = new CLC7PresetGroup(this, key);
		if (!group->Load(ds))
		{
			delete group;
			return false;
		}

		m_groups[key] = group;
	}

	return true;
}

void CLC7PresetManager::flush(void)
{TR;
	Save();
}


ILC7PresetGroup *CLC7PresetManager::newPresetGroup(QString groupname)
{TR;
	if (m_groups.contains(groupname))
	{
		return NULL;
	}

	CLC7PresetGroup *group = new CLC7PresetGroup(this, groupname);
	m_groups[groupname] = group;

	Save();

	return group;
}

bool CLC7PresetManager::deletePresetGroup(QString groupname)
{TR;
	if (!m_groups.contains(groupname))
	{
		return false;
	}

	CLC7PresetGroup *group = m_groups[groupname];
	m_groups.remove(groupname);
	delete group;

	Save();

	return true;
}

ILC7PresetGroup *CLC7PresetManager::presetGroup(QString groupname)
{
	if (!m_groups.contains(groupname))
	{
		return NULL;
	}
	return m_groups[groupname];
}

ILC7Preset *CLC7PresetManager::presetById(QUuid preset_id)
{
	foreach(CLC7PresetGroup *group, m_groups)
	{
		ILC7Preset *preset = group->presetById(preset_id);
		if (preset)
		{
			return preset;
		}
	}

	return NULL;
}