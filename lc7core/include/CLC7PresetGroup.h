#ifndef __INC_CLC7PRESETGROUP_H
#define __INC_CLC7PRESETGROUP_H

class CLC7PresetManager;
class CLC7Preset;

class CLC7PresetGroup:public ILC7PresetGroup
{
protected:
	friend class CLC7PresetManager;

	CLC7PresetManager *m_manager;

	QString m_name;
	QMap<QUuid, CLC7Preset *> m_presets_by_id;
	QList<QUuid> m_presets;
	QUuid m_default_preset;

	bool Save(QDataStream &ds);
	bool Load(QDataStream &ds);

public:
	CLC7PresetGroup(CLC7PresetManager *manager, QString name);
	virtual ~CLC7PresetGroup();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual ILC7Preset *presetById(QUuid preset_id);

	virtual int idToPosition(QUuid preset_id);
	virtual QUuid positionToId(int pos);

	virtual int presetCount();
	virtual ILC7Preset *presetAt(int pos);

	virtual ILC7Preset *newPreset(int pos = -1, QUuid uuid=QUuid());
	virtual bool deletePresetAt(int pos);
	virtual bool deletePresetById(QUuid preset_id);

	virtual bool movePreset(int from, int to);
	virtual bool copyPreset(int from, int insert_pos = -1);

	virtual QUuid defaultPreset();
	virtual void setDefaultPreset(QUuid preset_id);
};


#endif