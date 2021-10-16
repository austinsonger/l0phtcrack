#ifndef __INC_CLC7IMPORTWINPLUGIN_H
#define __INC_CLC7IMPORTWINPLUGIN_H

#include"CImportWindowsRemote.h"
#include"CImportWindowsRemoteGUI.h"
#include"CImportWindowsLocal.h"
#include"CImportWindowsLocalGUI.h"
#include"CImportSAM.h"
#include"CImportSAMGUI.h"
#include"CImportNTDS.h"
#include"CImportNTDSGUI.h"
#include"CImportPWDump.h"
#include"CImportPWDumpGUI.h"
#include"CWindowsImportSettings.h"

class CLC7ImportWinPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:

	CImportSAM * m_pImportSAM;
	CImportSAMGUI * m_pImportSAMGUI;
	CImportNTDS * m_pImportNTDS;
	CImportNTDSGUI * m_pImportNTDSGUI;
	CImportPWDump * m_pImportPWDump;
	CImportPWDumpGUI * m_pImportPWDumpGUI;
	CImportWindowsRemote *m_pImportWinRemote;
	CImportWindowsRemoteGUI *m_pImportWinRemoteGUI;
	CImportWindowsLocal *m_pImportWinLocal;
	CImportWindowsLocalGUI *m_pImportWinLocalGUI;
	ILC7ActionCategory *m_pImportCat;
	ILC7ActionCategory *m_pLocalCat;
	ILC7ActionCategory *m_pRemoteCat;
	ILC7ActionCategory *m_pFileCat;
	ILC7Action *m_pLocalImportAct;
	ILC7Action *m_pRemoteImportAct;
	ILC7Action *m_pFileImportAct;
	ILC7Action *m_pSAMImportAct;
	ILC7Action *m_pNTDSImportAct;
	ILC7ActionCategory *m_pCommandsCat;
	ILC7Action *m_pGenerateRemoteAgentAct;

	ILC7ActionCategory *m_pSystemCat;
	ILC7Action *m_pWindowsImportSettingsAct;

	CWindowsImportSettings *m_pWindowsImportSettings;

	bool m_use_chalresp;

public:

	CLC7ImportWinPlugin();
	virtual ~CLC7ImportWinPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool UseChallengeResponse();
	
	virtual bool Activate();
	virtual bool Deactivate();
};

#endif