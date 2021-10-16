#ifndef __INC_ILC7ACTIONCATEGORY_H
#define __INC_ILC7ACTIONCATEGORY_H

#include "core/ILC7Interface.h"

#include<qstring.h>
#include<qicon.h>
#include<quuid.h>

class ILC7Action;

class ILC7ActionCategory:public ILC7Interface
{
protected:
	virtual ~ILC7ActionCategory() {}
	
public:

	virtual ILC7ActionCategory *ParentCategory()=0;
	virtual QString InternalName()=0;
	virtual QString Name()=0;
	virtual QString Desc()=0;
	virtual QIcon Icon()=0;
	
	virtual quint32 GetGenerationNumber() = 0;

	virtual ILC7ActionCategory *CreateActionCategory(QString internal_name, QString name, QString desc, QIcon icon=QIcon())=0;
	virtual void RemoveActionCategory(ILC7ActionCategory *cat) = 0;
	virtual QList<ILC7ActionCategory *> GetActionCategories()=0;

	virtual ILC7Action *CreateAction(QUuid componentid, QString command, QStringList args, QString name, QString desc, QIcon icon=QIcon())=0;
	virtual void RemoveAction(ILC7Action *action) = 0;
	virtual QList<ILC7Action *> GetActions()=0;

};

#endif