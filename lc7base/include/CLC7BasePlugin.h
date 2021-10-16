#ifndef __INC_CLC7BASEPLUGIN_H
#define __INC_CLC7BASEPLUGIN_H

#include"lc7api.h"

#include"CReportsPage.h"
#include"CQueuePage.h"
#include"CSchedulePage.h"
#include"CHelpPage.h"

class CLC7BasePlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:
	CReportsPage *m_pReportsPage;
	CQueuePage *m_pQueuePage;
	CSchedulePage *m_pSchedulePage;
	CHelpPage *m_pHelpPage;

public:

	CLC7BasePlugin();
	virtual ~CLC7BasePlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();


};

#endif