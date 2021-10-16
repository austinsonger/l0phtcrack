#ifndef __INC_ILC7INTERFACE_H
#define __INC_ILC7INTERFACE_H

class ILC7Interface
{
protected:
	virtual ~ILC7Interface() {}

public:

	virtual ILC7Interface *GetInterfaceVersion(QString interface_name)=0;
};

#endif