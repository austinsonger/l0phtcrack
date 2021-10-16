#include<stdafx.h>


CLC7PresetGroup::CLC7PresetGroup(CLC7PresetManager *manager, QString name)
{TR;
	m_manager = manager;
	m_name = name;
}

CLC7PresetGroup::~CLC7PresetGroup()
{TR;
	foreach(CLC7Preset *preset, m_presets_by_id.values())
	{
		delete preset;
	}
}

ILC7Interface *CLC7PresetGroup::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7PresetGroup")
	{
		return this;
	}
	return NULL;
}


bool CLC7PresetGroup::Save(QDataStream &ds)
{TR;
	quint32 version = 1;
	ds << version;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	qint32 cnt = m_presets.size();

	ds << cnt;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	foreach(QUuid preset_id, m_presets)
	{
		ds << preset_id;
		if (ds.status() != QDataStream::Ok)
		{
			return false;
		}

		if (!m_presets_by_id.contains(preset_id))
		{
			return false;
		}
		CLC7Preset *preset = m_presets_by_id[preset_id];
		
		if (!preset->Save(ds))
		{
			return false;
		}
	}

	ds << m_default_preset;

	return true;
}

bool CLC7PresetGroup::Load(QDataStream &ds)
{TR;
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
		QUuid id;

		ds >> id;
		if (ds.status() != QDataStream::Ok)
		{
			return false;
		}

		CLC7Preset *preset = new CLC7Preset(this, id);
		if (!preset->Load(ds))
		{
			delete preset;
			return false;
		}
		
		m_presets.append(id);
		m_presets_by_id[id] = preset;
	}

	ds >> m_default_preset;

	return true;
}

ILC7Preset *CLC7PresetGroup::presetById(QUuid preset_id)
{
	if (!m_presets_by_id.contains(preset_id))
	{
		return NULL;
	}

	return m_presets_by_id[preset_id];
}

int CLC7PresetGroup::idToPosition(QUuid preset_id)
{
	return m_presets.indexOf(preset_id);
}

QUuid CLC7PresetGroup::positionToId(int pos)
{
	if (pos < 0 || pos >=m_presets.size())
	{
		return QUuid();
	}

	return m_presets[pos];
}

int CLC7PresetGroup::presetCount()
{
	return m_presets.size();
}

ILC7Preset *CLC7PresetGroup::presetAt(int pos)
{
	if (pos < 0 || pos >= m_presets.size())
	{
		return NULL;
	}

	QUuid preset_id = m_presets[pos];

	if (!m_presets_by_id.contains(preset_id))
	{
		return false;
	}
		

	return m_presets_by_id[preset_id];
}

ILC7Preset *CLC7PresetGroup::newPreset(int pos, QUuid preset_id)
{TR;
	if (m_presets_by_id.contains(preset_id))
	{
		Q_ASSERT(0);
		return NULL;
	}
	if (preset_id.isNull())
	{
		preset_id = QUuid::createUuid();
	}

	CLC7Preset *preset = new CLC7Preset(this, preset_id);

	if (pos == -1)
	{
		m_presets.append(preset_id);
	}
	else
	{
		m_presets.insert(pos, preset_id);
	}
		
	m_presets_by_id[preset_id] = preset;
	return preset;
}

bool CLC7PresetGroup::deletePresetAt(int pos)
{TR;
	if (pos < 0 || pos >=m_presets.size())
	{
		return false;
	}

	return deletePresetById(m_presets[pos]);
}

bool CLC7PresetGroup::deletePresetById(QUuid preset_id)
{TR;
	if (!m_presets_by_id.contains(preset_id))
	{
		return false;
	}
		
	CLC7Preset *preset = m_presets_by_id[preset_id];
	if (preset == NULL || preset->readonly())
	{
		return false;
	}

	int pos = idToPosition(preset_id);
	if (pos == -1)
	{
		return false;
	}
	
	m_presets.removeAt(pos);
	m_presets_by_id.remove(preset_id);

	delete preset;

	if (m_default_preset == preset_id)
	{
		m_default_preset = QUuid();
	}

	return true;
}

bool CLC7PresetGroup::movePreset(int from, int to)
{TR;
	if (from < 0 || from >= m_presets.size())
	{
		return false;
	}
	if (to < -1 || to >= m_presets.size())
	{
		return false;
	}
	if (to == -1)
	{
		to = m_presets.size() - 1;
	}

	QUuid id = m_presets[from];
	m_presets.removeAt(from);
	m_presets.insert(to, id);

	return true;
}

bool CLC7PresetGroup::copyPreset(int from, int insert_pos)
{TR;
	if (from < 0 || from >= m_presets.size())
	{
		return false;
	}
	if (insert_pos == -1)
	{
		insert_pos = m_presets.size();
	}

	QUuid id = m_presets[from];
	CLC7Preset *preset = m_presets_by_id[id];

	QUuid copyid = QUuid::createUuid();
	CLC7Preset *copy = preset->Copy(copyid);
	
	m_presets.insert(insert_pos, copyid);
	m_presets_by_id[copyid] = copy;

	return true;
}


QUuid CLC7PresetGroup::defaultPreset()
{
	return m_default_preset;
}

void CLC7PresetGroup::setDefaultPreset(QUuid preset_id)
{
	m_default_preset = preset_id;
}
