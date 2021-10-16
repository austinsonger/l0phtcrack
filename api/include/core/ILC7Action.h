#ifndef __INC_ILC7ACTION_H
#define __INC_ILC7ACTION_H

#include "core/ILC7Interface.h"

#include <qicon.h>
#include <quuid.h>

class ILC7ActionCategory;

class ILC7Action:public ILC7Interface
{
protected:
	virtual ~ILC7Action() {}

public:
	
	virtual ILC7ActionCategory *ParentCategory()=0;
	virtual QUuid ComponentId()=0;
	virtual QString Command()=0;
	virtual QStringList Args() = 0;
	virtual QString Name() = 0;
	virtual QString Desc()=0;
	virtual QIcon Icon()=0;

};

#endif