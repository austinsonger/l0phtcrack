#ifndef __INC_IMPORTWINDOWSLOCALGUI_H
#define __INC_IMPORTWINDOWSLOCALGUI_H

#include"../../lc7/include/ILC7GUIComponent.h"

class CImportWindowsLocalGUI:public QObject,public ILC7Component
{
	Q_OBJECT;

private:
	bool SaveCreds(QString username, LC7SecureString password, QString domain, QString &error);
	bool LoadCreds(QString &username, LC7SecureString &password, QString &domain, QString &error);

public:
	CImportWindowsLocalGUI();
	virtual ~CImportWindowsLocalGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};



#endif