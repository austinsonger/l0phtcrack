#ifndef __INC_IMPORTWINDOWSREMOTEGUI_H
#define __INC_IMPORTWINDOWSREMOTEGUI_H

#include"../../lc7/include/ILC7GUIComponent.h"

class CImportWindowsRemoteGUI:public QObject, public ILC7Component
{
	Q_OBJECT;

public:
	CImportWindowsRemoteGUI();
	virtual ~CImportWindowsRemoteGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);

private:
	
	bool SaveCreds(QString host, QString username, LC7SecureString password, QString domain, QString &error);
	bool LoadCreds(QString host, QString &username, LC7SecureString &password, QString &domain, QString &error);

	void GenerateRemoteAgent(void);

};


#endif