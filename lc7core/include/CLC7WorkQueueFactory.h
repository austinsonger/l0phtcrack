#ifndef __INC_CLC7WORKQUEUEFACTORY_H
#define __INC_CLC7WORKQUEUEFACTORY_H

#include "lc7api.h"

class CLC7Controller;

class CLC7WorkQueueFactory:public ILC7SessionHandlerFactory
{
public:
	CLC7Controller *m_controller;
	bool m_validate_limits;
	
public:
	CLC7WorkQueueFactory(CLC7Controller *controller, bool validate_limits);
	virtual ~CLC7WorkQueueFactory();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	
	virtual QString GetName();
	virtual QString GetDescription();

	virtual ILC7SessionHandler *CreateSessionHandler(QUuid handler_id);
	virtual void DestroySessionHandler(ILC7SessionHandler *handler);
};

#endif