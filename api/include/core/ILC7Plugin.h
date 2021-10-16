#ifndef __INC_ILC7PLUGIN_H
#define __INC_ILC7PLUGIN_H

#include"core/ILC7Interface.h"

#include<qstring.h>
#include<quuid.h>
#include<qdatetime.h>

class ILC7Plugin:public ILC7Interface
{

protected:
	virtual ~ILC7Plugin() {}

public:
	
	virtual QUuid GetID()=0;
	virtual QList<QUuid> GetInternalDependencies()=0;

	virtual bool Activate()=0;
	virtual bool Deactivate()=0;
};


#endif