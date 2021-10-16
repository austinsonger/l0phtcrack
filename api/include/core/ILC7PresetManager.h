#ifndef __INC_ILC7PRESETMANAGER_H
#define __INC_ILC7PRESETMANAGER_H

#include"core/ILC7Interface.h"

class ILC7PresetManager:public ILC7Interface
{
protected:
	virtual ~ILC7PresetManager() {}

public:
	
	virtual void flush(void) = 0;

	virtual ILC7PresetGroup *newPresetGroup(QString groupname)=0;
	virtual bool deletePresetGroup(QString groupname)=0;
	virtual ILC7PresetGroup *presetGroup(QString groupname)=0;

	virtual ILC7Preset *presetById(QUuid preset_id)=0;
};


#endif