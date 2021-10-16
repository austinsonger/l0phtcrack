#ifndef __INC_IMPORTUNIXSSHGUI_H
#define __INC_IMPORTUNIXSSHGUI_H

#include"../../lc7/include/ILC7GUIComponent.h"

class CImportUnixSSHGUI :public QObject, public ILC7Component
{
	Q_OBJECT;

private:
	friend class ImportUnixSSHConfig;

	static bool SaveCreds(QString host, UnixSSHImporter::AUTHTYPE authtype, QString username, LC7SecureString password, QString privkeyfile, LC7SecureString privkeypassword,
		UnixSSHImporter::ELEVTYPE elevtype, LC7SecureString sudopassword, LC7SecureString  supassword, QString &error);
	static bool LoadCreds(QString host, UnixSSHImporter::AUTHTYPE &authtype, QString &username, LC7SecureString &password, QString &privkeyfile, LC7SecureString &privkeypassword,
		UnixSSHImporter::ELEVTYPE &elevtype, LC7SecureString &sudopassword, LC7SecureString &supassword, QString &error);

public:
	CImportUnixSSHGUI();
	virtual ~CImportUnixSSHGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};



#endif