#ifndef __INC_CLC77WIZARDPLUGIN_H
#define __INC_CLC77WIZARDPLUGIN_H

#include"lc7api.h"

class CLC7WizardComponent;

class CLC7WizardPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:

	ILC7ActionCategory *m_pWizardCat;
	ILC7Action *m_pWizardAct;
	CLC7WizardComponent *m_pWizard;

public:

	CLC7WizardPlugin();
	virtual ~CLC7WizardPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();
};

#endif