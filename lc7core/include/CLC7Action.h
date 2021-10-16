#ifndef __INC_CLC7ACTION_H
#define __INC_CLC7ACTION_H

#include<QObject>
#include<QUuid>
#include<QIcon>
#include<QString>

#include"lc7api.h"

class CLC7ActionCategory;

class CLC7Action:public QObject, public ILC7Action
{
	Q_OBJECT;
private:
	friend class CLC7ActionCategory;

	QUuid m_id;
	QUuid m_componentid;
	QString m_command;
	QStringList m_args;
	QString m_name;
	QString m_desc;
	QIcon m_icon;

	CLC7ActionCategory *m_category;
	int m_refcount;

protected:

	virtual int Ref();
	virtual int Unref();

	static QList<CLC7Action *> s_dead_actions;
	static void GarbageCollect(void);

public:
	CLC7Action(CLC7ActionCategory *category, QUuid componentid, QString command, QStringList args, QString name, QString desc, QIcon icon=QIcon());
	virtual ~CLC7Action();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual ILC7ActionCategory *ParentCategory();
	virtual QUuid ComponentId();
	virtual QString Command();
	virtual QStringList Args();
	virtual QString Name();
	virtual QString Desc();
	virtual QIcon Icon();

};

#endif