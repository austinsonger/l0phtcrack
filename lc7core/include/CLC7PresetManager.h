#ifndef __INC_CLC7PRESETMANAGER_H
#define __INC_CLC7PRESETMANAGER_H

class CLC7PresetGroup;

class CLC7PresetManager:public ILC7PresetManager
{
protected:
	friend class CLC7PresetGroup;

	CLC7Controller *m_ctrl;

	QMap<QString, CLC7PresetGroup *> m_groups;

	bool Save();
	bool Load();

public:
	CLC7PresetManager(CLC7Controller *ctrl);
	virtual ~CLC7PresetManager();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	void Reset();

	bool Initialize(QString &error, bool &presets_ok);
	void Terminate();

	virtual void flush(void);

	virtual ILC7PresetGroup *newPresetGroup(QString groupname);
	virtual bool deletePresetGroup(QString groupname);
	virtual ILC7PresetGroup *presetGroup(QString groupname);

	virtual ILC7Preset *presetById(QUuid preset_id);
};


#endif