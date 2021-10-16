#ifndef __INC_ILC7PRESET_H
#define __INC_ILC7PRESET_H

#include"core/ILC7Interface.h"

class ILC7Preset:public ILC7Interface
{
protected:
	virtual ~ILC7Preset() {}

public:
	
	virtual bool readonly()=0;
	virtual void setReadonly(bool readonly)=0;

	virtual int position()=0;
	virtual QUuid id()=0;

	virtual QString name()=0;
	virtual void setName(QString setName)=0;

	virtual QString description()=0;
	virtual void setDescription(QString description)=0;

	virtual QVariant config()=0;
	virtual void setConfig(QVariant config)=0;
};


#endif