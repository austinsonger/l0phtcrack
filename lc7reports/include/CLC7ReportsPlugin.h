#ifndef __INC_CLC7REPORTSPLUGIN_H
#define __INC_CLC7REPORTSPLUGIN_H

#include"CReports.h"
#include"CReportsGUI.h"

class CLC7ReportsPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:

	CReports *m_pReports;
	CReportsGUI *m_pReportsGUI;
	ILC7ActionCategory *m_pReportsCat;
	ILC7Action *m_pReportsAct;
	
public:

	CLC7ReportsPlugin();
	virtual ~CLC7ReportsPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();
};

#endif