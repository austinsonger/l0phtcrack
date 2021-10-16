#ifndef __INC_ILC7COMPONENT_H
#define __INC_ILC7COMPONENT_H

#include"core/ILC7Interface.h"

#include <quuid.h>
#include <qstring.h>
#include <qvariant.h>
#include <qmap.h>

class ILC7CommandControl;

class ILC7Component:public ILC7Interface
{
protected:
	virtual ~ILC7Component() {}
	
public:
	enum RETURNCODE
	{
		FAIL=-1,
		SUCCESS=0,
		PAUSED=1,
		STOPPED=2
	};

	virtual QUuid GetID()=0;
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl=NULL)=0;
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error) = 0;
};

#endif