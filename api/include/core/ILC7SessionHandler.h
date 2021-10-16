#ifndef __INC_ILC7SESSIONHANDLER_H
#define __INC_ILC7SESSIONHANDLER_H

#include"core/ILC7Interface.h"

#include<quuid.h>

class ILC7SessionHandler:public ILC7Interface
{
protected:
	virtual ~ILC7SessionHandler() {}
	
public:
	virtual QUuid GetId()=0;

	virtual void Acquire()=0;
	virtual void Release()=0;

	virtual bool Save(QDataStream & out)=0;
	virtual bool Load(QDataStream & in)=0;
};

#endif
