#ifndef __INC_CLC7ACCOUNTLISTFACTORY_H
#define __INC_CLC7ACCOUNTLISTFACTORY_H

class CLC7AccountListFactory:public ILC7SessionHandlerFactory
{
private:
	
public:
	CLC7AccountListFactory();
	virtual ~CLC7AccountListFactory();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QString GetName();
	virtual QString GetDescription();

	virtual ILC7SessionHandler *CreateSessionHandler(QUuid handler_id);
	virtual void DestroySessionHandler(ILC7SessionHandler *handler);
};

#endif