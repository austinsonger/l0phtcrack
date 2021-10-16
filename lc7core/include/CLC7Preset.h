#ifndef __INC_CLC7PRESET_H
#define __INC_CLC7PRESET_H

class CLC7PresetGroup;

class CLC7Preset :public ILC7Preset
{
protected:
	friend class CLC7PresetGroup;

	CLC7PresetGroup *m_group;
	QUuid m_id;
	bool m_readonly;
	
	QString m_name;
	QString m_description;
	QVariant m_config; 

	bool Save(QDataStream &ds);
	bool Load(QDataStream &ds);

	CLC7Preset *Copy(QUuid copyid);

public:

	CLC7Preset(CLC7PresetGroup *group, QUuid id);
	virtual ~CLC7Preset();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual bool readonly();
	virtual void setReadonly(bool readonly);

	virtual int position();
	virtual QUuid id();

	virtual QString name();
	virtual void setName(QString name);

	virtual QString description();
	virtual void setDescription(QString description);

	virtual QVariant config();
	virtual void setConfig(QVariant config);
};


#endif