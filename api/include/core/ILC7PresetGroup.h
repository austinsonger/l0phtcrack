#ifndef __INC_ILC7PRESETGROUP_H
#define __INC_ILC7PRESETGROUP_H

#include"core/ILC7Interface.h"

class ILC7PresetGroup:public ILC7Interface
{
protected:
	virtual ~ILC7PresetGroup() {}
	
public:

	virtual ILC7Preset *presetById(QUuid preset_id)=0;
	
	virtual int idToPosition(QUuid preset_id)=0;
	virtual QUuid positionToId(int pos)=0;

	virtual int presetCount() = 0;
	virtual ILC7Preset *presetAt(int pos) = 0;
	
	virtual ILC7Preset *newPreset(int pos = -1, QUuid uuid = QUuid()) = 0;
	virtual bool deletePresetAt(int pos) = 0;
	virtual bool deletePresetById(QUuid preset_id) = 0;

	virtual bool movePreset(int from, int to) = 0;
	virtual bool copyPreset(int from, int insert_pos = -1) = 0;	

	virtual QUuid defaultPreset() = 0;
	virtual void setDefaultPreset(QUuid preset_id) = 0;
};


#endif