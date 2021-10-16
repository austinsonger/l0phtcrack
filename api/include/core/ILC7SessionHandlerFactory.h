#ifndef __INC_ILC7SESSIONHANDLERFACTORY
#define __INC_ILC7SESSIONHANDLERFACTORY

#include"core/ILC7Interface.h"

#include<quuid.h>

class ILC7SessionHandler;
class ILC7Session;

class ILC7SessionHandlerFactory:public ILC7Interface
{
protected:
	virtual ~ILC7SessionHandlerFactory() {}
	
public:
	virtual QString GetName()=0;
	virtual QString GetDescription()=0;
	
	virtual ILC7SessionHandler *CreateSessionHandler(QUuid handler_id) = 0;
	virtual void DestroySessionHandler(ILC7SessionHandler *handler) = 0;
};


#endif