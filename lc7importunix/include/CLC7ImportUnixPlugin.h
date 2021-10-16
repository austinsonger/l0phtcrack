#ifndef __INC_CLC7IMPORTUNIXPLUGIN_H
#define __INC_CLC7IMPORTUNIXPLUGIN_H

#include"CImportShadow.h"
#include"CImportShadowGUI.h"
#include"CImportUnixSSH.h"
#include"CImportUnixSSHGUI.h"

class CLC7ImportUnixPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:

	CImportShadow * m_pImportShadow;
	CImportShadowGUI * m_pImportShadowGUI;
	CImportUnixSSH * m_pImportUnixSSH;
	CImportUnixSSHGUI * m_pImportUnixSSHGUI;
	ILC7ActionCategory *m_pImportCat;
	ILC7ActionCategory *m_pFileCat;
	ILC7ActionCategory *m_pRemoteCat;
	ILC7Action *m_pShadowImportAct;
	ILC7Action *m_pSSHImportAct;

public:

	CLC7ImportUnixPlugin();
	virtual ~CLC7ImportUnixPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();
};

#endif