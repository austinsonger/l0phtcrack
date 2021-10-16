#include"stdafx.h"

CLC7WorkQueueFactory::CLC7WorkQueueFactory(CLC7Controller *controller, bool validate_limits)
{TR;
	m_controller=controller;
	m_validate_limits = validate_limits;
}

CLC7WorkQueueFactory::~CLC7WorkQueueFactory()
{TR;
}


ILC7Interface *CLC7WorkQueueFactory::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7WorkQueueFactory")
	{
		return this;
	}
	return NULL;
}


QString CLC7WorkQueueFactory::GetName()
{TR;
	return "CLC7WorkQueue";
}

QString CLC7WorkQueueFactory::GetDescription()
{TR;
	return "LC7 Work Queue";
}

ILC7SessionHandler *CLC7WorkQueueFactory::CreateSessionHandler(QUuid handler_id)
{ 
	return new CLC7WorkQueue(handler_id, m_controller, m_validate_limits);
}

void CLC7WorkQueueFactory::DestroySessionHandler(ILC7SessionHandler *handler)
{
	delete (CLC7WorkQueue *)handler;
}
