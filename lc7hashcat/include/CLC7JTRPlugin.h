#ifndef __INC_CLC7TECHNIQUEJTRPLUGIN_H
#define __INC_CLC7TECHNIQUEJTRPLUGIN_H

#include"lc7api.h"

#include"CSystemJTR.h"
#include"CTechniqueJTR.h"
#include"CTechniqueJTRSingleGUI.h"
#include"CTechniqueJTRBruteGUI.h"
#include"CTechniqueJTRDictionaryGUI.h"

class CLC7JTRPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:

	CSystemJTR *m_pSystemJTR;
	CTechniqueJTR *m_pTechniqueJTR;
	CTechniqueJTRSingleGUI *m_pTechniqueJTRSingleGUI;
	CTechniqueJTRBruteGUI *m_pTechniqueJTRBruteGUI;
	CTechniqueJTRDictionaryGUI *m_pTechniqueJTRDictionaryGUI;
	ILC7ActionCategory *m_pTechniqueCat;
	ILC7ActionCategory *m_pBaseCat;
	ILC7ActionCategory *m_pSystemCat;
	ILC7Action *m_pSingleAct;
	ILC7Action *m_pBruteAct;
	ILC7Action *m_pDictionaryAct;
	ILC7Action *m_pSettingsAct;
	ILC7Action *m_pCalibrateAct;
	QList<fourcc> m_supportedAccountTypes;

	void CreatePresetGroups(void);
	void CreateAccountTypes(void);
	
	void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

private slots:
	void slot_checkCalibration(void);

signals:
	void sig_checkCalibration(void);

public:

	CLC7JTRPlugin();
	virtual ~CLC7JTRPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();

	void CheckCalibration(void);
};

#endif