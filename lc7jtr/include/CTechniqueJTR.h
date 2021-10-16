#ifndef __INC_TECHNIQUEJTR_H
#define __INC_TECHNIQUEJTR_H

#include "lc7api.h"

class CTechniqueJTR:public QObject, public ILC7Component
{
	Q_OBJECT;

public:
	CTechniqueJTR();
	virtual ~CTechniqueJTR();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE  ExecuteCommand(QString command, QStringList args,  QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl=NULL);
	virtual bool ValidateCommand(QMap<QString,QVariant> & state, QString command, QStringList args, QMap<QString,QVariant> & config, QString & error);
};


#endif