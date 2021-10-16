#ifndef __INC_CREPORTS_H
#define __INC_CREPORTS_H

#include<QObject>

#include "lc7api.h"

class CReports:public QObject, public ILC7Component
{
	Q_OBJECT;

private:

public:
	CReports();
	virtual ~CReports();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif