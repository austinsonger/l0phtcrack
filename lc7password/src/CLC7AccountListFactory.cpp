#include"stdafx.h"

CLC7AccountListFactory::CLC7AccountListFactory()
{TR;
}

CLC7AccountListFactory::~CLC7AccountListFactory()
{TR;
}


ILC7Interface *CLC7AccountListFactory::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7AccountListFactory")
	{
		return this;
	}
	return NULL;
}

QString CLC7AccountListFactory::GetName()
{TR;
	return "CLC7AccountList";
}

QString CLC7AccountListFactory::GetDescription()
{TR;
	return "LC7 Account List";
}

ILC7SessionHandler *CLC7AccountListFactory::CreateSessionHandler(QUuid handler_id)
{ 
	return new CLC7AccountList(handler_id);
}

void CLC7AccountListFactory::DestroySessionHandler(ILC7SessionHandler *handler)
{
	delete (CLC7AccountList *)handler;
}


